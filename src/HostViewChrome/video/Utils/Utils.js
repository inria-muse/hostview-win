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
 * The Utils.js file contains helper classes, functions and variables.
 */

TrivialPerf= function() {
    Log.d("Utisl.js", "TrivialPerf object was created.")
}

TrivialPerf.prototype.getReport= function() {
    Log.d("Utisl.js", "TrivialPerf.getReport() was called.")
}

/** @type {MyPerformance} */
//MyPerformance.object= null;

MyPerformance= function() {
    MyPerformance.object=this;

    // my properties
    this.scriptInit= new Date().getTime();
    this.listenersRegistered= 0;
    this.videoEmptied= 0;
    this.myDocumentReady= 0;
    this.eventWaiting=0;
    this.eventPlaying=0;
    this.eventLoadstart=0;

    // Performance.timing properties
    var object= window.performance.timing;
    for (var property in object) {
        if (/*object.hasOwnProperty(property)*/ typeof object[property] !== "function") {
            MyPerformance.object[property]=0;
            //Log.e("", property);
            //Log.e("Utils.js", "event."+property+"= "+object[property]);
        }
    }

    //Log.i("", JSON.stringify(window.performance.timing));
    //Log.i("", JSON.stringify(MyPerformance.object));
}

MyPerformance.prototype.getReport= (function() {
    // Get values from the Performance.timing object;
    var object= window.performance.timing;
    for (var property in object) {
        if (/*object.hasOwnProperty(property)*/ typeof object[property] !== "function") {
            MyPerformance.object[property]=window.performance.timing[property];
            //Log.e("", ""+window.performance.timing[property]);
        }
    }

    // order the properties  in increasing timestamp value
    var propertyNames= new Array();
    var propertyValues= new Array();
    var object= MyPerformance.object;
    for (var property in object) {
        if (object.hasOwnProperty(property)) {
            propertyNames.push(property);
            propertyValues.push(object[property]);
        }
    }

    // See http://stackoverflow.com/questions/5427141/quickest-way-to-sort-a-js-array-based-on-another-arrays-values
    var A= propertyValues;
    var B= propertyNames;
    var all = [];
    for (var i = 0; i < B.length; i++) {
        all.push({ 'A': A[i], 'B': B[i] });
    }
    all.sort(function(a, b) {
        return a.A - b.A;
    });
    A = [];
    B = [];
    for (var i = 0; i < all.length; i++) {
        A.push(all[i].A);
        B.push(all[i].B);
    }
    propertyValues= A;
    propertyNames= B;

    // Output
    str= "Report:\n\t\t\t\t";
    var tabs;
    var spaces;
    prevT=0;
    for (var i = 0; i < propertyNames.length; i++) {
        spaces= "                              ".substr(0, 30- propertyNames[i].length);
        tabs= (propertyNames[i].length > 12) ? "\t" : "\t\t";
        if (prevT == 0) {
            str+= propertyNames[i]+":"+spaces+propertyValues[i]+"\n\t\t\t\t";
        } else {
            str+= propertyNames[i]+":"+spaces+"+"+(propertyValues[i]-prevT)+"\n\t\t\t\t";
        }
        prevT= propertyValues[i];
    }
    return str;
}).bind(MyPerformance.object);


/**
 * Singleton Performance
 */
/*Performance= {
    scriptInit: new Date().getTime(),
    connectionStarted: window.performance.timing.connectStart,
    domLoaded:0,
    listenersRegistered:0,
    videoEmptied:0,

    getReport: function() {
        t0= this.scriptInit;
        str= "Report:\n\t\t\t\t";
        str+="conStart:\t\t"+this.connectionStarted+"\n\t\t\t\t";
        str+="domLoaded:\t\t"+this.domLoaded+"\n\t\t\t\t";
        str+="scriptInit:\t\t"+(this.scriptInit)+"\n\t\t\t\t";
        str+="listenersRegistered:\t+"+(this.listenersRegistered-this.scriptInit)+"\n\t\t\t\t";
        str+="videoEmptied:\t\t+"+(this.videoEmptied-this.listenersRegistered)+"\n";
        return str;
    }
}*/

/**
 * Class Queue
 */

/** @constructor */
function Queue() {
    this._oldestIndex = 1;
    this._newestIndex = 1;
    this._storage = {};
}

/** @returns {number} The number of elements in the Queue */
Queue.prototype.size = function() {
    return this._newestIndex - this._oldestIndex;
};

/** @param data {object} An object to store in the Queue*/
Queue.prototype.enqueue = function(data) {
    this._storage[this._newestIndex] = data;
    this._newestIndex++;
};

/**  @returns {object} An object from the Queue */
Queue.prototype.dequeue = function() {
    var oldestIndex = this._oldestIndex,
        newestIndex = this._newestIndex,
        deletedData;
    if (oldestIndex !== newestIndex) {
        deletedData = this._storage[oldestIndex];
        delete this._storage[oldestIndex];
        this._oldestIndex++;
        return deletedData;
    }
};


/**
 * Class LogQueueItem
 */

/**
 * @constructor
 * @param level
 * @param tag
 * @param message
 * @param t
 */
LogQueueItem= function(level, tag, message, t) {
    this.t= t;
    this.level= level;
    this.tag= tag;
    this.message= message;

    this.getLogged= function() {
        Log._log(this.level, this.tag, "[Queued] "+this.message, this.t);
    }
}

/**
 * Logs all properties of an object at the Logger div.
 * @param object  {Object} The object whose properties will be logged.
 * @param showInherited {boolean} If true, the inherited properties will be logged, too.
 */
var logObjProperties= function(object, showInherited) {
    for (var property in object) {
        if (object.hasOwnProperty(property) || showInherited==true) {
        Log.e("Utils.js", "event."+property+"= "+object[property]);
        }
    }
}

/**
 * Returns whether the current browser tab is visible or not, dealing with the different browser-specific syntaxes.
 * See: http://stackoverflow.com/questions/19519535/detect-if-browser-tab-is-active-or-user-has-switched-away
 *
 * This function captures the following cases:
 *     1. The browser window becomes is minimized.
 *     2. This browser tab becomes non-active (the users clicks on another tab).
 *
 * Important! This function does NOT capture the following cases:
 *     1. Another window gets on top of the web browser, covering part on the entire (e.g., maximization) web
 *        browser window.
 *
 * @param c {Function} Optional. If given, the passed function c is registered for visibility change events.
 * @returns {Boolean} Returns the current visibility state.
 */
var isTabHidden = (function(){
    var stateKey, eventKey, keys = {
        hidden: "visibilitychange",
        webkitHidden: "webkitvisibilitychange",
        mozHidden: "mozvisibilitychange",
        msHidden: "msvisibilitychange"
    };
    for (stateKey in keys) {
        if (stateKey in document) {
            eventKey = keys[stateKey];
            break;
        }
    }
    return function(c) {
        if (c) document.addEventListener(eventKey, c);
        return document[stateKey];
    }
})();



Timer= function(timerTask, interval, context) {
    this.timerTask= timerTask;
    this.interval= interval;
    this._ref= null;
}
Timer.prototype.start= function() {
    this._ref= window.setInterval(function() {this.timerTask.bind(context);}, this.interval);
}
Timer.prototype.stop= function() {
    clearInterval(this._ref);
}
