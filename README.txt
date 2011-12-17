Dietrich's Game
===============

This started out as a "Spectre" clone (the 1991 video game) but it
turned into a platform for my own personal game development.  It has
some rudimentary UI classes, texture loading, text rendering, and a
bin of ill-advised platform code.  I also dumped an incomplete port of
a game I made around 1999; both this game and the Spectre clone can be
selected from the main menu.

Much of the platform-specific code reimplements the kind of
functionality found in LibSDL.  One of the goals of this project is to
provide functionality close to that of a native application, which is
beyond SDL's capabilities.  The trade-off is that this way requires
more work.

Requirements
------------

Linux:
* Gtk+ 2.0
* GtkGLExt
* Cairo
* Pango (with Cairo backend)
* OpenGL
* LibPNG
* C++ compiler
* Evdev (no other keyboard input is supported, sorry)

Mac OS X:
* Mac OS X 10.5 (uses the Core Text API)

Known Limitations
-----------------

The GTK version assumes that the user is using evdev exclusively.
Evdev is Linux-specific, and older Linux distributions do not
necessarily use evdev at all.  If you use X11 but not evdev, the
default keyboard configuration will be messed up and saved
configurations will not be portable to other systems.

Mac OS X Notes
--------------

If building, go to the projects Executables > Game > Info, and under
the "Arguments" tab add the following arguments:

    -i
    $(PROJECT_DIR)/Data

This doesn't get checked into Git since it's part of the user
settings.
