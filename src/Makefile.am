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
	sys/autocf_osx.hpp \
	sys/buffer.hpp \
	sys/clock.hpp \
	sys/clock.cpp \
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
libmgfx_a_CXXFLAGS = $(common_CFLAGS)
libmgfx_a_SOURCES = \
	client/color.hpp \
	client/dummytexture.cpp \
	client/dummytexture.hpp \
	client/keyboard/keycode.h \
	client/keyboard/keyid.c \
	client/keyboard/keyid.h \
	client/keyboard/keytable.h \
	client/keyboard/keytable_evdev.c \
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
	game/ld22/defs.hpp \
	game/ld22/player.cpp \
	game/ld22/player.hpp \
	game/ld22/screen.cpp \
	game/ld22/screen.hpp \
	game/ld22/walker.cpp \
	game/ld22/walker.hpp

libmtype_a_CFLAGS = $(common_CFLAGS) @PANGOCAIRO_CFLAGS@
libmtype_a_CXXFLAGS = $(common_CFLAGS) @PANGOCAIRO_CFLAGS@
libmtype_a_SOURCES = \
	client/type/font.hpp \
	client/type/font_pango.cpp \
	client/type/font_pango.hpp \
	client/type/rastertext.cpp \
	client/type/rastertext.hpp \
	client/type/rastertext_cairo.cpp

game_bin_CXXFLAGS = $(common_CFLAGS) @GTK_CFLAGS@ @GTKGLEXT_CFLAGS@
game_bin_LDFLAGS = @GTK_LIBS@ @GTKGLEXT_LIBS@ @PANGOCAIRO_LIBS@
game_bin_LDADD = libmgfx.a libmtype.a libmsys.a
game_bin_SOURCES = \
	gtk/main.cpp

SUFFIXES = .rl

.rl.cpp:
	$(RAGEL) $(RAGELFLAGS) -C $< -o $@
