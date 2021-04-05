This package contains all the necessary files and scripts to install the
FAU201 Rev. 1 Configuration Program. The scripts included here are compatible
with most 32-bit and 64-bit Debian based operating systems (e.g. Linux Mint,
Ubuntu and variants, etc). Prior to installation, you must certify that your
system is Debian based (or at least uses apt-get) and that you have an active
Internet connection.

A list of the included scripts follows:
– install-rules.sh;
– install.sh;
– uninstall-rules.sh;
– uninstall.sh.

In order to compile and install the program for the first time, you have to
run "install.sh" by typing "sudo ./install.sh" on a terminal window, after
changing your working directory to the current one. This script will first
obtain and install the required "build-essential" and "libusb-1.0-0-dev"
packages (if they are not installed yet). Then it will compile the binary and
move it to "/usr/local/bin/". It will also copy the corresponding man page to
"/usr/local/share/man/man1/".

The above mentioned installation script won't create any rules to access
unconfigured devices (or, in other words, ones that contain blank CP2130
interfaces). If you don't have equivalent rules for accessing such devices
(with matching VIP 0x10C4 and PID 0x87A0), you must run this script too, by
invoking "sudo ./install-rules.sh".

To uninstall, you should run "uninstall.sh" by typing "sudo ./uninstall.sh",
again on a terminal window after making sure that your working directory is
this one. This will delete the previously installed binary, the corresponding
man page and the source code. However it won't remove the "build-essential"
and "libusb-1.0-0-dev" packages, since they could be already installed prior
to the first installation. This script will not remove the previously created
rules (if applicable) and, therefore, you should run "uninstall-rules.sh" by
invoking "sudo ./uninstall-rules.sh" if you wish to do so.
