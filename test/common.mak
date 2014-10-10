LIBS :=
LDFLAGS :=
CFLAGS := -O0 -g
CWARN := -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wmissing-prototypes
override CFLAGS += $(CWARN) -I../.. -I../../include

ifdef ASAN
override CFLAGS += -fsanitize=address
override LDFLAGS += -fsanitize=address
endif
