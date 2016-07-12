# HostView Data Collection and Configuration #

This README explains in detail the collected metrics and collector configuration (parameters).


## Configuration Basics ##

HostView is installed with a default configuration file from:

	./installer/settings

The file is installed with HostView to %PROGRAMFILES%\Hostview\settings and the software checks
periodially for updates (see Automatic Updates below). To track the current settings version, 
the file has the following key (reported along with session configuration to the backend):

    Setting: version = 1

For boolean options, 0 == FALSE and 1 == TRUE.

If an option is missing from the config file, the code has some hard-coded defaults (more or less
the ones documented below).


## Debugging ##

Set the debug flag to 1 to turn on more verbose logging and to keep a copy of all uploaded files 
(in %PROGRAMFILES%\Hostview\debug):

	Setting: debug = 0



## Event-based Data ##


### Session ###

The HostView runs (sessions) are tracked in the sqlite database (one per session). A new session
starts when HostView (service) is started, system resumes from sleep, or HostView is re-started after 
user paused it. A session ends when HostView (service) is shutdown, user pushes the pause button or 
the computer goes to sleep (suspend). The end event may be missing if HostView service crashed.

If HostView (PC) is on all the time, HostView auto re-starts a new session once a day at night in order 
to limit the size of the uploaded file(s). The autostop event and the autorestart event of the new session will
have the same timestamps in order to facilitate reconstruction of complete sessions if need.

	Table: session(timestamp, event)

	Possible events: start|resume|restart|stop|pause|suspend|autostop|autorestart

If user pauses HostView, it will restart again after a configured timeout (user has the option to restart
earlier as well):

	Setting: autoRestartTimeout = 1800000


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
network location from the backend service. 

	Table: location(timestamp, ..)

* Enable/disable network location identification, default 'enabled'

	Setting: netLocationActive = 1

* Network location API url (resolver service)

	Setting: netLocationApiUrl = https://muse.inria.fr/hostview/location

In addition, we ask the user to label the network locations and store the data 
to the database. Unique network locations are identified using tuple 
(device guid, gateway) and the user is asked for the label only once per location.

	Table: netlabel(timestamp, ...);

* Enable/disable network labeling, default 'enabled'

	Setting: netLabellingActive = 1


### System Power States ###

HostView tracks updates to system power state changes via power settings change events. 

	Table: powerstates(timestamp, event, value)

	Possible events: 
		power_source - 0 (AC power) | 1 (battery) | 2 (short-term source e.g. UPS)
		battery_remaining - value (pourcentage)
		display_state - 0 (off) | 1 (on) | 2 (dimmed)
		user_precense - 0 (present) | 2 (not present)
		monitor_power - 0 (off) | 1 (on)

See MSDN documentation on 'Power Setting GUIDs' for more info on the above power states.


### User Activity ###

User activity (changes in foreground app, idle or fullscreen status) is polled every timeout (milliseconds), user is declared idle after 
idle timeout (milliseconds) of inactivity:

	Setting: userMonitorTimeout = 1000
	Setting: userIdleTimeout = 5000

	Table: activity(timestamp, )


## Continuous Timeseries ##

Continuous metrics are recorded periodically using the intervals specified below (all timeouts are in 
milliseconds). The data is stored in the current session database, and each row is timestamped with 
an UTC epoch timestamp with millisecond accuracy.


* Wireless network metrics (RSSI, tx/rx speed):

	Setting: wirelessMonitorTimeout = 10000

	Table: wifistats(timestamp, ...)

* Socket table (sockets to processes mapping):

	Setting: socketStatsTimeout = 60000

	Table: ports(timestamp, 5-tuple, pid, name)

* Running applications (processes and their cpu + memory use):

	Setting: systemStatsTimeout = 60000

	Table: procs(timestamp, pid, name ...)

* System IO activity (if microphone, camera, mic are in use and by which app), checked at timeout intervals (milliseconds):

	Setting: ioTimeout = 1000

	Table: io(timestamp, )


The IO devices are identified as follows:

enum IoDevice {
        Camera = 0, 
        Keyboard = 1, 
        Microphone = 2,
        Mouse = 3, 
        Speaker = 4
};



## Packet Capture ##

HostView does packet capture on each active interface to record IP+TCP/UDP headers + complete DNS traffic. Captures are 
started / stopped when network interfaces go up/down. In addition, HostView enforces a max size for a single
capture file. Related configurations:

* Max pcap file size, default 100MB:

	Setting: pcapSizeLimit = 100000000

The raw capture files are named as follows:

	File: sessionStartTime_connectivityUpTime_number_interfaceId_[part|last].pcap

The two timestamps in the file name correspond to the session start time (session table 'start' event) 
and connectivity up time (connectivity table connected=1). Then number is just an increasing counter (first file 
has number 1). The interface id is the network interface id as in the connectivity table. The flag
after the device id indicates if this file is the last pcap of the connection, or if its a rotated file (part).
The backend processing scripts should always first merge all pcaps with same session and connection id
before any processing. The number and the part|last flag can be used to track if all the pcaps are 
available.

HostView can do some processing on the packet trace during the capture: it parses DNS packets and HTTP requests.
DNS packets are also fully recorded in the pcap. In contrast, HTTP headers are not included in the trace, only the parsed
requests. The HTTP parsing is a bit redundant with the browser activity tracking extensions (which can also see HTTPS
requests that we cannot parse from the pcap) but we'll keep it for now to capture non-browser HTTP traffic for example.

The dns and http table record the id of the related connectivity event time (connstart), so that we can find 
the raw pcap file corresponding to the rows.

	Table: dns(timestamp, connstart, 5-tuple, ...)
	Table: http(timestamp, connstart, 5-tuple, ...)

* Enable trace parsing

	Setting: enableDnsParsing = 1
	Setting: enableHttpParsing = 0 

TODO: we should probably remove the http parsing feature all together, browser plugins do better job here.


## Browser Data ##

The browser addons send updates on user browsing activity, pageload performance and video qoe. The location
updates (new tab is opened, new url is loaded, user switches between tabs) are written to the database, and
the performance metrics are written to json files. 

	Table: browseractivity(timestamp, browser, url)

	File: sessionStartTime_timestamp_browserupload.json


## ESM questionnaire ##

The experience sample questionaire popups are controlled using the following parameters:

* Enable/disable ESM

	Setting: esmActive = 1

* Applist history (show apps that have been active in the last x milliseconds):

	Setting: esmAppListHistory = 60000

* Popup algorithm (see below):

	Setting: esmCoinFlipInterval = 3600000
	Setting: esmCoinFlipProb = 15
	Setting: esmMaxShows = 3

The popup algorithm is roughly as follows:

	every day:
		esmShows = 0
		every esmCoinFlipInterval: 
			if (esmShows < esmMaxShows && useractive && !fullscreen && flipCoin() < exmCoinFlipProb):
				doESM()
				esmShows++


The results are stored in the session db (rows with the same timestamp belong to the same questionnaire, timestamp
is the time it was shown to the user):

	Table: esm(timestamp, ..)
	Table: esm_activity_tags(timestamp, ...)
	Table: esm_problem_tags(timestamp, ...)


## Data Uploads ##

All data files ready for upload are stored in %PROGRAMFILES%\HostView\submit. When a file is moved to submit folder it
is compressed (7z zip). HostView tries to upload the zip files periodically to the submitServer, and upon success
removes the files from the local file system. HostView also keeps track of the disk space used by the un-submitted files,
and if these files take more than 50% of the available free space, oldest files are removed to make space. 

Following settings control the upload behavior:

* Upload server URL:

	Setting: submitServer = https://muse.inria.fr/hostviewupload

* Upload interval in milliseconds, default 60min:

	Setting: autoSubmitInterval = 3600000

* Number of retries if upload fails and retry interval in milliseconds, default 10s (i.e. HostView tries to upload data 
3 times every ten seconds before waiting again the full interval): 

	Setting: autoSubmitRetryCount = 3
	Setting: autoSubmitRetryInterval = 10000

* cURL options for data uploads (if the key is missing, will use cURL defaults), 
see http://curl.haxx.se/libcurl/c/curl_easy_setopt.html for more info:

	Setting: curlUploadVerifyPeer = 1
	Setting: curlUploadLowSpeedLimit = 12500 // 12.5Kb/s 10% of max
	Setting: curlUploadLowSpeedTime = 10
	Setting: curlUploadMaxSendSpeed = 125000 // 125Kb/s == 1Mbit/s


## Automatic Updates ##

* Update check interval in milliseconds, default 24h:

	autoUpdateInterval = 86400000

* Update URL

	Setting: updateLocation = https://muse.inria.fr/hostview/latest/

* Hostview update is checking for (and downloading) the following updates:

	https://muse.inria.fr/hostview/latest/version 					[updated version]	
	https://muse.inria.fr/hostview/latest/hostview_installer.exe 	[updated installer]	
	https://muse.inria.fr/hostview/latest/settings 					[updated settings]	
	https://muse.inria.fr/hostview/latest/html/esm_wizard.html		[updated ESM questionnaire UI]	
	https://muse.inria.fr/hostview/latest/html/network_wizard.html	[updated network labeling UI]


## Local Hostview Database Schema ##

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
	connstart INT8);

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

CREATE TABLE IF NOT EXISTS netlabel(
	timestamp INT8, 
	guid VARCHAR(260), 
	gateway VARCHAR(64), 
	label VARCHAR(260));

CREATE TABLE IF NOT EXISTS esm(
	timestamp INT8, 
	ondemand TINYINT, 
	duration INT8, 
	qoe_score INTEGER);

CREATE TABLE IF NOT EXISTS esm_activity_tags(
	timestamp INT8, 
	appname VARCHAR(260), 
	tags VARCHAR(1024));
	
CREATE TABLE IF NOT EXISTS esm_problem_tags(
	timestamp INT8, 
	appname VARCHAR(260), 
	tags VARCHAR(1024));
