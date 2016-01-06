HostView Windows
================

This project contains all HostView code for windows including the native data collection sotfware + various browser plugins
to complement the data collections

The repository is organized as follows:

* include   - header files
* installer - windows installer code
* lib       - 3rd party libraries
* src       - source code (HostView application, service, libraries + browser plugins)

Native HostView
---------------

The client project can be compiled using Visual Studio 2015 by opening HostView.sln, and then from the menu Build -> Batch Build -> Select All -> Rebuild.

HostView application is composed of the following:

* include (common include header files among projects or external projects - winpcap);
* lib (generated lib files for dynamic linking among projects or external projects)
* src (the source code for multiple projects)

The projects present in this solution are:

* HostView - the client
* HostViewBHO - the Internet Explorer add-on
* hostviewcli - the service
* pcap - library which exposes network relation functionalities such as capturing or querying interface information
* proc - library which exposes various functionalities such as process information (cpu, ram, sockets), system information, I/O polling, etc.
* store -library which exposes various storage functionalities such as local storage, upload / download of files, etc.

The build will generate 'bin' folder which will include Win32 / x64 and each of them Debug / Release binaries for each platform.

Browser Plugins
---------------

TODO

Hostview Installer
------------------

To generate a new installer one needs to do the following:

* have NSIS installed
* in installer\winpcap_bundle compile winpcap-nmap.nsi script
* in installer\ compile hostview.nsi script

To generate a new update / release a new version:

* increment the product version from include/product.h
* rebuild the entire solution
* go to installer\ and execute generate_update.cmd

Afterwards we will have the latest folder populated which we can then move to the server side.

Authors
-------

George Rosca <george.rosca@inria.fr>
Anna-Kaisa Petilainen <anna-kaisa.pietilainen@inria.fr>

License
-------

The MIT License (MIT)

Copyright (c) 2015-2016 Muse / INRIA

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.