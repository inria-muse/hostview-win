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
const Request = require("sdk/request").Request;
const { setTimeout, clearTimeout } = require("sdk/timers");

const HOSTVIEW_URL = 'http://localhost:40123/';
const MAX_RETRY = 2;

var send = function(req, retry) {
	console.log("send retry " + retry, req);
	Request({
		url : req.url,
		content : req.content,
		contentType : req.contentType,
		onComplete : function (res) {
			// hostview had trouble processing the request - try again few times
			console.log("send onComplete status="+res.status);
			if (res.status>0 && res.status !== 200 && retry < MAX_RETRY) {
				setTimeout(function() {
					send(req, retry + 1); // try again
				}, 5000);
			} // no connection - hostview not running
		}
	}).post();	
}

/** Send location update to local hostview client. */
var loc = exports.sendlocation = function(hostname) {
	var cstr = "browser=firefox&location=" + encodeURIComponent(hostname);
	send({
		url : HOSTVIEW_URL + 'locupdate',
		content : cstr,
		contentType : "application/x-www-form-urlencoded"
	}, 0);
};


/** Send json object to local hostview client. */
var jobj = exports.sendjson = function(jsonobj) {
	send({
		url : HOSTVIEW_URL + 'upload',
  		content : JSON.stringify(jsonobj),
  		contentType : 'application/json',
	}, 0);
};