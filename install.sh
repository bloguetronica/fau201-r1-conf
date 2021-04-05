#!/bin/sh

echo Obtaining required packages...
apt-get -qq update
apt-get -qq install build-essential
apt-get -qq install libusb-1.0-0-dev
echo Copying source code files...
mkdir -p /usr/local/src/fau201-r1-conf
cp -f src/fau201-r1-conf.c /usr/local/src/fau201-r1-conf/.
cp -f src/GPL.txt /usr/local/src/fau201-r1-conf/.
cp -f src/LGPL.txt /usr/local/src/fau201-r1-conf/.
cp -f src/libusb-extra.c /usr/local/src/fau201-r1-conf/.
cp -f src/libusb-extra.h /usr/local/src/fau201-r1-conf/.
cp -f src/Makefile /usr/local/src/fau201-r1-conf/.
cp -f src/README.txt /usr/local/src/fau201-r1-conf/.
echo Building and installing binaries...
make -C /usr/local/src/fau201-r1-conf all install clean
echo Installing man pages...
mkdir -p /usr/local/share/man/man1
cp -f man/fau201-r1-conf.1.gz /usr/local/share/man/man1/.
echo Done!
