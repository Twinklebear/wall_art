#!/bin/python

#from PIL import Image, ImageFilter

#with Image.open("forum.jpg") as img:
#    print(img.size)
#    blurred = img.filter(ImageFilter.GaussianBlur(96))
#    blurred.save("blurred.jpg", "JPEG")

import sqlite3
import os
from flask import Flask, jsonify, send_file, g, request, redirect, abort
from werkzeug.utils import secure_filename

DATABASE = "database.db"
UPLOAD_FOLDER = "./images/"

app = Flask(__name__)
app.config.from_object(__name__)

@app.route("/")
def index():
    return "Wall Art"

images = [
    {
        "id": 1,
        "name": "Forum"
    }
]

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

@app.route("/api/image", methods=["GET"])
def get_images():
    return jsonify({ "images": images })

@app.route("/api/image/<int:image_id>", methods=["GET"])
def get_image(image_id):
    # Note: This assumes the image id is valid
    img = query_db("select * from entries where id = ?", [image_id], one=True)
    if img is None:
        abort(404)
    return send_file(img[2], mimetype="image/jpeg")

@app.route("/upload", methods=["GET", "POST"])
def upload_image():
    if request.method == "POST":
        f = request.files["file"]
        if f:
            filename = os.path.join(app.config["UPLOAD_FOLDER"], secure_filename(f.filename))
            f.save(filename)
            if query_db("select * from entries where title = ?", [request.form["title"]], one=True) is None:
                db = get_db()
                print("Will insert new image")
                db.execute("insert into entries (title, filename) values (?, ?)", [request.form["title"], filename])
                db.commit()
        return redirect("/upload")

    return """
    <!DOCTYPE html5>
    <title>Upload image</title>
    </h1>Upload File</h1>
    <form action="" method=post enctype=multipart/form-data>
        <p>
           <input type=file name=file>
           <label>Title
           <input type=text name=title>
           </label>
           <input type=submit value=Upload>
        </p>
    </form>
    """

if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)

