all:
	${MAKE} p2k-core

linux:
	${MAKE} p2k-core LDFLAGS="-lm -lc -L/usr/lib"
mac:
	${MAKE} p2k-core-mac
win32:
	${MAKE} p2k-core-win32
clean:
	rm p2k-core.o
	rm p2k-core

p2k-core-mac: p2k-core.cpp
	gcc -DDARWIN -arch i386 -arch ppc -g -o p2k-core p2k-core.cpp -I/usr/include -lm -lc -L/usr/lib -lusb -lreadline -lcurses
p2k-core-win32: p2k-core.cpp
	gcc -DWIN32 -g -o p2k-core p2k-core.cpp -I/usr/include -lm -lc -L/usr/lib -lusb -lreadline -lcurses
p2k-core: p2k-core.cpp
	gcc -Wall -DLINUX -g -o p2k-core p2k-core.cpp -I/usr/include $(LDFLAGS) -lusb -lreadline -lncurses

install:
	install -s -m 755 -g root -o root p2k-core /usr/local/bin
		
