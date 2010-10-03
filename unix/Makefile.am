AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects nostdinc

bin_PROGRAMS = game

AM_LDFLAGS = @SDL_LIBS@ @PANGOCAIRO_LIBS@
AM_CPPFLAGS = @warning_cppflags@ -I@srcdir@/../src @SDL_CFLAGS@ @PANGOCAIRO_CFLAGS@

game_SOURCES = \
	../src/color.hpp \
	../src/explosion.cpp \
	../src/explosion.hpp \
	../src/main.cpp \
	../src/model.cpp \
	../src/model.hpp \
	../src/object.cpp \
	../src/object.hpp \
	../src/obstacle.cpp \
	../src/obstacle.hpp \
	../src/player.cpp \
	../src/player.hpp \
	../src/rand.cpp \
	../src/rand.hpp \
	../src/shot.cpp \
	../src/shot.hpp \
	../src/world.cpp \
	../src/world.hpp \
	../src/type/font.hpp \
	../src/type/font_pango.cpp \
	../src/type/font_pango.hpp \
	../src/type/rastertext.cpp \
	../src/type/rastertext.hpp \
	../src/type/rastertext_cairo.cpp \
	../src/ui/mainmenu.cpp
