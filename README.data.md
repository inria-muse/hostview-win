Data Collection and Configuration
=================================

This README explains in detail the collected metrics and collector configuration (parameters).

Configuration Basics
--------------------

HostView is installed with a default configuration file from:

	./installer/settings

The file is installed with HostView to %PROGRAMFILES%/Hostview/settings and the software checks
periodially for updates (see Automatic Updates below).


Event-based Data
----------------

### Session ###

The HostView runs (sessions) are tracked in the sqlite database (one per session). Start event 
is quaranteed, others maybe missing (user never pauses HostView or the service dies). 

	Table: session(timestamp, event)

	Possible events: start|pause|restart|stop

When HostView is stopped by the user, or in anycase at least once a day at night, we mark 
the current session as stopped, and rotate the local sqlite database and capture files. This is done 
to limit the size of the local  database and to guarantee continuous data uploads from
the monitored host.

### SysInfo ###

At start time HostView records the current system information (some of these may change from one 
session to another,, e.g. HostView version update, system memory update, ..). 

	Table: sysinfo(timestamp, ..)

### Connectivity ###

HostView tracks updates to network connections and records interface up/down events 
to the local database. Each up event triggers a packet capture on the corresponding
interface (see Packet Capture for more info).

	Table: connectivity(timestamp, connected, ...)

### Network Location ###

TODO: need to still check that all this works as advertised !!

When HostView identifies a new network location, it requests more information
about the network using RIPEstats APIs and stores this to the database. A network
location is identified using tuple () for location updates. 

	Table: location(timestamp, ..)

* Enable/disable network location identification, default 'enabled'

	netLocationActive = 1

* Location data timeout (in days), re-fresh location data if older than this

	netLocationTimeout = 7

In addition, we ask the user to label the network and store the data to the database. Network
locations are identified using tuple () for labeling.

	Table: networkname(timestamp, );

* Enable/disable network labeling, default 'enabled'

	netLabellingActive = 1


### User Activity ###

User activity (changes in foreground app, idle or fullscreen status) is polled every timeout (milliseconds), user is declared idle after 
idle timeout (milliseconds) of inactivity:

	userMonitorTimeout = 1000
	userIdleTimeout = 5000

	Table: activity(timestamp, )


Continuous Data
---------------

Continuos metrics are recorded periodically using the intervals specified below (all timeouts are in 
milliseconds). The data is stored in the current session database, and each row is timestamped with 
an UTC timestamp with millisecond accuracy. 

* Wireless network metrics (RSSI, tx/rx speed):

	wirelessMonitorTimeout = 10000

	Table: wifi(timestamp, ...)

* System battery status (charge, status):

	batteryMonitorTimeout = 15000

	Table: battery(timestamp, ...)

* Socket table (sockets to processes mapping):

	socketStatsTimeout = 60000

	Table: ports(timestamp, 5-tuple, pid, name)

* Running applications (processes and their cpu + memory use):

	systemStatsTimeout = 60000

	Table: procs(timestamp, pid, name ...)

* System IO activity (if microphone, camera, mic are in use and by which app), checked at timeout intervals (milliseconds):

	ioTimeout = 1000

	Table: io(timestamp, )


Packet Capture
--------------

We do a packet capture on each active interface to record IP+TCP/UDP headers + complete DNS traffic. Captures are 
started / stopped when network interfaces go up/down. In addition, HostView enforces a max size for a single
capture file. Related configurations:

* Max pcap file size, default 100MB:

	pcapSizeLimit = 100000000

The raw capture files are named as follows:

	File: sessionStartTime_connectivityUpTime_fileStartTime_interfaceId.pcap

The three timestamps in the file name correspond to the session start time (session table 'start' event), 
connectivity up time (connectivity table connected=1), and the pcap file creation time (can be used to 
distinguish and order rotated pcaps for merging). The interface id is the network interface id as in 
the connectivity table.

HostView does some processing on the packet trace during the capture: it parses DNS packets and HTTP requests.
DNS packets are also fully recorded in the pcap. In contrast, HTTP headers are not included in the trace, only the parsed
requests. The HTTP parsing is a bit redundant with the browser activity tracking extensions (which can also see HTTPS
requests that we cannot parse from the pcap) but we'll keep it for now to capture non-browser HTTP traffic for example.

	Table: dns(timestamp, 5-tuple, ...)
	Table: http(timestamp, 5-tuple, ...)


Browser Data
------------

The browser addons send updates on user browsing activity, pageload performance and video qoe. The location
updates (new tab is opened, new url is loaded, user switches between tabs) are written to the database, and
the performance metrics are written to json files. 

	Table: browseractivity(timestamp, browser, url)
	File: sessionStartTime_timestamp_browserupload.json


ESM questionnaire
-----------------

The experience sample questionaire popups are controlled using the following parameters:

* Enable/disable ESM

	questionnaireActive = 1

* Popup algorithm (see below):

	esmCoinFlipInterval = 3600000
	esmCoinFlipProb = 15
	esmMaxShows = 3
	esmStartHour = 8 
	esmStopHour = 23

The popup algorithm is roughly as follows:

	every day:
		esmShows = 0
		every esmCoinFlipInterval between esmStartHour to esmStopHour: 
			if (esmShows < esmMaxShows && useractive && !fullscreen && flipCoin() < exmCoinFlipProb):
				doESM()
				esmShows++


Data Uploads
------------

* Upload server URL:

	submitServer = https://muse.inria.fr/hostview2016/upload

* Upload interval in milliseconds, default 60min:

	autoSubmitInterval = 3600000

* Number of retries if upload fails and retry interval in milliseconds, default 10s (i.e. HostView tries to upload data 
3 times every ten seconds before waiting again the full interval): 

	autoSubmitRetryCount = 3
	autoSubmitRetryInterval = 10000

* cURL options for data uploads (if the key is missing, will use cURL defaults), 
see http://curl.haxx.se/libcurl/c/curl_easy_setopt.html for more info:

	curlUploadVerifyPeer = 1
	curlUploadLowSpeedLimit = 16000
	curlUploadLowSpeedTime = 5
	curlUploadMaxSendSpeed = 10000000


Automatic Updates
-----------------

* Settings file version (change if you need to track varying config versions):

    version = 1

* Update check interval in milliseconds, default 24h:

	autoUpdateInterval = 86400000

* Update URL

	updateLocation = https://muse.inria.fr/hostview2016/latest/

* Hostview update is checking for (and downloading) the following updates:

	https://muse.inria.fr/hostview2016/latest/version 					[available update version]	
	https://muse.inria.fr/hostview2016/latest/hostview_installer.exe 	[updated installer]	
	https://muse.inria.fr/hostview2016/latest/settings 					[updated settings]	
	https://muse.inria.fr/hostview2016/latest/html/esm_wizard.html		[updated ESM questionnaire UI]	
	https://muse.inria.fr/hostview2016/latest/html/network_wizard.html	[updated network labeling UI]

