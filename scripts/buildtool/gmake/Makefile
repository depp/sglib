all:

-include config.mak

RM = rm -f

########################################
# Platform defaults

# Below are "sensible defaults" based on knowledge of specific
# platforms.  The defaults are ignored if 'configure' is run.
ifndef HAVE_CONFIG

CC = gcc
CXX = g++

CPPFLAGS =
CFLAGS = -g -O2
CXXFLAGS = -g -O2
CWARN = -Wall -Wextra -Wpointer-arith -Wno-sign-compare \
	-Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes
CXXWARN = -Wall -Wextra -Wpointer-arith -Wno-sign-compare
DEFS =
LDFLAGS =
LIBS =

HAVE_GTK := $(shell sh -c 'pkg-config gtk+-2.0 gtkglext-1.0 2>/dev/null && echo yes')
ifdef HAVE_GTK
GTK_CFLAGS := $(shell pkg-config --cflags gtk+-2.0 gtkglext-1.0)
GTK_LIBS := $(shell pkg-config --libs gtk+-2.0 gtkglext-1.0)
GTK_LIBS += -Wl,--no-export-dynamic # Turned on by gmodule
endif

HAVE_LIBPNG := $(shell sh -c 'pkg-config libpng 2>/dev/null && echo yes')
ifdef HAVE_LIBPNG
PNG_CFLAGS := $(shell pkg-config --cflags libpng)
PNG_LIBS := $(shell pkg-config --libs libpng)
endif

HAVE_LIBJPEG := yes
JPEG_CFLAGS :=
JPEG_LIBS := -ljpeg

IMAGE_CFLAGS := $(PNG_CFLAGS) $(JPEG_CFLAGS)
IMAGE_LIBS := $(PNG_LIBS) $(JPEG_LIBS)

HAVE_PANGO := $(shell sh -c 'pkg-config pangocairo 2>/dev/null && echo yes')
ifdef HAVE_PANGO
PANGO_CFLAGS := $(shell pkg-config --cflags pangocairo)
PANGO_LIBS := $(shell pkg-config --libs pangocairo)
endif

TYPE_CFLAGS = $(PANGO_CFLAGS)
TYPE_LIBS = $(PANGO_LIBS)
OPENGL_LIBS := -lGL -lGLU
BASE_LDFLAGS := -Wl,--gc-sections --Wl,--as-needed

endif

########################################
# Variables

ALL_CPPFLAGS = $(DEFS) $(BASE_CPPFLAGS) $(CPPFLAGS)
ALL_CFLAGS = $(BASE_CFLAGS) $(CWARN) $(CFLAGS)
ALL_CXXFLAGS = $(BASE_CXXFLAGS) $(CXXWARN) $(CXXFLAGS)
ALL_LDFLAGS = $(BASE_LDFLAGS) $(LDFLAGS)
ALL_LIBS = $(BASE_LIBS) $(LIBS)

BASE_CPPFLAGS = @CPPFLAGS@
BASE_CFLAGS = @CFLAGS@
BASE_CXXFLAGS = @CXXFLAGS@
BASE_LIBS = @LIBS@

########################################
# Source files

BASE_OBJS = @BASE_OBJS@
GTK_OBJS = @GTK_OBJS@
SDL_OBJS = @SDL_OBJS@
LIBPNG_OBJS = @LIBPNG_OBJS@
LIBJPEG_OBJS = @LIBJPEG_OBJS@
PANGO_OBJS = @PANGO_OBJS@

C_OBJS = @C_OBJS@
CXX_OBJS = @CXX_OBJS@
ALL_OBJS = $(C_OBJS) $(CXX_OBJS)

$(GTK_OBJS): BASE_CFLAGS += $(GTK_CFLAGS)
$(SDL_OBJS): BASE_CFLAGS += $(SDL_CFLAGS)

ifdef HAVE_LIBPNG
$(LIBPNG_OBJS): BASE_CFLAGS += $(LIBPNG_CFLAGS)
BASE_OBJS += $(LIBPNG_OBJS)
endif

ifdef HAVE_LIBJPEG
$(LIBJPEG_OBJS): BASE_CFLAGS += $(LIBJPEG_CFLAGS)
BASE_OBJS += $(LIBJPEG_OBJS)
endif

ifdef HAVE_PANGO
$(PANGO_OBJS): BASE_CFLAGS += $(PANGO_CFLAGS)
BASE_OBJS += $(PANGO_OBJS)
endif

########################################
# Dependency analysis

dep_check = $(shell $(CC) $(ALL_CFLAGS) \
	-c -MF /dev/null -MMD -MP -x c /dev/null -o /dev/null 2>&1; \
	echo $$?)

ifeq ($(dep_check),0)
COMPUTE_DEPS = yes
else
COMPUTE_DEPS = no
endif

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
# Verbose / quiet output

ifndef V
	QUIET_CC   = @echo '   ' CC $<;
	QUIET_CXX  = @echo '   ' CXX $<;
	QUIET_OBJC = @echo '   ' OBJC $<;
	QUIET_LINK = @echo '   ' LD $@;
	QUIET_VERS = @echo '   ' VERSION;
endif

########################################
# Intermediate files

src/base/version_str.c: version-stamp
version-stamp:
	$(QUIET_VERS)sh scripts/version.sh . .

$(C_OBJS): %.o: %.c $(dep_dirs_missing)
	$(QUIET_CC)$(CC) -o $@ -c  $(dep_args) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $<

$(CXX_OBJS): %.o: %.cpp $(dep_dirs_missing)
	$(QUIET_CXX)$(CXX) -o $@ -c $(dep_args) $(ALL_CPPFLAGS) $(ALL_CXXFLAGS) $<

$(OBJC_OBJS): %.o: %.m $(dep_dirs_missing)
	$(QUIET_OBJC)$(CC) -o $@ -c $(dep_args) $(ALL_CPPFLAGS) $(ALL_OBJCFLAGS) $<

########################################
# Build products

EXENAME=@EXE_LINUX@

run.sh: scripts/run.sh.in
	sed \
		-e 's,@SRCDIR''@,$(srcdir),g' \
		-e 's,@BUILDDIR''@,$(srcdir),g' \
		-e 's,@EXENAME''@,$(EXENAME),g' \
		$< > $@.tmp
	chmod +x $@.tmp
	mv $@.tmp $@

ifdef HAVE_GTK
targets += $(EXENAME)-gtk
$(EXENAME)-gtk: BASE_LIBS += $(GTK_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS) $(TYPE_LIBS)
$(EXENAME)-gtk: $(BASE_OBJS) $(GTK_OBJS)
	$(QUIET_LINK)$(CXX) $(ALL_LDFLAGS) -o $@ $^ $(ALL_LIBS)
endif

ifdef HAVE_SDL
targets += $(EXENAME)-sdl
$(EXENAME)-sdl: BASE_LIBS += $(SDL_LIBS) $(OPENGL_LIBS) $(IMAGE_LIBS) $(TYPE_LIBS)
$(EXENAME)-sdl: $(BASE_OBJS) $(SDL_OBJS)
	$(QUIET_LINK)$(CXX) $(ALL_LDFLAGS) -o $@ $^ $(ALL_LIBS)
endif

########################################
# Pseudo-targets

all: run.sh $(targets)

clean:
	$(RM) run.sh $(targets) $(ALL_OBJS) src/base/version_str.c
	$(RM) -r $(dep_dirs)

.PHONY: all clean
