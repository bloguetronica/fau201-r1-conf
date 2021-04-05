#!/bin/sh

echo Applying rules...
cat > /etc/udev/rules.d/70-slab-cp2130.rules << EOF
SUBSYSTEM=="usb", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="87a0", MODE="0666"
SUBSYSTEM=="usb_device", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="87a0", MODE="0666"
EOF
service udev restart
echo Done!
