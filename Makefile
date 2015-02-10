NAME = luci-app-rtorrent
VERSION = $(shell awk '/^Version:/ {print $$2}' control)
ARCH = $(shell awk '/^Architecture:/ {print $$2}' control)
IPK = $(NAME)_$(VERSION)_$(ARCH).ipk

all: ipk

ipk: clean
	mkdir ipk
	cd src && tar czf ../ipk/data.tar.gz *
	echo "/etc/config/rtorrent" > ipk/conffiles
	cp control ipk/control
	cd ipk && tar czf control.tar.gz control conffiles
	echo "2.0" > ipk/debian-binary
	cd ipk && tar czf $(IPK) control.tar.gz data.tar.gz debian-binary
	rm -f ipk/conffiles ipk/control ipk/*.tar.gz ipk/debian-binary

clean:
	rm -fr ipk

