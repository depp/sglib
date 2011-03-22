AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects nostdinc
ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = game

AM_LDFLAGS = @SDL_LIBS@ @PANGOCAIRO_LIBS@
AM_CPPFLAGS = @warning_cppflags@ -I@srcdir@/../src @SDL_CFLAGS@ @PANGOCAIRO_CFLAGS@

TESTS = testconfig

check_PROGRAMS = testconfig

noinst_SCRIPTS = config.ini
noinst_PROGRAMS = parseconfig

parseconfig_SOURCES = \
	../src/sys/parseconfig.cpp \
	../src/sys/configfile.cpp \
	../src/sys/configfile.hpp \
	../src/sys/configfileread.cpp \
	../src/sys/stringarray.cpp \
	../src/sys/stringarray.hpp

config.ini : config.ini.in Makefile
	sed \
		-e 's,@''builddir@,@abs_top_builddir@,g' \
		-e 's,@''srcdir@,@abs_top_srcdir@,g' \
		$(srcdir)/config.ini.in > $@.tmp
	mv $@.tmp $@

testconfig_SOURCES = \
	../src/sys/testconfig.cpp \
	../src/sys/configfile.cpp \
	../src/sys/configfile.hpp \
	../src/sys/stringarray.cpp \
	../src/sys/stringarray.hpp

game_SOURCES = \
	../src/sys/buffer.hpp \
	../src/sys/config.cpp \
	../src/sys/config.hpp \
	../src/sys/configfile.cpp \
	../src/sys/configfile.hpp \
	../src/sys/configfileread.cpp \
	../src/sys/ifile.cpp \
	../src/sys/ifile.hpp \
	../src/sys/ifilereg.cpp \
	../src/sys/ifilereg.hpp \
	../src/sys/path.cpp \
	../src/sys/path.hpp \
	../src/sys/path_linux.cpp \
	../src/sys/path_xdg.cpp \
	../src/sys/resource.cpp \
	../src/sys/resource.hpp \
	../src/sys/sharedref.hpp \
	../src/sys/stringarray.cpp \
	../src/sys/stringarray.hpp \
	../src/sys/system_error.cpp \
	../src/sys/system_error.hpp \
	../src/game/explosion.cpp \
	../src/game/explosion.hpp \
	../src/game/object.cpp \
	../src/game/object.hpp \
	../src/game/obstacle.cpp \
	../src/game/obstacle.hpp \
	../src/game/player.cpp \
	../src/game/player.hpp \
	../src/game/shot.cpp \
	../src/game/shot.hpp \
	../src/game/world.cpp \
	../src/game/world.hpp \
	../src/graphics/color.hpp \
	../src/graphics/dummytexture.cpp \
	../src/graphics/dummytexture.hpp \
	../src/graphics/model.cpp \
	../src/graphics/model.hpp \
	../src/graphics/texture.cpp \
	../src/graphics/texture.hpp \
	../src/graphics/texturefile.cpp \
	../src/graphics/texturefile.hpp \
	../src/graphics/video.cpp \
	../src/graphics/video.hpp \
	../src/main.cpp \
	../src/rand.cpp \
	../src/rand.hpp \
	../src/scene/group.cpp \
	../src/scene/group.hpp \
	../src/scene/leafobject.cpp \
	../src/scene/leafobject.hpp \
	../src/scene/object.cpp \
	../src/scene/object.hpp \
	../src/type/font.hpp \
	../src/type/font_pango.cpp \
	../src/type/font_pango.hpp \
	../src/type/rastertext_cairo.cpp \
	../src/type/rastertext.cpp \
	../src/type/rastertext.hpp \
	../src/ui/action.hpp \
	../src/ui/button.cpp \
	../src/ui/button.hpp \
	../src/ui/event.hpp \
	../src/ui/game.cpp \
	../src/ui/game.hpp \
	../src/ui/geometry.hpp \
	../src/ui/menu.cpp \
	../src/ui/menu.hpp \
	../src/ui/mousemanager.cpp \
	../src/ui/mousemanager.hpp \
	../src/ui/screen.cpp \
	../src/ui/screen.hpp \
	../src/ui/widget.cpp \
	../src/ui/widget.hpp

#	../src/type/rastertext_osx.cpp

SUFFIXES = .rl

.rl.cpp:
	$(RAGEL) $(RAGELFLAGS) -C $< -o $@
