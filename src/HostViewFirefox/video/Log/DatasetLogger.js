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

/**
 * Keeps logs in an Array-type property. Before the Dataset is POSTed to the server, the VideoMonitor puts the cotents
 * of DatasetLogger#logArray in the dataset.
 *
 * @constructor
 */
function DatasetLogger() {
	DatasetLogger.object = this;

	/** @type {Array} Stores the log lines in memory, to be included in the Dataset object.*/
	this.logArray= new Array();

	this.d("Logger", "Logger initialized.");
}

/**
 * Removes and returns all contents from the DatasetLogger#logArray property.
 * @returns {Array|*}
 */
DatasetLogger.prototype.clearLogs= function() {
	var logs= this.logArray;
	this.logArray= new Array();
	return logs;
}

/**
 * 'Private' method.
 * @param level
 * @param tag
 * @param message
 * @private
 */
DatasetLogger.prototype._log= function(level, tag, message) {
	var t= Date.now();
	if (tag==null)
		tag="";
	var tagSpaces= "                ";
	tag+= tagSpaces.substr(0, (16-tag.length) );
	//var str= t+"\t"+tag+""+message;
	var str= t+": "+level+'/'+tag+": "+message;
	this.logArray.push(str);
}

DatasetLogger.prototype.v=function(tag, message) {
	this._log("V", tag, message);
}

DatasetLogger.prototype.d=function(tag, message) {
	this._log("D", tag, message);
}

DatasetLogger.prototype.i=function(tag, message) {
	this._log("I", tag, message);
}

DatasetLogger.prototype.w=function(tag, message) {
	this._log("W", tag, message);
}

DatasetLogger.prototype.e=function(tag, message) {
	this._log("E", tag, message);
}

DatasetLogger.object= null;