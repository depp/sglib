Dietrich's Game
===============

This is a small collection of mostly incomplete games, some not even
prototypes.  It started out as a set of separate games, one even
dating back to 1999 or so.  All of the separate games were integrated
into the same tree so they could share code more effectively.

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
* Pango (with Cairo backend)
* OpenGL
* LibPNG
* LibJPEG
* C++ compiler
* Evdev (no other keyboard input is supported, sorry)

Mac OS X:
* Mac OS X 10.5 or more recent (uses the Core Text API)
* Universal, tested on PowerPC and Intel processors

Windows:
* Vista or more recent (for WinCodec)

Building
--------

On Linux, use the standard build procedure.  The configuration step is
optional but recommended.  The makefile will generate a "run.sh"
script which sets up the data paths correctly.

    $ ./configure
    $ make -j4
    $ ./run.sh

On Mac OS X, use the provided Xcode projects.  They target Xcode 3.1
and Mac OS X 10.5.

On Windows, use the provided Visual Studio projects.  They were made
for Microsoft Visual Studio Express 2010.

Known Limitations
-----------------

The GTK version assumes that the user is using evdev exclusively.
Evdev is Linux-specific, and older Linux distributions do not
necessarily use evdev at all.  If you use X11 but not evdev, the
default keyboard configuration will be messed up and saved
configurations will not be portable to other systems.
