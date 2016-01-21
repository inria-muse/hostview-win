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

var HOSTVIEW_URL = 'http://localhost:40123';
var MAX_RETRY = 2;

/* Upload helper. */
function send(req, retry) {
    console.log(req,retry);

    var http = new XMLHttpRequest();   
    http.open("POST", req.url, true);
    http.setRequestHeader("Content-type", req.contentType);
    http.onreadystatechange = function() {
    	if (http.readyState === 4 ) {
        	if (http.status && http.status !== 200 && retry < MAX_RETRY) {
            	// hostview had trouble processing the request - try again few times
            	setTimeout(function() {
               		send(req, retry + 1); // try again
            	}, 5000);
	        } // no connection == hostview not running => just forget the data
      	}
    }

    try {
		http.send(req.content);
  	} catch (e) {}
}

/** Send activte location updates to local hostview client. */
function sendlocation(hostname) {
   var cstr = "browser=chrome&location=" + encodeURIComponent(hostname);
   send({
      url : HOSTVIEW_URL + '/locupdate',
      content : cstr,
      contentType : "application/x-www-form-urlencoded"
   }, 0);
};


/** Send json object to local hostview client. */
function sendjson(jsonobj) {
   send({
      url : HOSTVIEW_URL + '/upload',
      content : JSON.stringify(jsonobj),
      contentType : 'application/json',
   }, 0);
};

function resolveIp(hostname) {
	return null;
}

function stripUrl(url) {
	return {
		hostname : url
	};
}

// active tabs cache
var tabs = {};

chrome.tabs.onActivated.addListener(function(activeInfo) {
	console.log("onActivated", activeInfo);
	if (tabs[activeInfo.tabId]) {
		sendlocation(tabs[activeInfo.tabId].hostname);
	}
});

chrome.tabs.onUpdated.addListener(function(tabId, changeInfo, updatedTab) {
	console.log("onUpdated " + tabId, changeInfo);

	if (changeInfo.url) {
		tabs[tabId] = stripUrl(changeInfo.url);
		sendlocation(tabs[tabId].hostname);
	}

	if (changeInfo.status && changeInfo.status === 'complete') {	

		// get the pageload stats
		chrome.tabs.sendMessage(tabId, "getplt", {}, function(obj) {
			console.log(obj);
			if (!obj || !obj.location)
				return;

			// cleanup all urls
			obj.location = stripUrl(obj.location);
			if (obj.location.hostname)
				obj.location.ip = resolveIp(obj.location.hostname);

			for (var i = 0; i < obj.restiming.length; i++) {
				var e = obj.restiming[i];
				e.name = stripUrl(e.name);
				if (e.name.hostname)
					e.ip = resolveIp(e.name.hostname);
			}

			// add some addon + browser metadata
			obj.addon_version = chrome.app.getDetails().version;
			obj.system_platform = chrome.runtime.platform;
			obj.system_architecture = chrome.runtime.platformArch;

			// upload with hostview
			sendjson(obj);
		});

		// TODO: video qoe stuff
	}
});

chrome.tabs.onRemoved.addListener(function(tabId, removeInfo) {
	console.log("onRemoved " + tabId, removeInfo);
	delete tabs[tabId];
});