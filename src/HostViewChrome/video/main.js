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
 * Configuration for the mechanism that monitors the application-level QoS of video streaming services.
 */
const VIDEO_MON_CONF= {
	//
	// "in-dataset": logs in JSON dataset sent to the server
	// "in-page": logs in a div element, inside the web page
	LOGGER: "in-dataset",
	//
	// Sampling period (in ms) for some properties of the video element.
	POLLER_T: 1000,
    //
    // This function can be implemented to handle the dataset uploading process.
	/** @param jsonString {String} String containing a JSON-serialized Dataset object. */
    CUSTOM_UPLOAD_FUNC: function (jsonString) {
        Log.d("main.js", "CUSTOM_UPLOAD_FUNC() sending to HostView.");
        self.port.emit("upload", jsonString);
    },
    //
    // Boolean. If set to true, the VIDEO_MON_CONF.CUSTOM_UPLOAD_FUNC(...) function will be called, instead of the
    // default Dataset.postToServer(...) method.
    USE_CUSTOM_UPLOAD_FUNC: true,
	//
	// The URL where the JSON dataset will be POSTed, if USE_CUSTOM_UPLAD_FUNC is false.
	SRV_POST_URL: "http://192.168.56.101/get_data.php",
};


/**
 * Short-named, global scope variable to write logs.
 * @type {Logger|DatasetLogger}
 *
 * Interface:
 *  - Log.v(tag, message)
 *  - Log.d(tag, message)
 *  - Log.i(tag, message)
 *  - Log.w(tag, message)
 *  - Log.e(tag, message)
 */
var Log = null;
if (VIDEO_MON_CONF.LOGGER == "in-dataset") {
    Log = new DatasetLogger();
} else {
    Log = new Logger();
}


try {
    var myPerformance = new MyPerformance();
    var videoMonitor = new VideoMonitor();

    videoMonitor.watch('playerState', function (id, oldval, newval) {
        Log.i("main.js", 'videoMonitor.' + id + ' changed from ' + oldval + ' to ' + newval);
        return newval;
    });

    videoMonitor.watch('isBuffering', function (id, oldval, newval) {
        Log.i("main.js", 'videoMonitor.' + id + ' changed from ' + oldval + ' to ' + newval);
        return newval;
    });

    $(document).ready((function () {
        Log.v("main.js", "document ready");
        //Log.v("main.js", "new version 2");
        myPerformance.myDocumentReady = new Date().getTime();
    }).bind(this));

    $(document).on('webkitfullscreenchange mozfullscreenchange fullscreenchange MSFullscreenChange', function () {
        Log.i("main.js", "Toggle full screen mode");
        videoMonitor.toggleFullScreen();
    });

    window.onbeforeunload = function (e) {
        videoMonitor.onBeforePageUnload(e);
    };
} catch (err) {
    if (typeof Logger !== 'undefined') {
        if (Log instanceof Logger) {
            // Log is of type Logger. Just calling Log.e(...). This will be printed inside the webpage.
            Log.e("main.js", "Error: " + err);
        } else {
            // Log is not of type Logger, but Logger.js is included. Creating a new Logger and using it.
            var backupLog = new Logger();
            backupLog.e("main.js", "Error: " + err);
        }
    } else {
        // Logger is undefined. Showing the error as an alert.
        //window.alert("Error: " + err);
    }
}