# Multiload-ng

## Overview
Multiload-ng is a near-complete rewrite of the good old GNOME multiload applet.
<!--TODO plugin screenshots, into all panels - change shape and colors too, to illustrate every possibility-->
<!--TODO screenshot of preferences window too-->
It has plugins for the following panels:
- XFCE 4 (xfce4-panel)
- LXDE (lxpanel)
- MATE (mate-panel)

In addition it can be built as a standalone window, that is, not embedded in any panel.



## Features
- Draw graphs of system resources
- Very customizable
- Automatically adapts to container changes (panel or window)
- Written in pure C with few dependencies - little CPU/memory footprint
- Custom actions on double click



## The Graphs
#### CPU Graph
Draws CPU usage, differentiating between User/System/Nice/IOWait.
#### Memory Graph
Draws RAM usage, differentiating between memory used by the applications
(directly and through shared modules) and memory used as cache/buffers.
#### Network Graph
Draws network I/O of every supported network interface, differentiating
between input, output and local (loopback, adb, etc) traffic.
#### Swap Graph
Draws swap usage, when detected.
#### Load Average Graph
Draws load average, as returned by `uptime`.
#### Disk Graph
Draws Disk I/O, differentiating between read and write.
#### Temperature Graph
Draws temperature of the system, based on the hottest temperature
detected among the supported sensors in the system.



## History
Multiload-ng started as a simple port of nandhp's source to lxpanel>0.7.

As I become familiar with code, I started making other little changes, and cleaning the code.
I then contacted original author, but received no reply - meanwhile the plugin continued improving.

This came to the point where the changes became many and deep, and I realized that this wasn't the same project anymore.

I knew that a fresh start would give a boost to development, and at the same time it
would allow to choose future directions with more ease.

For the above reasons, I made Multiload-ng a separate project.
The name changes too (so the filenames), in order to allow them to be installed together.



## System Requirements
#### Common requirements
These are the packages required to build any version of the plugin.
See per-panel section below for full list.

Package                     | Min version
:-------------------------- | -------------:
gtk+                        | >= 2.18.0
cairo                       | >= 1.0
libgtop                     | >= 2.11.92

#### Requirements for XFCE4 panel
In addition to common requirements (see above)
these packages are required to build XFCE4 panel plugin:

Package                     | Min version
:-------------------------- | -------------:
libxfce4panel               | >= 4.6.0
libxfce4util                | >= 4.6.0
libxfce4ui-1 OR libxfcegui4 | >= 4.8.0

Note that XFCE 4.6 or greater is required.

#### Requirements for LXDE panel
In addition to common requirements (see above)
these packages are required to build LXDE panel plugin:

Package                     | Min version
:-------------------------- | -------------:
lxpanel                     | >= 0.7.0
libfm                       | >= 1.2.0

Due to an error in lxpanel source, if you are using lxpanel 0.7.0 you will
need also libmenu-cache. This was fixed in version 0.7.1. Read about this
[here](http://wiki.lxde.org/en/How_to_write_plugins_for_LXPanel#Preconditions).

Note that LXDE 0.7 or greater is required.

#### Requirements for MATE panel
In addition to common requirements (see above)
these packages are required to build MATE panel plugin:

Package                     | Min version
:-------------------------- | -------------:
libmatepanelapplet-4        | >= 1.7.0

Note that MATE 1.7 or greater is required.

#### Requirements for temperature graph
This is not a build-time requirement, rather a run-time one. The plugin search
sysfs nodes corresponding to thermal zones, chooses the hottest one and draws it
in the graph. The only requirement here is a Linux kernel compiled with
`CONFIG_THERMAL (Generic Thermal sysfs driver)`. Any modern kernel (since 2010)
sets it automatically, so it should be just fine.




## Build instructions

#### Get the source
Execute the following command line (you must have git installed):  
`git clone https://github.com/udda/multiload-ng`

If you don't have git, download the source ZIP [here](https://github.com/udda/multiload-ng/archive/master.zip).

#### Configure
Move to the directory that contains source code just cloned and run:  
`./autogen.sh`

Now run configure script:  
`./configure --prefix=/usr`
Change prefix as needed (/usr is the default of most distros and it's just OK; if not specified it defaults to /usr/local).

Configure script automatically detects installed panels (and related development packages) and enables panel plugins accordingly.  
You can force enable/disable for some panels (and standalone). To learn how, along with other available options, type:  
`./configure --help`

Then run `./configure` with selected options.

#### Build
This is simple. Move to the directory that contains source code and execute:  
`make`

#### Install/uninstall
To install (must run `make` before), execute:  
`sudo make install`

To later uninstall you need source directory. If you deleted it, just download again, and run *Configure* part. Then execute:  
`sudo make uninstall`



## FAQ

#### Q: Which are the differences with original Multiload applet?
A: First of all, this project is *forked* from original Multiload, so they share the same base. There are some notable differences:

* Original multiload contains old and unmantained code, Multiload-ng is actively mantained
* Multiload-ng is very well documented (see [Wiki](../../wiki))
* Multiload-ng has additional graphs, and more will be added in the future
* Multiload-ng has more graphical customizations, like individual colored border
* Multiload-ng has color themes support
* Multiload-ng responds to mouse events with customizable actions
* Multiload-ng can choose its orientation regardless of panel orientation
* Multiload-ng can also be run without any panel

#### Q: Doesn't a system monitor use system resources by itself?
A: Yes. This is true for every system monitor. That's why resources usage from Multiload-ng is kept as low as possible.

