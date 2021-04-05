#!/bin/sh

echo Removing rules...
rm -f /etc/udev/rules.d/70-slab-cp2130.rules
service udev restart
echo Done!
