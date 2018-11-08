wall\_art
===

**Warning:** This project is in very early development and doesn't actually do much useful yet.

Generates [Artful Mac](http://artfulmac.com/) style wall papers from images. This is
done by cropping the image to match the aspect ratio of the user's desktop,
and letting the window manager scale it to fit.

# Client

The client is written in C++ and requires [Qt5](http://www.qt.io/). It can then
be built with CMake. The client assumes the server is running at localhost.

# Server

The server is written in Python and requires [Flask](http://flask.pocoo.org/),
[sqlite](https://www.sqlite.org/) is used to store the image meta-data and
file locations on the server. First you'll need to make the database for the server
with sqlite, and make the images directory

```
cd server
sqlite3 database.db < schema.sql
mkdir images
```

Then you can run the server with python, which will host at localhost:5000.
To add images to the database go to `localhost:5000/upload` and upload
some images. The images will be saved under `server/images/`

## API Endpoints

The API endpoints returning data will return JSON, the image retrieval
endpoints return JPGs.

- `/api/image`: Get a list of all images on the server
- `/api/image/<int>`: Get information about a specific image based on its ID
- `/api/image/<int>/original`: Download the JPG of the image with the ID

