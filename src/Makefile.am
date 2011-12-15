AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects nostdinc

bin_PROGRAMS = game-bin
noinst_LIBRARIES = libmsys.a libmgfx.a libmtype.a
noinst_SCRIPTS = config.ini

config.ini : config.ini.in Makefile
	sed \
		-e 's,@''builddir@,@abs_top_builddir@,g' \
		-e 's,@''srcdir@,@abs_top_srcdir@,g' \
		$(srcdir)/config.ini.in > $@.tmp
	mv $@.tmp $@


common_CFLAGS = @warning_cppflags@ -I@srcdir@ -I@builddir@

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
	sys/clock_osx.cpp \
	type/font_osx.cpp \
	type/font_osx.hpp \
	type/rastertext_osx.cpp \
	ui/keyboard/keytable_mac.c

other_sources = \
	sys/parseconfig.cpp \
	sys/testconfig.cpp

libmsys_a_CFLAGS = $(common_CFLAGS)
libmsys_a_CXXFLAGS = $(common_CFLAGS)
libmsys_a_SOURCES = \
	opengl.hpp \
	rand.cpp \
	rand.hpp \
	sys/autocf_osx.hpp \
	sys/buffer.hpp \
	sys/clock.hpp \
	sys/clock_posix.cpp \
	sys/config.cpp \
	sys/config.hpp \
	sys/configfile.cpp \
	sys/configfile.hpp \
	sys/configfileread.rl \
	sys/ifile.cpp \
	sys/ifile.hpp \
	sys/ifilereg.cpp \
	sys/ifilereg.hpp \
	sys/path.cpp \
	sys/path.hpp \
	sys/path_linux.cpp \
	sys/path_xdg.cpp \
	sys/resource.cpp \
	sys/resource.hpp \
	sys/sharedref.hpp \
	sys/stringarray.cpp \
	sys/stringarray.hpp \
	sys/system_error.cpp \
	sys/system_error.hpp

libmgfx_a_CFLAGS = $(common_CFLAGS)
libmgfx_a_CXXFLAGS = $(common_CFLAGS)
libmgfx_a_SOURCES = \
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
	game/space/world.hpp \
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
	graphics/color.hpp \
	graphics/dummytexture.cpp \
	graphics/dummytexture.hpp \
	graphics/model.cpp \
	graphics/model.hpp \
	graphics/texture.cpp \
	graphics/texture.hpp \
	graphics/texturefile.cpp \
	graphics/texturefile.hpp \
	scene/group.cpp \
	scene/group.hpp \
	scene/leafobject.cpp \
	scene/leafobject.hpp \
	scene/object.cpp \
	scene/object.hpp \
	ui/action.hpp \
	ui/button.cpp \
	ui/button.hpp \
	ui/event.hpp \
	ui/geometry.hpp \
	ui/keyboard/keycode.h \
	ui/keyboard/keyid.c \
	ui/keyboard/keyid.h \
	ui/keyboard/keytable.h \
	ui/keyboard/keytable_evdev.c \
	ui/menu.cpp \
	ui/menu.hpp \
	ui/mousemanager.cpp \
	ui/mousemanager.hpp \
	ui/screen.cpp \
	ui/screen.hpp \
	ui/widget.cpp \
	ui/widget.hpp \
	ui/window.cpp \
	ui/window.hpp

libmtype_a_CFLAGS = $(common_CFLAGS) @PANGOCAIRO_CFLAGS@
libmtype_a_CXXFLAGS = $(common_CFLAGS) @PANGOCAIRO_CFLAGS@
libmtype_a_SOURCES = \
	type/font.hpp \
	type/font_pango.cpp \
	type/font_pango.hpp \
	type/rastertext.cpp \
	type/rastertext.hpp \
	type/rastertext_cairo.cpp

game_bin_CXXFLAGS = $(common_CFLAGS) @GTK_CFLAGS@ @GTKGLEXT_CFLAGS@
game_bin_LDFLAGS = @GTK_LIBS@ @GTKGLEXT_LIBS@ @PANGOCAIRO_LIBS@
game_bin_LDADD = libmgfx.a libmtype.a libmsys.a
game_bin_SOURCES = \
	gtk/main.cpp

SUFFIXES = .rl

.rl.cpp:
	$(RAGEL) $(RAGELFLAGS) -C $< -o $@
