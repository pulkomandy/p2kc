What is it
==========

P2KC is a tool to communicate with Motorola P2K phones.

Supported devices:
 * Motorola RAZR² v9
 * This list is very incomplete. Please test and report if your phone works.

This code was originally developped for "P2K Commander" and open sourced by
Sandor Otvos. Thanks for this, it is very useful!

These mobile phones implement a configurable USB interface and can be used as
an AT Modem (over USB-CDC), an USB LAN adapter, a mass storage device (exposing
the SD card contents), a recovery "flash" mode, and P2K mode.

In P2K mode, the phone receives and replies to commands. This mode can be used
to access the phone internal memory as well as the SD card. It is more reliable
than the SD card mode because it avoids sharing the filesystem between the phone
and the computer accessing it (a sure way to corrupt the filesystem from my
experience).

Tools provided
==============

p2k-core
--------

There is a simple tool called p2k-core with an interactive command line
interface. This implements the P2K protocol and communicates with the phone. It
is highly portable and needs only libusb, readline, and ncurses.

p2kc
----

p2kc is a file-manager-like UI shell for p2k-core. It uses GTK for user
interface, and communicates with p2k-core through pipes to send commands and
receive results.

Changelog
=========

2015-02-22
----------

Development resumes on github.
* Added Haiku support.

2007-12-30
----------

Version 0.6
* Source archive now contains pre-built binaries. You can "make install" without compiling. If it doesn't work on your distribution, please recompile.
* Seem upload and download function implemented.
* In the options dialog you can set extra arguments to p2k-core, and a command to run on exit.
* "Create folder" and "delete file" operations implemented.

2007-12-11
----------

* You can now use "make install" to install p2kc and p2k-core into /usr/local.
* Lots of tooltips and keyboard shortcuts added. TAB will move cursor to left to right panel, and vice-versa. You can use F1...F10, and rightclick menus
* List views can be sorted (by name , size , time). Just click onto column headers.
*  File download, upload and delete completed. p2k filelist automatically updated after operation.
* You can enter p2k-core commands in the log window. For example to unset the "read-only" attribute: "file a /a/mobile/poicture/mypic.jpg 0".
* pressing "esc" will copy last active full filename to the log window

