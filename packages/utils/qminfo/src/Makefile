# ─────────────────────────────────────────────────────────────────
#  Makefile for qminfo
#
#  Build:
#    make
#
#  Cross Build:
#    make ARCH=aarch64
#    make ARCH=armv7
#    make ARCH=armv5
#    make ARCH=mipsle
#
#  С sysroot (Buildroot/Yocto/OpenWRT):
#    make ARCH=mipsle SYSROOT=/path/to/staging
# ─────────────────────────────────────────────────────────────────

ARCH    ?=
SYSROOT ?=

ifeq ($(ARCH),aarch64)
    CROSS_TRIPLE := aarch64-linux-gnu
    ARCH_CFLAGS  :=
else ifeq ($(ARCH),armv7)
    CROSS_TRIPLE := arm-linux-gnueabihf
    ARCH_CFLAGS  := -march=armv7-a -mfpu=vfpv3 -mfloat-abi=hard
else ifeq ($(ARCH),armv5)
    CROSS_TRIPLE := arm-linux-gnueabi
    ARCH_CFLAGS  := -march=armv5te -mfloat-abi=soft
else ifeq ($(ARCH),mipsle)
    CROSS_TRIPLE := mipsel-linux-gnu
    ARCH_CFLAGS  := -march=mips32 -mabi=32 -EL
else
    CROSS_TRIPLE :=
    ARCH_CFLAGS  :=
endif

ifdef CROSS_TRIPLE
    CC    := $(CROSS_TRIPLE)-gcc
    STRIP := $(CROSS_TRIPLE)-strip
else
    CC    := gcc
    STRIP := strip
endif

TARGET := qminfo
SRC    := qminfo.c
HDRS   := qminfo-bands.h qminfo-helpers.h
OBJ    := $(SRC:.c=.o)

# pkg-config
PKG_QMI  := qmi-glib
PKG_MBIM := mbim-glib

ifdef SYSROOT
    PKG_CONFIG := PKG_CONFIG_SYSROOT_DIR=$(SYSROOT) \
                  PKG_CONFIG_LIBDIR=$(SYSROOT)/usr/lib/pkgconfig:$(SYSROOT)/usr/share/pkgconfig \
                  PKG_CONFIG_PATH= \
                  pkg-config
    SYSROOT_FLAGS := --sysroot=$(SYSROOT)
else ifdef CROSS_TRIPLE
    PKG_CONFIG := PKG_CONFIG_PATH=/usr/lib/$(CROSS_TRIPLE)/pkgconfig \
                  pkg-config
    SYSROOT_FLAGS :=
else
    PKG_CONFIG    := pkg-config
    SYSROOT_FLAGS :=
endif

# if mbim-glib — link it (need for QMI over MBIM)
MBIM_LIBS := $(shell $(PKG_CONFIG) --libs $(PKG_MBIM) 2>/dev/null)
MBIM_CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_MBIM) 2>/dev/null)

CFLAGS := $(shell $(PKG_CONFIG) --cflags $(PKG_QMI)) \
          $(MBIM_CFLAGS) \
          $(ARCH_CFLAGS) \
          $(SYSROOT_FLAGS) \
          -Wall -Wextra -O2 -pipe

LIBS   := $(shell $(PKG_CONFIG) --libs $(PKG_QMI)) \
          $(MBIM_LIBS) \
          -lm

PREFIX := /usr/local
BINDIR := $(PREFIX)/bin

.PHONY: all clean install uninstall run json run-mbim json-mbim strip help check-pkg

all: check-pkg $(TARGET)

check-pkg:
	@$(PKG_CONFIG) --exists $(PKG_QMI) || \
		{ echo "Error: $(PKG_QMI) not found. Install libqmi-glib-dev"; exit 1; }
	@$(PKG_CONFIG) --exists $(PKG_MBIM) && \
		echo "  mbim-glib: found (QMI over MBIM supported)" || \
		echo "  mbim-glib: not found (QMI over MBIM disabled)"

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

strip: $(TARGET)
	$(STRIP) --strip-all $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)
