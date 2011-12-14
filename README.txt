Known Limitations
-----------------

The GTK version assumes that the user is using evdev exclusively.
Evdev is Linux-specific, and older Linux distributions do not
necessarily use evdev at all.  If you use X11 but not evdev, the
default keyboard configuration will be messed up and saved
configurations will not be portable to other systems.
