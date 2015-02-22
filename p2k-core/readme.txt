to compile source you need:

libusb
libusb-devel
libreadline
libreadline-devel
libncurses
libncurses-devel

then simple make, and ./p2-core

on Mac OSX pls use "make mac" instead of make.
on Windows pls use "make win32" instead of make.

commandline options:
verbose mode p2k-core -v 
device mode  p2k-core -d xxxx (22b8 is Motorola Vendor Id, by default without -d)
device mode  p2k-core -d all (search all usb devices)
slave mode   p2k-core -s (for gui applications)
modem mode   p2k-core -m /dev/ttyACM0 (specify modem device)
modem mode without modem name: "p2k-core -m" will autodetect acm device
if you use this pls use it last parameter, unless the next param will be the modem name.
bad: p2k-core -m -v
good: p2k-core -v -m

examples: 
p2k-core -v -d 22b8  verbose,scan only vid=22b8
p2k-core -v -m       verbose,modem mode with autodetect

On first run you can enter 'help' command to see how to use.

