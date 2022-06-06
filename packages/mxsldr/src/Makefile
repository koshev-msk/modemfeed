DESTDIR ?=
prefix ?= "/usr/local"
PKG_CONFIG ?= pkg-config

LIBUSB_FOUND := $(shell $(PKG_CONFIG) --atleast-version=1 libusb-1.0 && echo Y)

ifneq ($(LIBUSB_FOUND),Y)
$(error Missing libusb!)
endif

CFLAGS += $(shell $(PKG_CONFIG) --cflags libusb-1.0)
LIBS += $(shell $(PKG_CONFIG) --libs libusb-1.0)

all: mxsldr

mxsldr: mxsldr.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ -lusb-1.0

install: mxsldr
	mkdir -p $(DESTDIR)/$(prefix)/bin
	install -m 755 $^ $(DESTDIR)/$(prefix)/bin

clean:
	rm -f mxsldr
