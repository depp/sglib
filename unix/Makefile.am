AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects nostdinc

bin_PROGRAMS = game

AM_LDFLAGS = @SDL_LIBS@ @PANGOCAIRO_LIBS@
AM_CPPFLAGS = @warning_cppflags@ -I@srcdir@/../src @SDL_CFLAGS@ @PANGOCAIRO_CFLAGS@

game_SOURCES = \
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
	../src/graphics/model.cpp \
	../src/graphics/model.hpp \
	../src/graphics/video.cpp \
	../src/graphics/video.hpp \
	../src/main.cpp \
	../src/rand.cpp \
	../src/rand.hpp \
	../src/type/font.hpp \
	../src/type/font_pango.cpp \
	../src/type/font_pango.hpp \
	../src/type/rastertext_cairo.cpp \
	../src/type/rastertext.cpp \
	../src/type/rastertext.hpp \
	../src/ui/action.hpp \
	../src/ui/button.cpp \
	../src/ui/button.hpp \
	../src/ui/container.cpp \
	../src/ui/container.hpp \
	../src/ui/event.hpp \
	../src/ui/menu.cpp \
	../src/ui/menu.hpp \
	../src/ui/rect.hpp \
	../src/ui/screen.cpp \
	../src/ui/screen.hpp \
	../src/ui/widget.cpp \
	../src/ui/widget.hpp

#	../src/type/rastertext_osx.cpp
