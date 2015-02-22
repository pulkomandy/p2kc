CC = cc
CFLAGS = -Wall -g -export-dynamic `pkg-config --cflags --libs gtk+-2.0 libglade-2.0`

all:
	cd p2k-core && $(MAKE) all;
	$(MAKE) p2kc
p2kc: p2kc.c
	$(CC) $(CFLAGS) $@.c -o $@

clean:
	rm -f p2kc p2kc.o p2k-core/p2k-core p2k-core/p2k-core.o ~/p2kc.conf

install:
	install -s -m 755 -g root -o root p2k-core/p2k-core /usr/local/bin
	install -s -m 755 -g root -o root p2kc /usr/local/bin
	mkdir -p /usr/local/share/p2kc/pixmaps
	install -m 644 pixmaps/*.* /usr/local/share/p2kc/pixmaps
	install -m 644 *.glade /usr/local/share/p2kc
