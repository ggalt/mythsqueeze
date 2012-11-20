Copyright George Galt 2012
george at galtfamily dot net

Released under the GPL Version 2

This plugin is a very stripped down version of the prior mythsqueezebox plugin.
It's only function is to start the squeezeslave client using parameters set
through MythTV's settings function, and connect to the Logitech Media Server
(formerly Squeezebox Server) via the CLI (on port 9090) to control the
squeezeslave client and to receive messages to display.  Please take a 
look at video located here: http://www.youtube.com/watch?v=--x2amE0Qzk

You will need to have a suitable mythtv development environment.  For me, using
Fedora 16 (x86_64) with the atrpms (www.atrpms.net) I added the mythtv-devel
package as well as the following: avahi-compat-libdns_sd-devel libass-devel
fftw-devel libcrystalhd-devel pulseaudio-libs-devel xvidcore-devel x264-devel
libvpx-devel opencore-amr-devel faac-devel libva-devel

Install MythTV 0.25 source for mythplugins (NOTE: DO NOT use the development
branch, but the source tarball from www.mythtv.org) and run "./configure" to
create an environment in which you can compile the source for this plugin.
Copy source to a subdirectory under the mythplugins directory
(e.g., "MythSqueeze") and run the following commands:

qmake-qt4 -recursive
make
sudo make install

You will need to build and installed SqueezeSlave.  Latest available via:
svn checkout http://squeezeslave.googlecode.com/svn/squeezeslave/trunk/ squeezeslave
You are likely to need a number of development files.  Off the top of my head,
I believe you will need at lease theses: libmad-devel libflac-devel portaudio-devel

Copy the "library.xml" and "media_settings.xml" files into your local .mythtv
directory and edit them.  On my setup, these files were originally found at
/usr/share/mythtv/themes/defaultmenu/

To library.xml add:

   <button>
           <type>MYTHSQUEEZE</type>
           <text>MythSqueeze</text>
           <depends>mythsqueeze</depends>
           <action>PLUGIN mythsqueeze</action>
   </button>


To media_settings.xml add:

   <button>
           <type>MYTHSQUEEZE</type>
           <text>MythSqueeze Settings</text>
           <depends>mythsqueeze</depends>
           <action>CONFIGPLUGIN mythsqueeze</action>
   </button>
