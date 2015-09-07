#!/usr/bin/python

#from PIL import Image, ImageFilter

#with Image.open("forum.jpg") as img:
#    print(img.size)
#    blurred = img.filter(ImageFilter.GaussianBlur(96))
#    blurred.save("blurred.jpg", "JPEG")

import sqlite3
import os
from flask import Flask, json, send_file, g, request, redirect, abort
from werkzeug.utils import secure_filename

DATABASE = "database.db"
UPLOAD_FOLDER = "./images/"

app = Flask(__name__)
app.config.from_object(__name__)

@app.route("/")
def index():
    return "Wall Art"

def connect_db():
    return sqlite3.connect(app.config["DATABASE"])

def get_db():
    db = getattr(g, "_database", None)
    if db is None:
        db = g._database = connect_db()
    return db

@app.teardown_appcontext
def close_db(exception):
    db = getattr(g, "_database", None)
    if db is not None:
        db.close()

def query_db(query, args=(), one=False):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return (rv[0] if rv else None) if one else rv

# Build a list of all images as an array of dicts, with one dict per image
# to be easily returned as a JSON string
def build_image_dict(query_result):
    images = []
    for img in query_result:
        images.append({"id": img[0], "title": img[1], "artist": img[2],
            "work_type": img[3], "culture": img[4], "has_nudity": img[5],
            "filename": img[6], "blurred_filename": img[7]})
    return images

# API endpoint to get list of all images on server
@app.route("/api/image", methods=["GET"])
def get_images():
    images = build_image_dict(query_db("select * from images"))
    return json.dumps(images, ensure_ascii=False).encode("utf8")

# API endpoint to get information about image <id>
@app.route("/api/image/<int:image_id>", methods=["GET"])
def get_image_info(image_id):
    img = query_db("select * from images where id = ?", [image_id], one=True)
    if img is None:
        abort(404)
    image_info = {"id": img[0], "title": img[1], "artist": img[2],
            "work_type": img[3], "culture": img[4], "has_nudity": img[5],
            "filename": img[6], "blurred_filename": img[7]}
    return json.dumps(image_info, ensure_ascii=False).encode("utf8")

# API endpoint to get the original image for image <id>
@app.route("/api/image/<int:image_id>/original", methods=["GET"])
def get_original_image(image_id):
    img = query_db("select * from images where id = ?", [image_id], one=True)
    if img is None:
        abort(404)
    return send_file(img[6], mimetype="image/jpeg")

# API endpoint to get blurred image with some id
@app.route("/api/image/<int:image_id>/blurred", methods=["GET"])
def get_blurred_image(image_id):
    img = query_db("select * from images where id = ?", [image_id], one=True)
    if img is None:
        abort(404)
    return send_file(img[7], mimetype="image/jpeg")

@app.route("/api/artist/<artist_name>")
def get_artist_paintings(artist_name):
    artist = query_db("select * from images where artist = ?", [artist_name])
    if artist is None:
        abort(404)
    images = build_image_dict(artist)
    return json.dumps(images, ensure_ascii=False).encode("utf8")

@app.route("/upload", methods=["GET", "POST"])
def upload_image():
    if request.method == "POST":
        f = request.files["file"]
        bf = request.files["blurred_file"]
        if f and bf:
            filename = os.path.join(app.config["UPLOAD_FOLDER"],
                    secure_filename(f.filename))
            blurred_filename = os.path.join(app.config["UPLOAD_FOLDER"],
					secure_filename(bf.filename))
            f.save(filename)
            bf.save(blurred_filename)
            if query_db("select * from images where title = ?",
                    [request.form["title"]], one=True) is None:
                db = get_db()
                # TODO: We should actually have a check box for has nudity and
                # also accept a second file upload for the blurred file
                db.execute("""insert into images (title, artist, work_type,
                    culture, has_nudity, filename, blurred_filename)
                    values (?, ?, ?, ?, ?, ?, ?)""", [request.form["title"],
                        request.form["artist"], request.form["work_type"],
                        request.form["culture"], False, filename, blurred_filename])
                db.commit()
        return redirect("/upload")

    return """
    <!DOCTYPE html5>
    <title>Upload image</title>
    </h1>Upload File</h1>
    <form action="" method=post enctype=multipart/form-data>
        <p>
		   <label>Original Image
           <input type=file name=file>
		   </label>
		   <label>Blurred Image
           <input type=file name=blurred_file>
		   </label>
           <label>Title
           <input type=text name=title>
           </label>
           <label>Artist
           <input type=text name=artist>
           </label>
           <label>Work Type
           <input type=text name=work_type>
           </label>
           <label>Culture
           <input type=text name=culture>
           </label>
           <input type=submit value=Upload>
        </p>
    </form>
    """

if __name__ == "__main__":
    app.run(debug=True)

