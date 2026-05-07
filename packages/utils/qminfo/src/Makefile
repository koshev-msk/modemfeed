# ─────────────────────────────────────────────────────────────────
#  Makefile for qminfo
#
#  Usage:
#    make                          — compile
#    make install                  — install /usr/local/bin
#    make uninstall                — remove
#    make clean                    — remove build
#    make run  [DEV=/dev/cdc-wdm0] — run text mode
#    make json [DEV=/dev/cdc-wdm0] — run JSON output
# ─────────────────────────────────────────────────────────────────

CC      := gcc
TARGET  := qminfo
SRC     := qminfo.c
OBJ     := $(SRC:.c=.o)

PKG     := qmi-glib
CFLAGS  := $(shell pkg-config --cflags $(PKG)) -Wall -Wextra -O2
LIBS    := $(shell pkg-config --libs   $(PKG)) -lm

PREFIX  := /usr/local
BINDIR  := $(PREFIX)/bin
DEV     := /dev/cdc-wdm0

# ─────────────────────────────────────────────────────────────────

.PHONY: all clean install uninstall run json check-pkg

all: check-pkg $(TARGET)

check-pkg:
	@pkg-config --exists $(PKG) || \
		{ echo "Error: $(PKG) not found. Install libqmi-glib-dev"; exit 1; }

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

run: $(TARGET)
	sudo ./$(TARGET) $(DEV)

json: $(TARGET)
	sudo ./$(TARGET) $(DEV) --json

clean:
	rm -f $(OBJ) $(TARGET)
