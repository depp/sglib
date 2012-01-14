all:

UNAME_S := $(shell sh -c 'uname -s 2>/dev/null || echo unknown')
UNAME_M := $(shell sh -c 'uname -m 2>/dev/null || echo unknown')

CC = gcc
CXX = g++
RM = rm -f

CPPFLAGS =
CFLAGS = -g -O2
CXXFLAGS = -g -O2
CWARN = -Wall -Wextra -Wpointer-arith -Wno-sign-compare \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes
CXXWARN = -Wall -Wextra -Wpointer-arith -Wno-sign-compare
LDFLAGS =
LIBS =

ifeq ($(UNAME_S),Linux)
	HAVE_GTK := $(shell sh -c 'pkg-config gtk+-2.0 gtkglext-1.0 2>/dev/null && echo yes')
	ifdef HAVE_GTK
		GTK_CFLAGS := $(shell pkg-config --cflags gtk+-2.0 gtkglext-1.0)
		GTK_LIBS := $(shell pkg-config --libs gtk+-2.0 gtkglext-1.0)
		GTK_LIBS += -Wl,--no-export-dynamic # Turned on by gmodule
	endif
	HAVE_LIBPNG := $(shell sh -c 'pkg-config libpng 2>/dev/null && echo yes')
	ifdef HAVE_LIBPNG
		PNG_CFLAGS := -DHAVE_LIBPNG $(shell pkg-config --cflags libpng)
		PNG_LIBS := $(shell pkg-config --libs libpng)
	endif
	HAVE_LIBJPEG := yes
	JPEG_CFLAGS := -DHAVE_LIBJPEG
	JPEG_LIBS := -ljpeg
	IMAGE_CFLAGS := $(PNG_CFLAGS) $(JPEG_CFLAGS)
	IMAGE_LIBS := $(PNG_LIBS) $(JPEG_LIBS)
	HAVE_PANGO := $(shell sh -c 'pkg-config pangocairo 2>/dev/null && echo yes')
	ifdef HAVE_PANGO
		PANGO_CFLAGS := -DUSE_PANGOCAIRO $(shell pkg-config --cflags pangocairo)
		PANGO_LIBS := $(shell pkg-config --libs pangocairo)
	endif
	TYPE_CFLAGS = $(PANGO_CFLAGS)
	TYPE_LIBS = $(PANGO_LIBS)
	GL_LIBS := -lGL -lGLU
	BASE_LDFLAGS := --Wl,--as-needed
endif
ifeq ($(UNAME_S),Darwin)
	HAVE_COCOA := yes
	IMAGE_CFLAGS := -DHAVE_COREGRAPHICS -F/System/Library/Frameworks/ApplicationServices.framework/Frameworks
	IMAGE_LIBS := -framework ApplicationServices
	GL_LIBS := -framework OpenGL
	COCOA_LIBS := -framework AppKit -framework Foundation -framework CoreVideo -framework Carbon
endif

-include ../config.mak

########################################

ALL_CPPFLAGS = $(BASE_CPPFLAGS) $(CPPFLAGS)
ALL_CFLAGS = $(BASE_CFLAGS) $(CWARN) $(CFLAGS)
ALL_CXXFLAGS = $(BASE_CXXFLAGS) $(CXXWARN) $(CXXFLAGS)
ALL_LDFLAGS = $(BASE_LDFLAGS) $(LDFLAGS)
ALL_LIBS = $(BASE_LIBS) $(LIBS)

BASE_CPPFLAGS = -I.
BASE_CFLAGS =
BASE_CXXFLAGS =
BASE_LIBS =

########################################

dep_check = $(shell $(CC) $(ALL_CFLAGS) \
	-c -MF /dev/null -MMD -MP -x c /dev/null -o /dev/null 2>&1; \
	echo $$?)

ifeq ($(dep_check),0)
COMPUTE_DEPS = yes
else
COMPUTE_DEPS = no
endif

########################################

GTK_OBJS =
GTK_OBJS += platform/gtk.o
$(GTK_OBJS): BASE_CFLAGS += $(GTK_CFLAGS)

MAC_OBJS =
MAC_OBJS += platform/macosx/GApplication.o
MAC_OBJS += platform/macosx/GController.o
MAC_OBJS += platform/macosx/GDisplay.o
MAC_OBJS += platform/macosx/GMain.o
MAC_OBJS += platform/macosx/GView.o
MAC_OBJS += platform/macosx/GWindow.o

CBASE_OBJS =
CBASE_OBJS += base/configfile.o
CBASE_OBJS += base/configfile_read.o
CBASE_OBJS += base/clock.o
CBASE_OBJS += base/cvar.o
CBASE_OBJS += base/dict.o
CBASE_OBJS += base/error.o
CBASE_OBJS += base/hash.o
CBASE_OBJS += base/keycode/keyid.o
CBASE_OBJS += base/keycode/keytable_evdev.o
CBASE_OBJS += base/keycode/keytable_mac.o
CBASE_OBJS += base/keycode/keytable_win.o
CBASE_OBJS += base/file.o
CBASE_OBJS += base/file_path.o
CBASE_OBJS += base/model.o
CBASE_OBJS += base/opengl.o
CBASE_OBJS += base/pixbuf.o
CBASE_OBJS += base/pixbuf_loadimage.o
CBASE_OBJS += base/rand.o
CBASE_OBJS += base/resource.o
CBASE_OBJS += base/strbuf.o
CBASE_OBJS += base/sys.o
CBASE_OBJS += base/texture.o
CBASE_OBJS += base/type.o
CBASE_OBJS += version.o
base/pixbuf_loadimage.o: BASE_CFLAGS += $(IMAGE_CFLAGS)
base/type.o: BASE_CFLAGS += $(TYPE_CFLAGS)

SYS_OBJS =
SYS_OBJS += sys/error.o
SYS_OBJS += sys/file.o
SYS_OBJS += sys/stringarray.o
SYS_OBJS += sys/system_error.o

CLIENT_OBJS =
CLIENT_OBJS += client/bitmapfont.o
CLIENT_OBJS += client/letterbox.o
CLIENT_OBJS += client/model.o
CLIENT_OBJS += client/scene/group.o
CLIENT_OBJS += client/scene/leafobject.o
CLIENT_OBJS += client/scene/object.o
CLIENT_OBJS += client/texture.o
CLIENT_OBJS += client/ui/button.o
CLIENT_OBJS += client/ui/keymanager.o
CLIENT_OBJS += client/ui/menu.o
CLIENT_OBJS += client/ui/mousemanager.o
CLIENT_OBJS += client/ui/screen.o
CLIENT_OBJS += client/ui/widget.o
CLIENT_OBJS += client/viewport.o

GAME_LD22_OBJS =
GAME_LD22_OBJS += game/ld22/actor.o
GAME_LD22_OBJS += game/ld22/area.o
GAME_LD22_OBJS += game/ld22/background.o
GAME_LD22_OBJS += game/ld22/editor.o
GAME_LD22_OBJS += game/ld22/effect.o
GAME_LD22_OBJS += game/ld22/endwalk.o
GAME_LD22_OBJS += game/ld22/item.o
GAME_LD22_OBJS += game/ld22/level.o
GAME_LD22_OBJS += game/ld22/mover.o
GAME_LD22_OBJS += game/ld22/other.o
GAME_LD22_OBJS += game/ld22/player.o
GAME_LD22_OBJS += game/ld22/screen.o
GAME_LD22_OBJS += game/ld22/screenbase.o
GAME_LD22_OBJS += game/ld22/thinker.o
GAME_LD22_OBJS += game/ld22/tileset.o
GAME_LD22_OBJS += game/ld22/walker.o

GAME_TANK_OBJS =
GAME_TANK_OBJS += game/tank/explosion.o
GAME_TANK_OBJS += game/tank/gamescreen.o
GAME_TANK_OBJS += game/tank/object.o
GAME_TANK_OBJS += game/tank/obstacle.o
GAME_TANK_OBJS += game/tank/player.o
GAME_TANK_OBJS += game/tank/shot.o
GAME_TANK_OBJS += game/tank/world.o

GAME_SPACE_OBJS =
GAME_SPACE_OBJS += game/space/entity.o
GAME_SPACE_OBJS += game/space/gamescreen.o
GAME_SPACE_OBJS += game/space/main.o
GAME_SPACE_OBJS += game/space/player.o
GAME_SPACE_OBJS += game/space/random.o
GAME_SPACE_OBJS += game/space/scenery.o
GAME_SPACE_OBJS += game/space/shapes.o
GAME_SPACE_OBJS += game/space/ship.o
GAME_SPACE_OBJS += game/space/shot.o
GAME_SPACE_OBJS += game/space/stars.o
GAME_SPACE_OBJS += game/space/thinker.o
GAME_SPACE_OBJS += game/space/world.o

GAME_OBJS = $(GAME_LD22_OBJS) $(GAME_TANK_OBJS) $(GAME_SPACE_OBJS)

C_OBJS = $(GTK_OBJS) $(CBASE_OBJS)
CXX_OBJS = $(SYS_OBJS) $(CLIENT_OBJS) $(GAME_OBJS)
OBJC_OBJS = $(MAC_OBJS)

BASE_OBJS = $(CBASE_OBJS) $(CXX_OBJS)
ALL_OBJS = $(C_OBJS) $(CXX_OBJS) $(OBJC_OBJS)

########################################

ifeq ($(COMPUTE_DEPS),yes)
dep_files := $(foreach f,$(ALL_OBJS),$(dir $f).deps/$(notdir $f).d)
dep_dirs := $(addsuffix .deps,$(sort $(dir $(ALL_OBJS))))
$(dep_dirs):
	@mkdir -p $@
dep_dirs_missing := $(filter-out $(wildcard $(dep_dirs)), $(dep_dirs))
dep_files_present := $(wildcard $(dep_files))
ifneq ($(dep_files_present),)
include $(dep_files_present)
endif

dep_file = $(dir $@).deps/$(notdir $@).d
dep_args = -MF $(dep_file) -MMD -MP
else
dep_dirs_missing =
dep_args =
endif

########################################

ifndef V
	QUIET_CC   = @echo '   ' CC $<;
	QUIET_CXX  = @echo '   ' CXX $<;
	QUIET_OBJC = @echo '   ' OBJC $<;
	QUIET_LINK = @echo '   ' LD $@;
	QUIET_VERS = @echo '   ' VERSION;
endif

########################################

version.c: version-stamp
version-stamp:
	$(QUIET_VERS)sh ../scripts/version.sh .. ..

$(C_OBJS): %.o: %.c $(dep_dirs_missing)
	$(QUIET_CC)$(CC) -o $@ -c  $(dep_args) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $<

$(CXX_OBJS): %.o: %.cpp $(dep_dirs_missing)
	$(QUIET_CXX)$(CXX) -o $@ -c $(dep_args) $(ALL_CPPFLAGS) $(ALL_CXXFLAGS) $<

$(OBJC_OBJS): %.o: %.m $(dep_dirs_missing)
	$(QUIET_OBJC)$(CC) -o $@ -c $(dep_args) $(ALL_CPPFLAGS) $(ALL_OBJCFLAGS) $<

ifdef HAVE_GTK
targets += game-bin

game-bin: BASE_LIBS += $(GTK_LIBS) $(GL_LIBS) $(IMAGE_LIBS) $(TYPE_LIBS)

game-bin: $(BASE_OBJS) $(GTK_OBJS)
	$(QUIET_LINK)$(CXX) -o $@ $^ $(ALL_LDFLAGS) $(ALL_LIBS)
endif

ifdef HAVE_COCOA
targets += game-mac

game-mac: BASE_LIBS += $(GL_LIBS) $(COCOA_LIBS)

game-mac: $(BASE_OBJS) $(MAC_OBJS)
	$(QUIET_LINK)$(CXX) -o $@ $^ $(ALL_LDFLAGS) $(ALL_LIBS)
endif

########################################

all: $(targets)

clean:
	$(RM) $(targets) $(ALL_OBJS)
	$(RM) -r $(dep_dirs)

.PHONY: all clean
