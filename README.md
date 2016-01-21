HostView Windows
================

This project contains all HostView code for windows including the native data collection sotfware + various browser plugins
to complement the data collections

The repository is organized as follows:

* include   - header files
* installer - windows installer scripts and code
* lib       - 3rd party libraries
* src       - source code (HostView application, service, libraries + browser plugins)

Native HostView
---------------

HostView application is composed of the following:

* include (common include header files among projects or external projects - winpcap);
* lib (generated lib files for dynamic linking among projects or external projects)
* src (the source code for multiple projects)

The projects present in this solution are:

* HostViewBHO - Internet Explorer add-on
* HostView - client (UI)
* hostviewcli - service
* pcap - library which exposes network relation functionalities such as capturing or querying interface information
* proc - library which exposes various functionalities such as process information (cpu, ram, sockets), system information, I/O polling, etc.
* store -library which exposes various storage functionalities such as local storage, upload / download of files, etc.

### Building

The client project can be compiled using Visual Studio 2015 (free Community edition works fine) by opening HostView.sln, and then, 
choose from the menu Build -> Batch Build -> Select All -> Rebuild. The build will generate 'bin' folder which will include Win32 / x64 and 
each of them Debug / Release binaries for each platform.

Browser Plugins
---------------

* Internet Explorer - src/HostviewBHO (part of the native HostView solution)
* Firefox - src/HostviewFirefox (see specific README for more info)
* Chrome - src/HostviewChrom (see specific README for more info)

Hostview Installer
------------------

To generate a new installer one needs to do the following:

* have NSIS installed
* in installer\winpcap_bundle run 'makensis winpcap-nmap.nsi'
* in installer\ run 'makensis hostview.nsi'

To generate a complete update / release of a new version of HostView:

* increment the product version from include/product.h
* rebuild the entire solution
* go to installer\ and execute generate_update.cmd

Afterwards we will have the latest folder populated which we can then move to the server side.

Local Testing
-------------

There is a simple node.js script that can receive raw data uploads from Hostview. Usage:

	$ node script/userver.js

The script listens to uploads at http://localhost:3000 and stores received files to ./tmp/.

Configure this url to hostview settings 'submitServer' (debug builds do this by default).

Authors
-------

George Rosca <george.rosca@inria.fr>
Anna-Kaisa Petilainen <anna-kaisa.pietilainen@inria.fr>

The MIT License (MIT)
Copyright (c) 2015-2016 MUSE / Inria
