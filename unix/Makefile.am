AUTOMAKE_OPTIONS = 1.5 foreign subdir-objects

bin_PROGRAMS = game

AM_LDFLAGS = @SDL_LIBS@ @PANGOCAIRO_LIBS@
AM_CPPFLAGS = @warning_cppflags@ @SDL_CFLAGS@ @PANGOCAIRO_CFLAGS@

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
	../src/type/type.cpp \
	../src/type/type.hpp \
	../src/type/type_cairo.cpp
