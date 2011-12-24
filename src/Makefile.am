AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects nostdinc

bin_PROGRAMS = game-bin
noinst_LIBRARIES = libmimp.a libmsys.a libmgfx.a libmtype.a

common_CXXFLAGS = @CXXWARN@ -I@srcdir@ -I@builddir@
common_CFLAGS = @CWARN@ -I@srcdir@ -I@builddir@

mac_sources = \
	mac/GApplication.h \
	mac/GApplication.m \
	mac/GController.h \
	mac/GController.mm \
	mac/GDisplay.h \
	mac/GDisplay.mm \
	mac/GMain.m \
	mac/GPrefix.h \
	mac/GView.h \
	mac/GView.mm \
	mac/GWindow.h \
	mac/GWindow.m \
	sys/path_osx.cpp \
	sdl/main.cpp \
	type/font_osx.cpp \
	type/font_osx.hpp \
	type/rastertext_osx.cpp \
	ui/keyboard/keytable_mac.c

other_sources = \
	sys/parseconfig.cpp \
	sys/testconfig.cpp

libmimp_a_CFLAGS = $(common_CFLAGS)
libmimp_a_SOURCES = \
	impl/configfile.c \
	impl/configfile.h \
	impl/configfileread.c \
	impl/dict.c \
	impl/dict.h \
	impl/error.c \
	impl/error.h \
	impl/hash.c \
	impl/hash.h \
	impl/lfile.c \
	impl/lfile.h \
	impl/lpath.c

libmsys_a_CFLAGS = $(common_CFLAGS)
libmsys_a_CXXFLAGS = $(common_CXXFLAGS)
libmsys_a_SOURCES = \
	sys/autocf_osx.hpp \
	sys/buffer.hpp \
	sys/clock.hpp \
	sys/clock.cpp \
	sys/error.cpp \
	sys/error.hpp \
	sys/file.cpp \
	sys/file.hpp \
	sys/rand.cpp \
	sys/rand.hpp \
	sys/resource.cpp \
	sys/resource.hpp \
	sys/sharedref.hpp \
	sys/stringarray.cpp \
	sys/stringarray.hpp \
	sys/system_error.cpp \
	sys/system_error.hpp

libmgfx_a_CFLAGS = $(common_CFLAGS)
libmgfx_a_CXXFLAGS = $(common_CXXFLAGS)
libmgfx_a_SOURCES = \
	client/bitmapfont.cpp \
	client/bitmapfont.hpp \
	client/color.hpp \
	client/color.hpp \
	client/dummytexture.cpp \
	client/dummytexture.hpp \
	client/keyboard/keycode.h \
	client/keyboard/keyid.c \
	client/keyboard/keyid.h \
	client/keyboard/keytable.h \
	client/keyboard/keytable_evdev.c \
	client/letterbox.cpp \
	client/letterbox.hpp \
	client/model.cpp \
	client/model.hpp \
	client/opengl.hpp \
	client/scene/group.cpp \
	client/scene/group.hpp \
	client/scene/leafobject.cpp \
	client/scene/leafobject.hpp \
	client/scene/object.cpp \
	client/scene/object.hpp \
	client/texture.cpp \
	client/texture.hpp \
	client/texturefile.cpp \
	client/texturefile.hpp \
	client/ui/action.hpp \
	client/ui/button.cpp \
	client/ui/button.hpp \
	client/ui/event.hpp \
	client/ui/geometry.hpp \
	client/ui/keymanager.cpp \
	client/ui/keymanager.hpp \
	client/ui/menu.cpp \
	client/ui/menu.hpp \
	client/ui/mousemanager.cpp \
	client/ui/mousemanager.hpp \
	client/ui/screen.cpp \
	client/ui/screen.hpp \
	client/ui/widget.cpp \
	client/ui/widget.hpp \
	client/ui/window.cpp \
	client/ui/window.hpp \
	game/ld22/actor.cpp \
	game/ld22/actor.hpp \
	game/ld22/area.cpp \
	game/ld22/area.hpp \
	game/ld22/background.cpp \
	game/ld22/background.hpp \
	game/ld22/defs.hpp \
	game/ld22/editor.cpp \
	game/ld22/editor.hpp \
	game/ld22/effect.cpp \
	game/ld22/effect.hpp \
	game/ld22/endwalk.cpp \
	game/ld22/endwalk.hpp \
	game/ld22/item.cpp \
	game/ld22/item.hpp \
	game/ld22/level.cpp \
	game/ld22/level.hpp \
	game/ld22/mover.cpp \
	game/ld22/mover.hpp \
	game/ld22/other.cpp \
	game/ld22/other.hpp \
	game/ld22/player.cpp \
	game/ld22/player.hpp \
	game/ld22/screen.cpp \
	game/ld22/screen.hpp \
	game/ld22/screenbase.cpp \
	game/ld22/screenbase.hpp \
	game/ld22/thinker.cpp \
	game/ld22/thinker.hpp \
	game/ld22/tileset.cpp \
	game/ld22/tileset.hpp \
	game/ld22/walker.cpp \
	game/ld22/walker.hpp \
	game/tank/explosion.cpp \
	game/tank/explosion.hpp \
	game/tank/gamescreen.cpp \
	game/tank/gamescreen.hpp \
	game/tank/object.cpp \
	game/tank/object.hpp \
	game/tank/obstacle.cpp \
	game/tank/obstacle.hpp \
	game/tank/player.cpp \
	game/tank/player.hpp \
	game/tank/shot.cpp \
	game/tank/shot.hpp \
	game/tank/world.cpp \
	game/tank/world.hpp \
	game/space/entity.cpp \
	game/space/entity.hpp \
	game/space/gamescreen.cpp \
	game/space/gamescreen.hpp \
	game/space/main.cpp \
	game/space/player.cpp \
	game/space/player.hpp \
	game/space/random.cpp \
	game/space/random.hpp \
	game/space/scenery.cpp \
	game/space/scenery.hpp \
	game/space/shapes.cpp \
	game/space/shapes.hpp \
	game/space/ship.cpp \
	game/space/ship.hpp \
	game/space/shot.cpp \
	game/space/shot.hpp \
	game/space/stars.cpp \
	game/space/stars.hpp \
	game/space/thinker.cpp \
	game/space/thinker.hpp \
	game/space/vector.hpp \
	game/space/world.cpp \
	game/space/world.hpp

libmtype_a_CFLAGS = $(common_CFLAGS) @PANGOCAIRO_CFLAGS@
libmtype_a_CXXFLAGS = $(common_CXXFLAGS) @PANGOCAIRO_CFLAGS@
libmtype_a_SOURCES = \
	client/type/font.hpp \
	client/type/font_pango.cpp \
	client/type/font_pango.hpp \
	client/type/rastertext.cpp \
	client/type/rastertext.hpp \
	client/type/rastertext_cairo.cpp

game_bin_CFLAGS = $(common_CFLAGS) @GTK_CFLAGS@ @GTKGLEXT_CFLAGS@
game_bin_CXXFLAGS = $(common_CXXFLAGS) @GTK_CFLAGS@ @GTKGLEXT_CFLAGS@
game_bin_LDFLAGS = @GTK_LIBS@ @GTKGLEXT_LIBS@ @PANGOCAIRO_LIBS@
game_bin_LDADD = libmgfx.a libmtype.a libmsys.a libmimp.a
game_bin_SOURCES = \
	gtk/main.cpp
