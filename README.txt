SGLib
=====

This is a library for building games, with a Python-based build system
that will produce executables for Linux, Mac OS X, and Windows; as
well as IDE projects for Xcode and Visual Studio.

Much of the platform-specific code reimplements the kind of
functionality found in LibSDL.  One of the goals of this project is to
provide functionality close to that of a native application, which is
beyond SDL's capabilities.  The trade-off is that this way requires
more work.

This also provides some useful systems such as rendering text to
OpenGL textures and loading PNGs and JPEGs.

Requirements
------------

Linux:
* Gtk+ 2.0
* GtkGLExt
* Pango (with Cairo backend)
* OpenGL
* LibPNG
* LibJPEG
* C++ compiler (actually optional)
* Evdev (no other keyboard input is supported, sorry)

Mac OS X:
* Mac OS X 10.5 or more recent
* Universal, tested on PowerPC and Intel processors

Windows:
* Windows XP SP3 or more recent

More information
----------------

This repository does not include examples.  It does, however, include
documentation in the 'doc' folder.

Known limitations
-----------------

The GTK version assumes that the user is using evdev exclusively.
Evdev is Linux-specific, and older Linux distributions do not
necessarily use evdev at all.  If you use X11 but not evdev, the
default keyboard configuration will be messed up and saved
configurations will not be portable to other systems.
