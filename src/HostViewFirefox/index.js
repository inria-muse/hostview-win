/*
   HostView AddOn

   Copyright (C) 2015 Inria Paris-Roquencourt 

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
var dnsservice = Cc["@mozilla.org/network/dns-service;1"].createInstance(Ci.nsIDNSService);

var self = require('sdk/self');
var tabs = require("sdk/tabs");
const pageMod = require("sdk/page-mod");
var URL = require("sdk/url").URL;

var upload = require("./lib/upload.js");

// Caches for pages and tabs
var pages = {};
var tabcache = {};

/* Cleanup req params from the URL. */
var stripUrl = function(url) {
	var u = URL(url);
	var obj = {
		baseurl : u.protocol  + (u.hostname ? '//' + u.hostname : '')  + (u.port ? ':' + u.port : ''),
        extension : (u.pathname.lastIndexOf('.') > 0 ? u.pathname.substr(u.pathname.lastIndexOf('.')+1) : undefined),
		hostname : u.hostname,
		ip : undefined,
		port : (u.port ? u.port : (u.protocol.indexOf('https')>=0 ? 443 : 80))
	};

	try {
    	// this should be instant as the IP is cached by the browser        
		var flags = Ci.nsIDNSService.RESOLVE_CANONICAL_NAME;
        var r = dnsservice.resolve(obj.hostname, flags);
        if (r.hasMore()) {
            obj.ip = dnsservice.resolve(obj.hostname, flags).getNextAddrAsString();
        }
    } catch (e) {
    }	
	return obj;
};

// tab is active (current url goes foreground)
function onActivate(tab) {
	console.log('active ' + tab.url);
	var p = pages[tab.id+'_'+tab.url];
	if (p) {
		p.events.push({ e : 'foreground', ts : new Date().getTime() });
		// notify hostview
		upload.sendtohostview(p.url.hostname, tab.title);
	}
}

// tab is deactive (current url goes background)
function onDeactivate(tab) {
	console.log('deactive ' + tab.url);
	var p = pages[tab.id+'_'+tab.url];
	if (p) {
		p.events.push({ e : 'background', ts : new Date().getTime() });	
	}
}

// page loaded
function onReady(tab) {
	console.log('ready ' + tab.url);

	if (tabcache[tab.id] !== tab.url) {
		// navigated to a new page -- store and remove the previous page
		var p = pages[tab.id+'_'+tabcache[tab.id]];
		if (p) {
			// deactivate
			p.events.push({ e : 'background', ts : new Date().getTime() });	
			upload.add(p);
			// close
			delete pages[tab.id+'_'+tabcache[tab.id]];
		}
	}

	tabcache[tab.id] = tab.url;

	if (tab.url.indexOf('http')>=0) {
		// track only http(s) url
		console.log('ready and tracking ' + tab.url);
		var ts = new Date();
        var tzinfo = ts.toString().match(/([-\+][0-9]+)\s\(([A-Za-z\s].*)\)/);
        if (!tzinfo)
            tzinfo = ts.toString().match(/([-\+][0-9]+)/);

		var	p = {
			url : stripUrl(tab.url),
            ts : ts.getTime(), // UTC timestamp
            timezoneoffset : ts.getTimezoneOffset(),
            timezonename : (tzinfo && tzinfo.length > 1 ? tzinfo[2] : null),
			events : [{ e : 'foreground', ts : ts.getTime() }],
			objects : []
		}

		pages[tab.id+'_'+tab.url] = p;	

		// notify hostview
		upload.sendtohostview(p.url.hostname, tab.title);		
	}
}

// tab is closed (upload current url data)
function onClose(tab) {
	console.log('close ' + tab.url);
	if (tabcache[tab.id]) {
		var p = pages[tab.id+'_'+tab.url];
		if (p) {
			// may be a duplicate background event
			p.events.push({ e : 'background', ts : new Date().getTime() });	
			upload.add(p);
			delete pages[tab.id+'_'+tab.url];
		}
		delete tabcache[tab.id];
	}
}

function onOpen(tab) {
	console.log('open ' + tab.url);
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
    contentScript: 'self.port.on("hostview", function() { setTimeout(function() { self.port.emit("hostview", window.performance.getEntries()); }, 1000); });',
	contentScriptWhen: 'end',
	onAttach: function(worker) {
		worker.port.on("hostview", function(entries) {
			var p = pages[worker.tab.id+'_'+worker.tab.url];
			if (!p) return;
			for (var i = 0; i < entries.length; i++) {
				let e = entries[i];
				p.objects.push(stripUrl(e.name));
			}
		});
		worker.port.emit("hostview");
	}
});

/** Extension is loaded. */
exports.main = function (options, callbacks) {
	console.log('loading... ');

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
	console.log('unloading... ');

	tabs.removeListener('open', onOpen);

	for (let tab of tabs) {
		tab.removeListener("ready", onReady);
		tab.removeListener("activate", onActivate);
		tab.removeListener("deactivate", onDeactivate);
		tab.removeListener("close", onClose);		
		onClose(tab);
  	}

  	// flush the upload queue
  	upload.flush();
};