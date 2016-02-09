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
 * Class Logger
 *
 * The Logger provides an in-document logging mechanism. This is used for development (you can see the logs together
 * with the streamed video) and simple demos.
 *
 * The Logger creates a div element and adds it in the DOM as a child of the body element. Then, log messages are
 * written in this div. Calling the Log methods is possible before the document is fully loaded. In this case, Log
 * method calls are stored in a Queue and are appended in the div as soon as the document is loaded.
 */


/*
 * Static properties
 */

/** @type {Logger} */
Logger.object= null;


/*
 * Static methods
 */
// None yet.


/**
 * @constructor
 */
function Logger() {
    Logger.object = this;

    // Initializing instance properties.

    /** @type {{width: number, height: number, left: number, top: number}} */
    //var divConf= {width:750, height:430, left:500, top:100}; // in px
    var divConf= {width:850, height:800, left:1450, top:200}; // in px

    /** @type {boolean} */
    this.divCreated=false;

    this.logDiv;
    this.logDivContent;

    this.logQueue= new Queue(); // Queue is defined in Utils.js

    /*this.init= function() {
        // Do nothing;
    }*/

    this._log= function(level, tag, message, t) {
        if (t== null)
            t= Date.now();
        if (tag==null)
            tag="";
        var tagSpaces= "                ";
        tag+= tagSpaces.substr(0, (16-tag.length) );
        //if (tag.length<=6)
        //    tag= tag+"\t";
        var levels= {"v":"#AAA", "d":"blue", "i":"green", "w":"orange", "e":"red"};
        //this.logDivContent.appendChild(document.createTextNode(tag+"\t"+message+"\n"));
        var emNode= document.createElement('span');
        emNode.style.color=levels[level];
        var textNode= document.createTextNode(t+"\t"+tag+""+message+"\n");
        emNode.appendChild(textNode);
        this.logDivContent.appendChild(emNode);
        this.logDivContent.scrollTop = this.logDivContent.scrollHeight;
    }

    this.log= function(level, tag, message) {
        if (this.divCreated) {
            this._log(level, tag, message);
        } else {
            this.logQueue.enqueue(new LogQueueItem(level,tag, message, new Date().getTime()));
            // LogQueueItem is defined in Utils.js
        }
    }

    this.v=function(tag, message) {
        this.log("v", tag, message);
    }

    this.d=function(tag, message) {
        this.log("d", tag, message);
    }

    this.i=function(tag, message) {
        this.log("i", tag, message);
    }

    this.w=function(tag, message) {
        this.log("w", tag, message);
    }

    this.e=function(tag, message) {
        this.log("e", tag, message);
    }

    this.println= function (text) {
        this.d("[No Tag]", text);
    }

    // This will be executed when the document is ready. It creates the div element and outputs the queued log messages.
    $(document).ready( (function() {
        // Define style for the logger div
        cssCode= " #logger {border:solid 1px #000; background:#FFF; position:absolute; width:"+divConf.width+"px; height:"+divConf.height+"px; padding:0px; margin:5px; ";
        cssCode+= " font:13px Arial; cursor:move; float:left } ";
        cssCode+= " #logDivH3 {display:inline-block; margin:0px; background:#000; color:#FFF; width:100%; height:20px;} ";
        cssCode+= " #logDivContent {width:100%; height:"+(divConf.height-20)+"px; display:block; overflow-y:scroll;} ";
        var styleEl= document.createElement("style");
        var styleText = document.createTextNode(cssCode);
        styleEl.appendChild(styleText);
        document.getElementsByTagName("head")[0].appendChild(styleEl);

        // Create the logger div
        this.logDiv = document.createElement('div');
        this.logDiv.setAttribute("id", "logger");
        this.logDiv.setAttribute("class", "ui-draggable ui-resizable");
        this.logDiv.style.zIndex=999;
        //this.logDiv.style.border= "1px solid black";
        document.body.insertBefore(this.logDiv, document.body.firstChild);
        var h3= document.createElement("h3");
        h3.setAttribute("id", "logDivH3");
        //h3.setAttribute("style", "");
        var t = document.createTextNode("Logger.js:");
        this.logDiv.appendChild(h3);
        h3.appendChild(t);
        this.logDivContent= document.createElement('pre');
        this.logDivContent.setAttribute("id", "logDivContent");
        this.logDiv.appendChild(this.logDivContent);

        // Make the logger div draggable and resizable
        //$(function() { $("#logger").draggable().resizable(); });
        $('#logger')
            .draggable({
                handle: "#logDivH3",
            })
            .resizable({
                handles: "se",
                start: function(e, ui) {
                    //alert('resizing started');
                },
                resize: function(e, ui) {
                },
                stop: function(e, ui) {
                    //Logger.object.w('Logger', 'Resizing stopped. h='+ui.size.height+"px.");
                    Logger.object.logDivContent.style.height= (ui.size.height-20)+"px";
                }
            })
            .css('cursor', 'default');
        this.logDiv.style.left= divConf.left+"px";
        this.logDiv.style.top= divConf.top+"px";

        var logQueueItem= null;
        while ( (logQueueItem= this.logQueue.dequeue()) != null) {
            Logger.object._log(logQueueItem.level, logQueueItem.tag, "[Queued] "+logQueueItem.message, logQueueItem.t);
        }
        this.divCreated=true;

        this.d("Logger", "Logger initialized.");
    }).bind(this) );


}