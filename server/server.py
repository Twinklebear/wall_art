#!/usr/bin/python

import sqlite3
import os
import datetime
from flask import Flask, json, send_file, g, request, \
        redirect, abort, url_for, render_template
from werkzeug.utils import secure_filename
from PIL import Image

DATABASE = "database.db"
UPLOAD_FOLDER = "./images/"

app = Flask(__name__)
app.config.from_object(__name__)

@app.route("/")
def index():
    return "Wall Art"

def get_db():
    db = getattr(g, "_database", None)
    if db is None:
        db = g._database = sqlite3.connect(app.config["DATABASE"])
    return db

@app.teardown_appcontext
def close_db(exception):
    db = getattr(g, "_database", None)
    if db is not None:
        db.close()

def query_db(query, args=()):
    cur = get_db().execute(query, args)
    rv = cur.fetchall()
    cur.close()
    return rv

# Build a list of all images as an array of dicts, with one dict per image
# to be easily returned as a JSON string
def build_image_dict(query_result):
    images = []
    for img in query_result:
        images.append({"id": img[0], "title": img[1], "artist": img[2],
            "work_type": img[3], "has_nudity": img[4], "filename": img[5]})
    return images

# API endpoint to get list of all images on server
@app.route("/api/image", methods=["GET"])
def get_images():
    images = build_image_dict(query_db("select * from images;"))
    return json.dumps(images, ensure_ascii=False).encode("utf8")

# API endpoint to get information about image <id>
@app.route("/api/image/<int:image_id>", methods=["GET"])
def get_image_info(image_id):
    img = query_db("select * from images where id = ?", [image_id])
    if img is None:
        abort(404)
    img = img[0]

    image_info = {"id": img[0], "title": img[1], "artist": img[2],
            "work_type": img[3], "has_nudity": img[4], "filename": img[5]}
    return json.dumps(image_info, ensure_ascii=False).encode("utf8")

# API endpoint to get the original image for image <id>
@app.route("/api/image/<int:image_id>/original", methods=["GET"])
def get_original_image(image_id):
    img = query_db("select * from images where id = ?", [image_id])
    if img is None:
        abort(404)
    return send_file(img[0][5], mimetype="image/jpeg")

@app.route("/upload", methods=["GET", "POST"])
def upload_image():
    if request.method == "POST":
        try:
            f = Image.open(request.files["file"])
            if not query_db("select * from images where title = ? and artist = ?",
                    [request.form["title"], request.form["artist"]]):

                # Use pillow to save as quality 80 JPEG, maybe resize images that
                # are extremely large?
                f = Image.open(f)
                filename = os.path.join(app.config["UPLOAD_FOLDER"],
                        datetime.datetime.now().strftime("%Y%m%dT%H%M%SZ") + ".jpg")
                f.save(filename, "jpeg", quality=80)
                db = get_db()
                db.execute("insert into images (title, artist, work_type, has_nudity, filename) values (?, ?, ?, ?, ?)",
                        [request.form["title"], request.form["artist"], request.form["work_type"],
                            request.form.get("hasnudity", False), filename])
                db.commit()
            return redirect(url_for("upload_image"))
        except:
            return "Bad image file"
    return render_template("upload_page.html")

if __name__ == "__main__":
    app.run(debug=True)

