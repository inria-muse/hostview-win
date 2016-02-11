/*
   HostView AddOn

   Copyright (C) 2015-2016 MUSE / Inria Paris-Roquencourt 

   The MIT License (MIT)

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
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

const { Unknown } = require('sdk/platform/xpcom');
const {Cc, Ci, Cu} = require("chrome");
const dnsservice = Cc["@mozilla.org/network/dns-service;1"].createInstance(Ci.nsIDNSService);

const self = require('sdk/self');
const system = require('sdk/system');
const tabs = require("sdk/tabs");
const pageMod = require("sdk/page-mod");
const URL = require("sdk/url").URL;

// upload helper routines
const upload = require("./lib/upload.js");

// Caches for pages and tabs
var pages = {};
var tabcache = {};

/* Cleanup req path + params from the URL. */
var stripUrl = function(url) {
	var u = URL(url);
	var obj = {
		origin : u.protocol  + (u.hostname ? '//' + u.hostname : '')  + (u.port ? ':' + u.port : ''),
		hostname : u.hostname,
		protocol : u.protocol.replace(":",""),
		port : (u.port ? u.port : (u.protocol.indexOf('https')>=0 ? 443 : 80)),
        file_ext : (u.pathname.lastIndexOf('.') > 0 ? u.pathname.substr(u.pathname.lastIndexOf('.')+1) : undefined),
	};
	return obj;
};

var resolveIp = function(hostname) {	
	var flags = Ci.nsIDNSService.RESOLVE_CANONICAL_NAME;	
	try {
    	// this should be instant as the IP is cached by the browser        
        var r = dnsservice.resolve(hostname, flags);
        if (r.hasMore()) {
            return dnsservice.resolve(hostname, flags).getNextAddrAsString();
        }
    } catch (e) {
    }		
    return null;
}

// tab is active (current url goes foreground)
function onActivate(tab) {
	var p = pages[tab.id+'_'+tab.url];
	if (p) {
		upload.sendlocation(p.hostname);
	}
}

// tab is deactive (current url goes background)
function onDeactivate(tab) {
	// TODO: do we need to signal on background events to hostview ?
}

// page loaded
function onReady(tab) {
	tabcache[tab.id] = tab.url;

	// resolve new location
	var p = stripUrl(tab.url);	
	if (p.hostname)
		p.ip = resolveIp(p.hostname);
	pages[tab.id+'_'+tab.url] = p;	

	onActivate(tab);
}

// tab is closed (upload current url data)
function onClose(tab) {
	if (pages[tab.id+'_'+tab.url]) {
		onDeactivate(tab);
		delete pages[tab.id+'_'+tab.url];
	}

	if (tabcache[tab.id]) {
		delete tabcache[tab.id];
	}
}

function onOpen(tab) {
	if (!tabcache[tab.id]) {
		tabcache[tab.id] = tab.url;
		tab.on("ready", onReady);
		tab.on("activate", onActivate);
		tab.on("deactivate", onDeactivate);
		tab.on("close", onClose);	
	}
}

// page-mod to track objects on pages
pageMod.PageMod({
	include: ["*"],
    contentScriptFile: self.data.url('contentscript.js'),
	contentScriptWhen: 'end', // after the page has loaded
	onAttach: function(worker) {
		// request and handle pageload stats
		worker.port.on("plt", function(obj) {
			// cleanup all urls
			obj.location = stripUrl(obj.location);
			if (obj.location.hostname)
				obj.location.ip = resolveIp(obj.location.hostname);

			for (var i = 0; i < obj.restiming.length; i++) {
				let e = obj.restiming[i];
				e.name = stripUrl(e.name);
				if (e.name.hostname)
					e.ip = resolveIp(e.name.hostname);
			}

			// add some addon + browser metadata
			obj.addon_version = self.version;
			obj.system_platform = system.platform;
			obj.system_platform_version = system.platform_version;
			obj.system_architecture = system.architecture;
			obj.system_name = system.name;
			obj.system_version = system.version;
			obj.system_vendor = system.vendor;

			// upload with hostview
			upload.sendjson(obj);
		});
		worker.port.emit("getPlt");

		// TODO: something similar for video qoe stats
	}
});

/** Extension is loaded. */
exports.main = function (options, callbacks) {
	// mark all existing tabs open and loaded
	for (let tab of tabs) {		
		onOpen(tab);
		onReady(tab);
	}
	// attach listener for new tab opens
	tabs.on('open', onOpen);	
};

/** Extension is unloaded. */
exports.onUnload = function (reason) {
	tabs.removeListener('open', onOpen);

	for (let tab of tabs) {
		tab.removeListener("ready", onReady);
		tab.removeListener("activate", onActivate);
		tab.removeListener("deactivate", onDeactivate);
		tab.removeListener("close", onClose);		
		onClose(tab);
  	}
};


// PageMod to add the mechanism for monitoring the application-level QoS of video streaming services.
var data = require("sdk/self").data;
pageMod.PageMod({
	include: /^http[s]*\:\/\/.*youtube.com\/.*/,
	//contentStyleFile: [],
	contentScriptFile: [
//		data.url("../video/jquery/jquery-1.9.1.min.js"),
//		data.url("../video/jquery/jquery-ui-1.10.3.min.js"),
		data.url("../video/Utils/Utils.js"),
//		data.url("../video/Log/Logger.js"),
		data.url("../video/Log/DatasetLogger.js"),
		data.url("../video/Dataset/Dataset.js"),
		data.url("../video/Dataset/VideoSession.js"),
		data.url("../video/Dataset/BufferingEvent.js"),
		data.url("../video/Dataset/PauseEvent.js"),
		data.url("../video/Dataset/SeekEvent.js"),
		data.url("../video/Dataset/OffScreenEvent.js"),
		data.url("../video/Dataset/VideoResolution.js"),
		data.url("../video/Dataset/PlayerSize.js"),
		data.url("../video/Dataset/VideoPlaybackQualitySample.js"),
		data.url("../video/Dataset/BufferedPlayTimeSample.js"),
		data.url("../video/Dataset/VideoSessionSummary.js"),
		data.url("../video/VideoMonitor/VideoMonitor.js"),
		data.url("../video/main.js"),
	],
	contentScript: '',
	contentScriptWhen: 'start', // Important! "start" is required! // start, ready, end
	onAttach: function (worker) {
		// request and handle pageload stats
		worker.port.on("upload", function (obj) {
			// upload with hostview
			upload.sendjson(obj);

			// For debugging: Draw a red border to the body elemens.
			//require("sdk/tabs").activeTab.attach({
			//	contentScript: 'alert("index.js: Uploading"); document.body.style.border = "5px solid red";'
			//});
		});
	}
});