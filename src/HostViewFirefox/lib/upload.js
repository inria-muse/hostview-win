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
var self = require('sdk/self');
var system = require('sdk/system');
var ss = require("sdk/simple-storage");
var Request = require("sdk/request").Request;
var { setTimeout, clearTimeout } = require("sdk/timers");

// Upload configs
const UPLOAD_SERVER_URL = 'https://ucnproject.uk/ucnupload';
const UPLOAD_DISABLE = false;
const UPLOAD_FREQ = 15*60*1000;

// Hostview end-point
const HOSTVIEW_URL = 'http://localhost:27015/';

// uploads timer
var timer = undefined;

var randomize = function(f) {
	// f +/- f/2
	return ((f-f/4) + f/2*Math.random());
}

/* Add new item. */
var add = exports.add = function(obj) {
	console.log('upload add (disabled='+UPLOAD_DISABLE+')', obj);
	if (!UPLOAD_DISABLE)
		ss.storage.queue.push(obj);
};

/* Upload items from the queue. */
var flush = exports.flush = function(cb) {	
	console.log('upload flush (disabled='+UPLOAD_DISABLE+')');
	if (UPLOAD_DISABLE) {
		if (cb) cb(false);
		return;
	}

	if (timer)
		clearTimeout(timer);
	timer = undefined;

	var l = ss.storage.queue.length;
	console.log('upload send ' + l + ' items');

	var done = function() {
		// success if queue was empty or something was uploaded
		var succ = (l==0 || ss.storage.queue.length < l);
		console.log('upload done, succ='+succ);
		timer = setTimeout(flush, randomize(UPLOAD_FREQ));
		if (cb) cb(succ);
	};

	var loop = function() {
		if (ss.storage.queue.length==0) {
			return done();
		}

		var item = ss.storage.queue.pop();

		// add some metadata
		item.uid = ss.storage.uuid;
		item.collection = 'firefoxaddon'; // the MongoDB collection where we store this data

		item.addonversion = self.version;
		item.browser = system.name;
		item.browserversion = system.version;
		item.browserplatform = system.platform;

		Request({
	  		url : UPLOAD_SERVER_URL,
	  		content : JSON.stringify(item),
	  		contentType : 'application/json',
			onComplete : function (res) {
				console.log('upload status ' + res.status);

				if (res.status !== 200) {
					ss.storage.queue.push(item);
					done();
				} else {
					loop();
				}
			}
		}).post();
	};

	loop();
};

/** Send object to local hostview client. */
var sendtohostview = exports.sendtohostview = function(hostname, title) {
	var cstr = "browser=firefox&location=" + encodeURIComponent(hostname);
	console.log('upload sendtohostview (winnt only)', cstr);

	if (system.platform !== 'winnt') {
		return;
	}

	Request({
		url : HOSTVIEW_URL,
		content : cstr,
		contentType : "application/x-www-form-urlencoded",
		onComplete : function (res) {
			console.log('sendtohostview status ' + res.status);
		}
	}).post();	
};

// the upload queue
if (!ss.storage.queue)
	ss.storage.queue = [];

// unique identifier of this installation
if (!ss.storage.uuid)
	ss.storage.uuid = Math.random().toString(36).substring(5);

// handle simple-storage quota limits
ss.on("OverQuota", function() {
	console.warn('upload queue over quota - trying to empty now');
	flush(function(ok) {
		if (!ok) { 
			// discarding older items
			console.warn('upload queue over quota - failed to empty, removing old items, size: ' + ss.storage.queue.length);
			ss.storage.queue = ss.storage.queue.slice(Math.round(ss.storage.queue.length*0.25), ss.storage.queue.length-1); 
			console.warn('upload queue over quota - removed old items, new size: ' + ss.storage.queue.length);
		}
	});
});

if (!UPLOAD_DISABLE)
	timer = setTimeout(flush, UPLOAD_FREQ);