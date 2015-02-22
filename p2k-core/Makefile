all:
	${MAKE} p2k-core
mac:
	${MAKE} p2k-core-mac
win32:
	${MAKE} p2k-core-win32
clean:
	rm p2k-core.o
	rm p2k-core

p2k-core-mac: p2k-core.o
	gcc -DDARWIN -arch i386 -arch ppc -g -o p2k-core p2k-core.c -I/usr/include -lm -lc -L/usr/lib -lusb -lreadline -lcurses
p2k-core-win32: p2k-core.o
	gcc -DWIN32 -g -o p2k-core p2k-core.c -I/usr/include -lm -lc -L/usr/lib -lusb -lreadline -lcurses
p2k-core: p2k-core.o
	gcc -Wall -DLINUX -g -o p2k-core p2k-core.c -I/usr/include -lm -lc -L/usr/lib -lusb -lreadline -lcurses
p2k-core.o: p2k-core.c
	gcc -g -c p2k-core.c

install:
#	cp ./p2k-core /usr/local/bin/p2k-core
	install -s -m 755 -g root -o root p2k-core /usr/local/bin
		
