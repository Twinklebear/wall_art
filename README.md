wall\_art
===

**Warning:** This project is in very early development and doesn't actually do much useful yet.

Generates [Artful Mac](http://artfulmac.com/) style wall papers from images. This is done by fetching the original
image and a blurred version of it and resizing them to fit the user's desktop dimensions. The blurred image
is used as a background and stretched a bit larger and cropped while the original image is scaled down
and centered, letting the blurred copy provide a frame for it.

The resulting wallpaper is something like this:

![Landscape with Classical Ruins and Figures](http://i.imgur.com/TrxDEBB.jpg)

This wallpaper was generated from [Landscape with Classical Ruins and Figures](http://www.getty.edu/art/collection/objects/559/marco-ricci-and-sebastiano-ricci-landscape-with-classical-ruins-and-figures-italian-about-1725-1730/) by Marco Ricci and Sebastiano Ricci.

## Dependencies

### Client

The client is written and C++ and requires [Qt5](http://www.qt.io/).

### Server

The server is written in Python and requires [Flask](http://flask.pocoo.org/), [sqlite](https://www.sqlite.org/) is used
to store the image meta-data and file locations on the server.

