Lonely Star
===========
Dietrich Epp <depp@moria.us>
http://moria.us/ludumdare/ld22/

There's nothing like some quality time to yourself.  Get rid of
everyone else so you can be alone.

A Ludum Dare #22 entry, for the "Alone" theme.  There are six levels.
I have a hard time beating a couple of them.

Controls
--------

Movement: Arrow keys / WASD / Keypad 8456
Throw item: Space bar / Keypad 0
Restart level: F5
Skip level (cheat): F8

Operating System Support
------------------------

Windows: Requires MSVCP100.dll, whatever that means.  It runs on my
computer which has Microsoft Visual Studio 2010 Express installed.

Mac OS X: Requires OS X 10.5 or higher, PowerPC or Intel processor.

Linux: Building from source requires:

* Gtk+ 2.0
* GtkGLExt
* Cairo (see note)
* Pango (see note)
* OpenGL
* LibPNG
* C++ compiler

Linux doesn't actually need Cairo or Pango, you can remove those from
the build system without ill effects.

Linux / Mac OS X versions take a '-i' parameter to specify alternate
data directory, the Windows version takes a '/I' parameter.  This is
intended for use when running from an IDE or build tree, so you don't
have to copy files around.

Level Editor
------------

It probably only works on Linux, it sure doesn't work on Windows.
Start Linux version with '-e' command line switch.  Controls:

1: brush tool
    left click: paint
    right click: erase
    pgup/pgdn/home/end: select brush
    (note many tiles are invisible but solid)

2: entity tool
    left click: select / move
    right click: create
    pgup/pgdn/home/end: select entity type
    backspace/delete: delete selected entity

3: background tool
    pgup/pgdn/home/end: change background

F1: save current level
F5: load previous level (loses changes!)
F6: load next level (loses changes!)
