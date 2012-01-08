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

GTK_CFLAGS := $(shell pkg-config --cflags gtk+-2.0 gtkglext-1.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-2.0 gtkglext-1.0)
PNG_CFLAGS := $(shell pkg-config --cflags libpng)
PNG_LIBS := $(shell pkg-config --libs libpng)
JPEG_CFLAGS :=
JPEG_LIBS := -ljpeg
GL_LIBS := -lGL -lGLU

-include ../config.mak

########################################

ALL_CPPFLAGS = $(BASE_CPPFLAGS) $(CPPFLAGS)
ALL_CFLAGS = $(BASE_CFLAGS) $(CWARN) $(CFLAGS)
ALL_CXXFLAGS = $(BASE_CXXFLAGS) $(CXXWARN) $(CXXFLAGS)
ALL_LDFLAGS = $(LDFLAGS)
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
GTK_OBJS += gtk/main.o
$(GTK_OBJS): BASE_CFLAGS += $(GTK_CFLAGS)

MAC_OBJS =
MAC_OBJS += mac/GApplication.o
MAC_OBJS += mac/GController.o
MAC_OBJS += mac/GDisplay.o
MAC_OBJS += mac/GMain.o
MAC_OBJS += mac/GView.o
MAC_OBJS += mac/GWindow.o

IMPL_OBJS =
IMPL_OBJS += impl/configfile.o
IMPL_OBJS += impl/configfileread.o
IMPL_OBJS += impl/clock.o
IMPL_OBJS += impl/cvar.o
IMPL_OBJS += impl/dict.o
IMPL_OBJS += impl/error.o
IMPL_OBJS += impl/hash.o
IMPL_OBJS += impl/kbd/keyid.o
IMPL_OBJS += impl/kbd/keytable_evdev.o
IMPL_OBJS += impl/lfile.o
IMPL_OBJS += impl/lpath.o
IMPL_OBJS += impl/model.o
IMPL_OBJS += impl/opengl.o
IMPL_OBJS += impl/pixbuf.o
IMPL_OBJS += impl/pixbuf_loadimage.o
IMPL_OBJS += impl/rand.o
IMPL_OBJS += impl/resource.o
IMPL_OBJS += impl/strbuf.o
IMPL_OBJS += impl/texture.o
impl/pixbuf_loadimage.o: BASE_CFLAGS += $(PNG_CFLAGS) $(JPEG_CFLAGS)
impl/pixbuf_loadimage.o: BASE_CPPFLAGS += -DHAVE_LIBPNG -DHAVE_LIBJPEG

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
CLIENT_OBJS += client/ui/window.o

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

C_OBJS = $(GTK_OBJS) $(IMPL_OBJS)
CXX_OBJS = $(SYS_OBJS) $(CLIENT_OBJS) $(GAME_OBJS)
OBJC_OBJS =

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
	QUIET_OBJC = @echo '   ' CXX $<;
	QUIET_LINK = @echo '   ' LD $@;
endif

########################################

$(C_OBJS): %.o: %.c $(dep_dirs_missing)
	$(QUIET_CC)$(CC) -o $@ -c  $(dep_args) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $<

$(CXX_OBJS): %.o: %.cpp $(dep_dirs_missing)
	$(QUIET_CXX)$(CXX) -o $@ -c $(dep_args) $(ALL_CPPFLAGS) $(ALL_CXXFLAGS) $<

$(OBJC_OBJS): %.o: %.m $(dep_dirs_missing)
	$(QUIET_OBJC)$(CC) -o $@ -c $(dep_args) $(ALL_CPPFLAGS) $(ALL_OBJCFLAGS) $<

game-bin: BASE_LIBS += $(GTK_LIBS)
game-bin: BASE_LIBS += $(GL_LIBS) $(PNG_LIBS) $(JPEG_LIBS)

game-bin: $(ALL_OBJS)
	$(QUIET_LINK)$(CXX) -o $@ $(ALL_OBJS) $(ALL_LIBS)

########################################

all: game-bin

clean:
	$(RM) game-bin $(ALL_OBJS)
	$(RM) -r $(dep_dirs)

.PHONY: all clean
