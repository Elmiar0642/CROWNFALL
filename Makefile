CC ?= gcc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -g
CPPFLAGS += -Isrc -Iinclude
LDFLAGS ?=

GTK_CFLAGS := $(shell $(PKG_CONFIG) --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS := $(shell $(PKG_CONFIG) --libs gtk+-3.0 2>/dev/null)
ifeq ($(GTK_LIBS),)
GTK_DEFINE :=
else
GTK_DEFINE := -DCROWNFALL_HAVE_GTK
endif

SRC := src/main.c src/engine.c src/board.c src/rules.c src/movegen.c src/dice.c src/log.c src/replay.c src/api.c src/cli.c src/gui.c
OBJ := $(SRC:.c=.o)
BIN := bin/crownfall

.PHONY: all clean run

all: $(BIN)

$(BIN): $(OBJ)
	mkdir -p bin logs/snapshots
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS) $(GTK_LIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(GTK_DEFINE) $(GTK_CFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -f $(OBJ) $(BIN)
