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

The HostView runs (sessions) are tracked in the sqlite database (one per session). A new session
starts when HostView (service) is started, system resumes from sleep, or HostView is re-started after 
user paused it. A session ends when HostView (service) is shutdown, user pushes the pause button or 
the computer goes to sleep (suspend). The end event may be missing if HostView service crashed.

If HostView (PC) is on all the time, HostView auto re-starts a new session once a day (~3-5am) in order 
to limit the size of the uploaded files. The autostop event and the autorestart event of the new session will
have the same timestamps in order to facilitate reconstruction of complete sessions if need.

	Table: session(timestamp, event)

	Possible events: start|resume|restart|stop|pause|suspend|autostop|autorestart


### SysInfo ###

At session start, HostView records the current system information (some of these may change from one 
session to another,, e.g. HostView version update, system memory update, ..). 

	Table: sysinfo(timestamp, ..)


### Connectivity ###

HostView tracks updates to network connections and records interface up/down events 
to the local database. Each up event triggers a packet capture on the corresponding
interface (see Packet Capture for more info).

	Table: connectivity(timestamp, connected, ...)


### Network Location ###

When a network interface goes up, Hostview requests more information about the
network location from the backend service. TODO: just once per session ? 

	Table: location(timestamp, ..)

* Enable/disable network location identification, default 'enabled'

	netLocationActive = 1

* Network location API url (resolver service)

	netLocationApiUrl = https://muse.inria.fr/hostview/location

In addition, we ask the user to label the network locations and store the data 
to the database. Unique network locations are identified using tuple 
(device guid, gateway id) for labeling and the user is asked for the label only
once per location.

	Table: netlabel(timestamp, ...);

* Enable/disable network labeling, default 'enabled'

	netLabellingActive = 1


### System Power States ###

HostView tracks updates to system power state changes via power settings change events. 

	Table: powerstates(timestamp, event, value)

	Possible events: 
		power_source - 0 (AC power) | 1 (battery) | 2 (short-term source)
		battery_remaining - value (pourcentage)
		display_state - 0 (off) | 1 (on) | 2 (dimmed)
		user_precense - 0 (present) | 2 (not present)
		monitor_power - 0 (off) | 1 (on)

See MSDN documentation on ''Power Setting GUIDs' for more info on the above power states.


### User Activity ###

User activity (changes in foreground app, idle or fullscreen status) is polled every timeout (milliseconds), user is declared idle after 
idle timeout (milliseconds) of inactivity:

	userMonitorTimeout = 1000
	userIdleTimeout = 5000

	Table: activity(timestamp, )


Continuous Timeseries
---------------------

Continuos metrics are recorded periodically using the intervals specified below (all timeouts are in 
milliseconds). The data is stored in the current session database, and each row is timestamped with 
an UTC timestamp with millisecond accuracy.


* Wireless network metrics (RSSI, tx/rx speed):

	wirelessMonitorTimeout = 10000

	Table: wifistats(timestamp, ...)

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

	pcapSizeLimit = 1048576000

The raw capture files are named as follows:

	File: sessionStartTime_connectivityUpTime_fileStartTime_interfaceId_flag.pcap

The three timestamps in the file name correspond to the session start time (session table 'start' event), 
connectivity up time (connectivity table connected=1), and the pcap file creation time (can be used to 
distinguish and order rotated pcaps for merging). The interface id is the network interface id as in 
the connectivity table. The flag is one of F|P|L (First|Rotated|Last). If the connection only 

HostView does some processing on the packet trace during the capture: it parses DNS packets and HTTP requests.
DNS packets are also fully recorded in the pcap. In contrast, HTTP headers are not included in the trace, only the parsed
requests. The HTTP parsing is a bit redundant with the browser activity tracking extensions (which can also see HTTPS
requests that we cannot parse from the pcap) but we'll keep it for now to capture non-browser HTTP traffic for example.
Both dns and http table record the related connectivity event time (connstart), so that we can find the related pcap
file for further analysis.

	Table: dns(timestamp, connstart, 5-tuple, ...)
	Table: http(timestamp, connstart, 5-tuple, ...)


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

	esmActive = 1

* Popup algorithm (see below):

	esmCoinFlipInterval = 3600000
	esmCoinFlipProb = 15
	esmMaxShows = 3

The popup algorithm is roughly as follows:

	every day:
		esmShows = 0
		every esmCoinFlipInterval: 
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

	updateLocation = https://muse.inria.fr/hostview/latest/

* Hostview update is checking for (and downloading) the following updates:

	https://muse.inria.fr/hostview/latest/version 					[available update version]	
	https://muse.inria.fr/hostview/latest/hostview_installer.exe 	[updated installer]	
	https://muse.inria.fr/hostview/latest/settings 					[updated settings]	
	https://muse.inria.fr/hostview/latest/html/esm_wizard.html		[updated ESM questionnaire UI]	
	https://muse.inria.fr/hostview/latest/html/network_wizard.html	[updated network labeling UI]


Local Hostview Database Schema
------------------------------

CREATE TABLE IF NOT EXISTS connectivity(
	guid VARCHAR(260), 
	friendlyname VARCHAR(260), 
	description VARCHAR(260), 
	dnssuffix VARCHAR(260),
	mac VARCHAR(64),
	ips VARCHAR(300),
	gateways VARCHAR(300),
	dnses VARCHAR(300), 
	tspeed INT8, 
	rspeed INT8, 
	wireless TINYINT, 
	profile VARCHAR(64),
	ssid VARCHAR(64),
	bssid VARCHAR(64),
	bssidtype VARCHAR(20),
	phytype VARCHAR(20),
	phyindex INTEGER,
	channel INTEGER,
	connected TINYINT,
	timestamp INT8);

CREATE TABLE IF NOT EXISTS wifistats(
	guid VARCHAR(260), 
	tspeed INT8, 
	rspeed INT8, 
	signal INTEGER,
	rssi INTEGER, 
	state INTEGER, 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS procs(
	pid INTEGER, 
	name VARCHAR(260), 
	memory INTEGER,
	cpu DOUBLE, 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS ports(
	pid INTEGER, 
	name VARCHAR(260), 
	protocol INTEGER,
	srcip VARCHAR(42), 
	destip VARCHAR(42), 
	srcport INTEGER, 
	destport INTEGER, 
	state INTEGER, 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS activity(
	user VARCHAR(260), 
	pid INTEGER, 
	name VARCHAR(260), 
	description VARCHAR(260),
	fullscreen TINYINT, 
	idle TINYINT, 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS io(
	device INTEGER, 
	pid INTEGER, 
	name VARCHAR(260), 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS http(
	connstart INT8,
	httpverb VARCHAR(64), 
	httpverbparam VARCHAR(300), 
	httpstatuscode VARCHAR(64),
	httphost VARCHAR(300), 
	referer VARCHAR(300), 
	contenttype VARCHAR(300), 
	contentlength VARCHAR(300), 
	protocol INTEGER,
	srcip VARCHAR(42), 
	destip VARCHAR(42), 
	srcport INTEGER, 
	destport INTEGER, 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS dns(
	connstart INT8,
	type INTEGER, 
	ip VARCHAR(42), 
	host VARCHAR(260), 
	protocol INTEGER,
	srcip VARCHAR(42), 
	destip VARCHAR(42), 
	srcport INTEGER, 
	destport INTEGER, 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS location(
	guid VARCHAR(260), 
	ip VARCHAR(100),
	rdns VARCHAR(100), 
	asnumber VARCHAR(100), 
	asname VARCHAR(100),
	countryCode VARCHAR(100), 
	city VARCHAR(100), 
	lat VARCHAR(100), 
	lon VARCHAR(100), 
	timestamp INT8);

CREATE TABLE IF NOT EXISTS browseractivity(
	timestamp INT8, 
	browser VARCHAR(1024), 
	location VARCHAR(1024));

CREATE TABLE IF NOT EXISTS session(
	timestamp INT8, 
	event VARCHAR(32));

CREATE TABLE IF NOT EXISTS sysinfo(
	timestamp INT8, 
	manufacturer VARCHAR(32), 
	product VARCHAR(32), 
	os VARCHAR(32),
	cpu VARCHAR(32), 
	totalRAM INTEGER, 
	totalHDD INTEGER, 
	serial VARCHAR(32), 
	hostview_version VARCHAR(32), 
	settings_version INTEGER,
	timezone VARCHAR(32), 
	timezone_offset INTEGER);

CREATE TABLE IF NOT EXISTS powerstate(
	timestamp INT8, 
	event VARCHAR(32), 
	value INT);
