Copyright George Galt 2012
Released under the GPL Version 2

You may need to add the following libraries (tested under x86_64 Fedora 16 using ATrpms.net repo) to compile the source:
avahi-compat-libdns_sd-devel libass-devel fftw-devel libcrystalhd-devel pulseaudio-libs-devel xvidcore-devel x264-devel libvpx-devel opencore-amr-devel faac-devel libva-devel

Install MythTV 0.25 source for mythplugins and run "./configure" to create an environment in which you can compile the source
Copy source to a subdirectory under the mythplugins directory (e.g., "MythSqueeze")
Run qmake-qt4, then make and make install

You will need to have built and installed SqueezeSlave.  Latest available via:
svn checkout http://squeezeslave.googlecode.com/svn/squeezeslave/trunk/ squeezeslave

