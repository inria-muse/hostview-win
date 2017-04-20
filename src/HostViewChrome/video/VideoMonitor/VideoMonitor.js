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

const VIDEO_ELEMENT_SEARCH_T = 100; // in ms;
const VIDEO_ELEMENT_SEARCH_MAX_DUR = 20000; // in ms;
const TIME_WIN_CLICK_QUALITY_CLICK_RES= 4500; // in ms;

/**
 * Class VideoMonitor
 *
 * See https://developer.mozilla.org/en/docs/Web/API/HTMLMediaElement
 * See http://www.w3.org/2010/05/video/mediaevents.html
 */


/**
 *
 * @constructor
 */
function VideoMonitor() {
	VideoMonitor.object = this;

	/** @type {Date} */
	this.creationTime = new Date();

	Log.d("VideoMonitor", "VideoMonitor initialized.");
	//Log.i("VideoMonitor", "domComplete=" + window.performance.timing.domComplete + ". Dt=" + (new Date - window.performance.timing.domComplete ) + ".");

	/**
	 * Stores an interval id returned by window.setInterval(...). The specific interval is executed with high frequency
	 * while the DOM is loading, to find the video element as soon as possible. Once the video element is found, or
	 * after a specific amount of time (whichever happens first), this interval is canceled.
	 * @type {number}
	 */
	this.videoElementSearcher = null;

	/** @type {number} Counts how many times the {@link VideoMonitor#searchVideoElement()} has been executed. */
	this.searchVideoElementCounter = 0;

	/**
	 * Stores an interval id returned by window.setInterval(...). The specific interval is polling some properties of
	 * the video elements that do not support event listeners.
	 * @type {number}
	 */
	this.poller = null;

	// Initializing instance properties.
	this.playerState = VideoMonitor.PlayerStates.Off;
	this.dataset = null;
	this.videoSession = null;
	this.bufferingEvent = null;
	this.isBuffering = false;
	this.hasStartupBuffCompleted = false;
	this.pauseEvent = null;
	this.videoResolution = null;
	this.seekEvent = null;
	this.offScreenEvent = null;
	this.playerSize = null;

	this.ytpSettingsButton = null;
	this.userClickedYtpSettingsRegistered= false;
	this.userClickedQualityBtnRegistered = false;
	this.whenUserSwitchedRes= 0;

	this.playerWidth = 0;
	this.playerWidthOld = 0;
	this.playerHeight = 0;
	this.playerHeightOld = 0;
	this.isOnFullScreen = false;
	this.isHidden = isTabHidden();

	this.vidQual_tvf = 0;
	this.vidQual_dvf = 0;
	this.vidQual_cvf = 0;
	this.vidQual_tfd = 0;


	/** @type {HTMLVideoElement} */
	this.videoElement = document.getElementsByTagName('video')[0];

	if (this.videoElement != null)
		Log.v("VideoMonitor", "Video element found at initial assignment.");

	/** @type {VideoPlaybackQuality} */
	this.vidQual = (this.videoElement != null) ? this.videoElement.getVideoPlaybackQuality() : null;

	this.areListeneersRegistered = false;
	if (this.videoElement != null && this.areListeneersRegistered == false) {
		this.registerMediaEventListeners();
	}
	//
	// Register mediaEventListeners.visibilityChange for events of tab/window visibility changes.
	//
	// When this browser tab is not active anymore, or the browser window is minimized, this function will be called.
	// Important! This function does NOT capture all cases.Please read the documentation of isTabHidden() in Utils.js.
	isTabHidden(this.mediaEvent_visibilityChange.bind(this));
	//
	// Start searching for the video element.
	// TODO: Remove the 10 sec constraint from searchVideoElement(), and instead check if the document.onload() has been
	// TODO: fired. (register for document.onload() or compare current timestamp with performance.timing.loadEventEnd).
	this.videoElementSearcher = setInterval((function () {
		try {
			VideoMonitor.object.searchVideoElement();
		} catch (err) {
			Log.e("VideoMonitor", "videoElementSearcher(): Error: " + err);
		}
	}), VIDEO_ELEMENT_SEARCH_T);

	/*// Register toggleFullScreen() for events
	 $(document).on('webkitfullscreenchange mozfullscreenchange fullscreenchange MSFullscreenChange', function() {
	 Log.i("VideoMonitor", "Toggle full screen mode");
	 this.toggleFullScreen();
	 });
	 // Register onBeforePageUnload() for events
	 window.onbeforeunload= function(e) {
	 this.onBeforePageUnload(e);
	 };*/

	$(document).ready(function () {
		Log.i("VideoMonitor", "window.onload() event. Clearing this.videoElementSearcher");
		clearInterval(this.videoElementSearcher);
	});
}

/**
 *
 * @param state {VideoMonitor.PlayerStates}
 */
VideoMonitor.prototype.setPlayerState = function (state) {
	//Log.i("VideoMonitor", "playerState: "+VideoMonitor.object.playerState+"->"+state);
	VideoMonitor.object.playerState = state;
}

/**
 * Registers the {@link VideoMonitor#mediaEventListeners} functions for events.
 */
VideoMonitor.prototype.registerMediaEventListeners = function () {
	if (this.videoElement == null)
		Log.e("VideoMonitor", "Trying to register listeners but videoElement is null.");
	var key;
	for (var i = 0; i < VideoMonitor.mediaEventNames.length; i++) {
		key = VideoMonitor.mediaEventNames[i];
		this.videoElement.addEventListener(key, this["mediaEvent_"+key].bind(this), false);
		this.areListeneersRegistered = true;
	}
	myPerformance.listenersRegistered = new Date().getTime();
	//this.pollVideoProps.start();
}

/**
 * This method is called periodically, with high frequency, while the DOM is loading, by the
 * {@link VideoMonitor#videoElementSearcher}.
 */
VideoMonitor.prototype.searchVideoElement = function () {
	this.searchVideoElementCounter++;
	// Try getting a reference to the HTMLVideoElement object and its VideoPlaybackQuality object.
	if (this.videoElement == null) {
		this.videoElement = document.getElementsByTagName('video')[0];
		this.vidQual = (this.videoElement != null) ? this.videoElement.getVideoPlaybackQuality() : null;
	}
	//
	// If HTMLVideoElement found, register the this.mediaEventListeners functions for events.
	if (this.videoElement != null && this.areListeneersRegistered == false) {
		this.registerMediaEventListeners();
	} else {
		//Log.v("VideoMonitor", "Listeners already registered (exec #"+invalCnt+").");
	}
	//
	// Stop searching for the HTMLVideoElement when finding it or failing for 10 seconds
	if (this.videoElement != null || (new Date() - this.creationTime) > VIDEO_ELEMENT_SEARCH_MAX_DUR) {
		if (this.videoElement != null) {
			Log.v("VideoMonitor", "Video element found. Clearing interval on exec #" + this.searchVideoElementCounter + ".");
		} else {
			Log.v("VideoMonitor", "Video element not found. Clearing interval on exec #" + this.searchVideoElementCounter + ".");
		}
		clearInterval(this.videoElementSearcher);
	}
}

VideoMonitor.prototype.resetVariables = function () {
	this.playerState = VideoMonitor.PlayerStates.Off;
	// Create a summary object.
	/*var summary = new VideoSessionSummary(this.dataset); // TODO: Buffering-related values are wrong.
	this.dataset.VideoSessionSummaries.push(summary);
	// If the DatasetLogger is in use, add the logs in the Dataset.
	if (typeof DatasetLogger !== 'undefined') {
		if (Log instanceof DatasetLogger) {
			Log.d("VideoMonitor", "resetVariables(): DatasetLogger is used. Adding logs in the Dataset.");
			this.dataset.Logs= Log.clearLogs();
		} else {
			Log.d("VideoMonitor", "resetVariables(): Log is not of type DatasetLogger. No logs are added in the Dataset.");
		}
	} else {
		Log.d("VideoMonitor", "resetVariables(): DatasetLogger is undefined. No logs are added in the Dataset.");
	}*/
	// Serialize the dataset in JSON format and POST it to the server.
	Dataset.send(this.dataset);
	// Reset variable values
	this.dataset = null; // TODO: Reference becomes null, but Dataset.send() can continue working, right?
	this.videoSession = null;
	this.bufferingEvent = null;
	this.isBuffering = false;
	this.hasStartupBuffCompleted = false;
	this.pauseEvent = null;
	this.videoResolution = null;
	this.seekEvent = null;
	this.offScreenEvent = null;
	this.playerSize = null;

	this.ytpSettingsButton = null;
	this.userClickedYtpSettingsRegistered= false;
	this.userClickedQualityBtnRegistered = false;
	this.whenUserSwitchedRes= 0;

	this.playerWidth = 0;
	this.playerWidthOld = 0;
	this.playerHeight = 0;
	this.playerHeightOld = 0;
}

VideoMonitor.prototype.startPolling = function () {
	Log.d("VideoMonitor", "startPolling(): Setting an interval for polling.");
	this.poller = setInterval((function () {
		try {
			var t = new Date().getTime();
			VideoMonitor.object.pollPlayerSize(t);
			VideoMonitor.object.pollVideoQuality(t);
			VideoMonitor.object.pollBufferedVideoDur(t);
		} catch (err) {
			Log.e("VideoMonitor", "poller(): Error: " + err);
		}
	}), VIDEO_MON_CONF.POLLER_T);
}

VideoMonitor.prototype.stopPolling = function () {
	Log.d("VideoMonitor", "stopPolling(): Cleaning the polling interval.");
	clearInterval(this.poller);
	this.poller = null;
}

VideoMonitor.prototype.pollPlayerSize = function (t) {
	try {
		//Log.i("VideoMonitor", "pollVideoProps(): this= "+this);
		this.playerWidth = this.videoElement.style.width; // .getAttribute('width');
		this.playerHeight = this.videoElement.style.height; // .getAttribute('height');
		//Log.v("VideoMonitor", "pollVideoProps(): video player size= " + VideoMonitor.object.playerWidth + "x" + VideoMonitor.object.playerHeight);
		var cond = (this.playerWidth != this.playerWidthOld || this.playerHeight != this.playerHeightOld);
		if (cond) {
			var e = {
				'type': "videoPlayerResize",
				'timeStamp': (t * 1000), // *1000 for consistency with the rest of events.
				'playerWidth': VideoMonitor.object.playerWidth,
				'playerHeight': VideoMonitor.object.playerHeight,
				'isOnFullScreen': VideoMonitor.object.isOnFullScreen
			};
			this.mediaEvent_videoPlayerResize(e);
			this.playerWidthOld = this.playerWidth;
			this.playerHeightOld = this.playerHeight;
		}
	} catch (err) {
		Log.e("VideoMonitor", "pollVideoProps(): Error: " + err);
	}
}


VideoMonitor.prototype.pollVideoQuality = function (t) {
	try {
		var ve = this.videoElement;
		var vq = this.videoElement.getVideoPlaybackQuality();
		var vpqs = new VideoPlaybackQualitySample(t,
			// Supported since: {Chrome v23, Firefox v25(partially) & v42(Only Win 8+), IE v11, Opera v15, Safari v8}
			vq.totalVideoFrames, vq.droppedVideoFrames, vq.corruptedVideoFrames, vq.totalFrameDelay,
			// Supported only on Firefox v5 and later
			ve.mozParsedFrames, ve.mozDecodedFrames, ve.mozPresentedFrames, ve.mozPaintedFrames, ve.mozFrameDelay,
			this.videoSession);
		this.dataset.VideoPlaybackQualitySamples.push(vpqs);
	} catch (err) {
		Log.e("VideoMonitor", "pollVideoQuality(): Error: " + err);
	}
}

VideoMonitor.prototype.pollBufferedVideoDur = function (t) {
	try {
		var currPlayTime = this.videoElement.currentTime;
		var max = 0;
		var bufferedStart = 0;
		var bufferedEnd = 0;

		max = this.videoElement.buffered.length;
		for (var i = 0; i < max; i++) {
			bufferedStart = this.videoElement.buffered.start(i);
			bufferedEnd = this.videoElement.buffered.end(i);
			if (bufferedStart <= currPlayTime && currPlayTime <= bufferedEnd)
				break;
		}
		var bufferedMinusCurrent = Math.max(0, (bufferedEnd - currPlayTime));
		//Log.v("VideoMonitor", "pollBufferedVideoDur(): availableInBuffer= "+availableInBuffer+" sec.");
		var bufferedPlayTimeSample = new BufferedPlayTimeSample(t, currPlayTime, bufferedMinusCurrent, this.videoSession);
		this.dataset.BufferedPlayTimeSamples.push(bufferedPlayTimeSample);
	} catch (err) {
		Log.e("VideoMonitor", "pollBufferedVideoDur(): Error: " + err);
	}
}

/**
 * This is called by main.js
 */
VideoMonitor.prototype.onBeforePageUnload = function (e) {
	try {
		Log.i("VideoMonitor", "onBeforePageUnload(): Page is unloading! Calling abort() ");
		if (this.videoSession != null) {
			VideoMonitor.object.mediaEvent_abort(e);
		}
	} catch (err) {
		Log.e("VideoMonitor", "onBeforePageUnload(): Error: " + err);
	}
};

/**
 * This is called by main.js
 */
VideoMonitor.prototype.toggleFullScreen = function () {
	var t = new Date().getTime();
	if (this.isOnFullScreen == false)
		this.isOnFullScreen = true;
	else
		this.isOnFullScreen = false;

	/*if (this.isOnFullScreen==true) {
	 videoMonitor.mediaEventListeners.videoPlayerResize({
	 'type': "videoPlayerResize",
	 'timeStamp': t,
	 'playerWidth': screen.width,
	 'playerHeight': screen.height,
	 'isOnFullScreen': true,
	 });
	 } else {
	 videoMonitor.mediaEventListeners.videoPlayerResize({
	 'type': "videoPlayerResize",
	 'timeStamp': t,
	 'playerWidth': this.playerWidth,
	 'playerHeight': this.playerHeight,
	 'isOnFullScreen': false
	 });
	 }*/
}

VideoMonitor.prototype.parseServiceTypeAndProps = function () {
	try {
		//
		// Identification of serviceType
		//
		// There is red dot button at the control bar of the HTML5 video player. This button is shown only for
		// Live videos and is identified by its class="ytp-live-badge ytp-button" attribute.
		//
		// <button class="ytp-live-badge ytp-button" disabled="">
		//
		// The "disabled" attribute takes different values for Live and VoD videos.
		//    - For VoD videos: disabled= "true" | null.  The null value appears on VoD videos playing after
		//      a Live, without refreshing tha page
		//    - For Live videos: disabled=""
		//
		var buttons = document.getElementsByTagName("button");
		var isLive = false;
		for (var i = 0; i < buttons.length; i++) {
			if (buttons[i].getAttribute("class").indexOf("ytp-live-badge") > -1) {
				// Red dot button found
				//var isLiveLog= (buttons[i].getAttribute("disabled") != null) ? ""+buttons[i].getAttribute("disabled") : "[null]";
				//Log.e("isLive?", "disabled="+isLiveLog);
				if (buttons[i].getAttribute("disabled") == "") { // or buttons[i].style.visibility == "visible"
					// live badge is shown
					Log.i("VideoMonitor", "type: Live");
					isLive = true;
					this.videoSession.service_type = VideoSession.ServiceTypes.live.value;
				} else {
					// live badge is not shown
					Log.i("VideoMonitor", "type: VoD");
					isLive = false;
				}
			}
		}
		//
		// Other properties of the VideoSession object:
		this.videoSession.window_location = window.location.href;
		this.videoSession.current_src = this.videoElement.currentSrc;
		// Try finding the title
		try {
			var title = document.getElementById("watch-headline-title").firstElementChild.firstElementChild.textContent.trim();
			this.videoSession.title = title;
			//Log.i("VideoMonitor", "loadstart(): title= "+title);
		} catch (err1) {
			Log.w("VideoMonitor", "parseServiceTypeAndProps(): title not found. Error: " + err1);
		}
		//Log.i("VideoMonitor", "loadstart(): initialTime= "+this.videoElement.initialTime); // Non-standard. Does not work.

		// Find the menu button for changing the video resolution manually, and add an onClick event listener.
		if (this.userClickedYtpSettingsRegistered == false) {
			this.ytpSettingsButton = null;
			for (var i = 0; i < buttons.length; i++) {
				if (buttons[i].getAttribute("title") == "Settings") {
					this.ytpSettingsButton= buttons[i];
					this.ytpSettingsButton.addEventListener("click", this.mediaEvent_userClickedYtpSettingsBtn.bind(this));
					this.userClickedYtpSettingsRegistered = true;
					Log.v("VideoMonitor", "parseServiceTypeAndProps(): ytp-right-controls.Settings found! userClickedYtpSettingsBtn() registered.");
					break;
				}
			}
		}
	} catch (err) {
		Log.e("VideoMonitor", "parseServiceTypeAndProps(): Error: "+err);
	}
}

// TODO: delete
VideoMonitor.prototype.testNewFuncs = function () {
	try {
		Log.i("VideoMonitor", "testNewFuncs(): this.vidQual= " + JSON.stringify(this.vidQual));

		var max = this.videoElement.played.length;
		var str_played = "";
		for (var i = 0; i < max; i++) {
			str_played += " " + i + ": [" + this.videoElement.played.start(i) + "--" + this.videoElement.played.end(i) + "]";
		}
		var str_buffered = "";
		for (var i = 0; i < max; i++) {
			str_buffered += " " + i + ": [" + this.videoElement.buffered.start(i) + "--" + this.videoElement.buffered.end(i) + "]";
		}
		Log.i("VideoMonitor", "testNewFuncs(): this.videoElement.currentTime= " + this.videoElement.currentTime);
		Log.i("VideoMonitor", "testNewFuncs(): this.videoElement.played= " + str_played);
		Log.i("VideoMonitor", "testNewFuncs(): this.videoElement.buffered = " + str_buffered);


		Log.i("VideoMonitor", "testNewFuncs(): this.videoElement.srcObject = " + JSON.stringify(this.videoElement.srcObject));
		Log.i("VideoMonitor", "testNewFuncs(): this.videoElement.videoTracks = " + JSON.stringify(this.videoElement.videoTracks));
	} catch (err) {
		Log.e("VideoMonitor", "testNewFuncs(): Error: " + err);
	}
}

/*
 * During the loading process of an audio/video, the following events occur, in this order:
 *   1. loadstart
 *   2. durationchange
 *   3. loadedmetadata
 *   4. loadeddata
 *   5. progress
 *   6. canplay
 *   7. canplaythrough
 *
 * The loadstart event occurs when the browser starts looking for the specified audio/video. This is when the loading process starts.
 * The canplay event occurs when the browser can start playing the specified audio/video (when it has buffered enough to begin).
 *
 * See: http://www.w3schools.com/tags/ref_av_dom.asp
 *
 * When the user browses videos in YouTube, the following event sequence has been observed:
 * emptied
 * ratechange
 * play
 * waiting
 * 1. loadstart
 * 2. durationchange
 * resize
 * 3. loadedmetadata
 * 4. loadeddata
 * playing
 */

VideoMonitor.prototype.mediaEvent_play = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		//
		if (this.hasStartupBuffCompleted == false) {
			Log.d("VideoMonitor", "hasStartupBuffCompleted is false. Skipping.");
			return;
		}
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "play(): Error: this.videoSession == null");

		if (this.playerState == VideoMonitor.PlayerStates.Paused) {
			if (this.pauseEvent == null)
				Log.e("VideoMonitor", "play(): Error: this.pauseEvent is null");
			this.pauseEvent.end(event.timeStamp / 1000);
			this.dataset.PauseEvents.push(this.pauseEvent);
			this.pauseEvent = null;
		}
		// TODO: may need to return to Buffering state
		this.setPlayerState(VideoMonitor.PlayerStates.Playing);
	} catch (err) {
		Log.e("VideoMonitor", "play(): " + err);
	}
}

VideoMonitor.prototype.mediaEvent_loadstart = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		myPerformance.eventLoadstart = new Date().getTime();
		// Case: User clicked on another video. End the previous session with end_reason="abort".
		if (this.videoSession != null) {
			Log.i("VideoMonitor", "loadstart(): User clicked on another video. Calling abort().");
			VideoMonitor.object.mediaEvent_abort(event);
		}
		// Checks
		if (this.videoSession != null)
			Log.e("VideoMonitor", "loadstart(): Error: this.videoSession != null");
		if (this.bufferingEvent != null)
			Log.e("VideoMonitor", "loadstart(): Error: this.bufferingEvent != null");
		if (this.seekEvent != null)
			Log.e("VideoMonitor", "loadstart(): Error: this.seekEvent != null");
		if (this.pauseEvent != null)
			Log.e("VideoMonitor", "loadstart(): Error: this.seekEvent != null");
		if (this.offScreenEvent != null)
			Log.e("VideoMonitor", "loadstart(): Error: this.seekEvent != null");
		// Create a new Dataset object for this session.
		this.dataset = new Dataset();
		this.videoSession = new VideoSession(event.timeStamp / 1000);
		this.bufferingEvent = new BufferingEvent(event.timeStamp / 1000, BufferingEvent.types.startup.value, this.videoSession);
		// TODO: push in Dataset at the end?
		this.dataset.VideoSessions.push(this.videoSession);
		this.dataset.BufferingEvents.push(this.bufferingEvent);
		this.hasStartupBuffCompleted = false;

		// Create a PlayerSize object, if not already done by "pollPlayerSize() --event--> videoPlayerResize()"
		this.pollPlayerSize(event.timeStamp / 1000);

		// If this browser tab was loaded on the background, start an OffScreenEvent object.
		this.isHidden = isTabHidden();
		Log.d("VideoMonitor", "loadstart(): this.isHidden=" + this.isHidden);
		if (this.isHidden == true) {
			// Checks
			//if (this.offScreenEvent != null)
			//    Log.e("VideoMonitor", "loadstart(): Error: this.offScreenEvent != null");
			this.offScreenEvent = new OffScreenEvent(event.timeStamp / 1000, this.videoSession);
		}
		this.startPolling();
		this.isBuffering = true;
		this.setPlayerState(VideoMonitor.PlayerStates.Buffering);
	} catch (err) {
		Log.e("VideoMonitor", "loadstart(): " + err);
	}
}

/**
 * The durationchange event is fired when the duration attribute has been updated.
 * @param event
 */
VideoMonitor.prototype.mediaEvent_durationchange = function (event) {
	try {
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "durationchange(): Error: this.videoSession == null");
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type + " duration=" + this.videoElement.duration + ".");
		this.videoSession.duration = this.videoElement.duration;
		//Log.e("", "event="+event.toString());
		//logObjProperties(event, true);
	} catch (err) {
		Log.e("VideoMonitor", err);
	}
}

VideoMonitor.prototype.mediaEvent_loadedmetadata = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
}

VideoMonitor.prototype.mediaEvent_loadeddata = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
}

/**
 * The progress event is fired to indicate that an operation is in progress.
 * See https://developer.mozilla.org/en-US/docs/Web/Events/progress
 * @param event
 */
VideoMonitor.prototype.mediaEvent_progress = function (event) {
	//Log.d("VideoMonitor", "Event fired. event.type="+event.type);
}

/**
 * The canplay event is fired when the user agent can play the media, but estimates that not enough data has
 * been loaded to play the media up to its end without having to stop for further buffering of content.
 * @param event
 */
VideoMonitor.prototype.mediaEvent_canplay = function (event) {
	//Log.d("VideoMonitor", "Event fired. event.type="+event.type);
}

VideoMonitor.prototype.mediaEvent_canplaythrough = function (event) {
	//Log.d("VideoMonitor", "Event fired. event.type="+event.type);
}

/**
 * Fires when the current playlist is empty.
 * @param event
 */
VideoMonitor.prototype.mediaEvent_emptied = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
	myPerformance.videoEmptied = Math.round(event.timeStamp / 1000);
	//Log.i("VideoMonitor", myPerformance.getReport());
}

/**
 * The stalled event is fired when the user agent is trying to fetch media data, but data is unexpectedly
 * not forthcoming. Fires multiple times during a video,but the video does not freeze or fail.
 * @param event
 */
VideoMonitor.prototype.mediaEvent_stalled = function (event) {
	//Log.d("VideoMonitor", "Event fired. event.type="+event.type);
	//this.setPlayerState(VideoMonitor.PlayerStates.Off);
}

/**
 * Fires when the audio/video is playing after having been paused or stopped for buffering.
 * @param event
 */
VideoMonitor.prototype.mediaEvent_playing = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		// Checks
		if (this.playerState == VideoMonitor.PlayerStates.Playing) {
			Log.d("VideoMonitor", "playing(): this.playerState is already 'Playing'. Returning");
			return;
		}
		if (this.videoSession == null)
			Log.e("VideoMonitor", "playing(): Error: this.videoSession == null");

		//Log.w("VideoMonitor", "this.playerState= "+this.playerState);
		if (this.playerState == VideoMonitor.PlayerStates.Off || this.playerState == VideoMonitor.PlayerStates.Buffering) {
			//
			// End the BufferingEvent
			this.bufferingEvent.end(event.timeStamp / 1000, false);
			//
			// If it was the startup BufferingEvent, update the VideoSession object
			// TODO: replace the condition with " this.playerState == VideoMonitor.PlayerStates.Off " ?
			if (this.hasStartupBuffCompleted == false) {
				this.parseServiceTypeAndProps();
				this.hasStartupBuffCompleted = true;
			}

			this.bufferingEvent = null;
			this.isBuffering = false;
			this.setPlayerState(VideoMonitor.PlayerStates.Playing);
			myPerformance.eventPlaying = new Date().getTime();
		}

		/*var bugSetBufferingState= this.playerState==VideoMonitor.PlayerStates.Buffering && this.isBuffering==false;
		 if (this.playerState == VideoMonitor.PlayerStates.Playing)
		 return;
		 if (this.playerState == VideoMonitor.PlayerStates.Off) {
		 Log.i("", "Video playing started. event.type=" + event.type);
		 } else if (bugSetBufferingState) {
		 Log.i("", "Video playing started (bugSetBufferingState was true). event.type=" + event.type);
		 } else if (this.playerState == VideoMonitor.PlayerStates.Buffering) {
		 Log.w("", "Buffering ended. event.type=" + event.type);
		 } else {
		 Log.d("VideoMonitor", "Event fired. event.type="+event.type+ " <--- Why?");
		 }
		 this.setPlayerState(VideoMonitor.PlayerStates.Playing);
		 myPerformance.eventPlaying= new Date().getTime();
		 this.isBuffering=false;*/
	} catch (err) {
		Log.e("VideoMonitor", err);
	}
}

VideoMonitor.prototype.mediaEvent_waiting = function (event) {
	try {
		Log.w("", "Buffering started. event.type=" + event.type); // +" t="+event.timeStamp/1000+"."
		if (this.hasStartupBuffCompleted == false) {
			Log.w("", "hasStartupBuffCompleted is false. Skipping.");
			return;
		}

		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "seeking(): Error: this.videoSession == null");
		if (this.bufferingEvent != null)
			Log.e("VideoMonitor", "seeking(): Error: this.bufferingEvent != null");
		// TODO: Add checks about PlayerStates

		this.bufferingEvent = new BufferingEvent(event.timeStamp / 1000, BufferingEvent.types.plain.value, this.videoSession);
		//
		// Check if this BufferingEvent was caused by a manual switch of resolution
		if (this.whenUserSwitchedRes>0 && (event.timeStamp/1000 -this.whenUserSwitchedRes) < TIME_WIN_CLICK_QUALITY_CLICK_RES) {
			Log.d("VideoMonitor", "waiting(): tClick="+(this.whenUserSwitchedRes) );
			Log.d("VideoMonitor", "waiting(): tBuff="+(event.timeStamp/1000) );
			Log.d("VideoMonitor", "waiting(): Dt="+(event.timeStamp/1000 -this.whenUserSwitchedRes)+ " < "+TIME_WIN_CLICK_QUALITY_CLICK_RES );
			Log.i("VideoMonitor", "waiting(): This BufferingEvent was probably caused by a manual resolution switch.");
			this.bufferingEvent.type= BufferingEvent.types.manResSwitch.value;
			this.whenUserSwitchedRes=0;
		}
		// TODO: BufferingEvent should be saved at the end
		this.dataset.BufferingEvents.push(this.bufferingEvent);
		this.isBuffering = true;
		this.setPlayerState(VideoMonitor.PlayerStates.Buffering);
		myPerformance.eventWaiting = new Date().getTime();
	} catch (err) {
		Log.e("VideoMonitor", err);
	}
}

/**
 * Sent when a seek operation begins. It creates a BufferingEvent of type Seek
 * @param event
 */
VideoMonitor.prototype.mediaEvent_seeking = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "seeking(): Error: this.videoSession == null");
		if (this.bufferingEvent != null && this.bufferingEvent.type == BufferingEvent.types.manResSwitch.value) {
			Log.d("VideoMonitor", "seeking(): Returning. There is a BufferingEvent of type 'manResSwitch' on progress.");
			return;
		}
		if (this.bufferingEvent != null) {
			Log.e("VideoMonitor", "seeking(): Error: this.bufferingEvent != null");
			return;
		}
		if (this.seekEvent != null)
			Log.e("VideoMonitor", "seeking(): Error: this.seekEvent != null");
		// TODO: This could happen if the user seeked again, before 'Seek'-type buffering ended.

		// Create the bufferingEvent of type "Seek"
		this.bufferingEvent = new BufferingEvent(event.timeStamp / 1000, BufferingEvent.types.seek.value, this.videoSession);

		// Find the last PauseEvent and update its type to "Seek".
		// A pauseEvent starts onMouseDown on the progress bar, that ends onMouseUp on it. The duration of the
		// PauseEvent is the time the user spends to decide at which exact position of the video he would like
		// to move.
		var lastPauseEvent = this.dataset.PauseEvents[this.dataset.PauseEvents.length - 1];
		lastPauseEvent.setType(PauseEvent.types.seek.value);

		// Create SeekEvent
		// TODO: find toTimestamp currentTime
		this.seekEvent = new SeekEvent(event.timeStamp / 1000, 0, this.videoSession);
		this.seekEvent.setBufferingEvent(this.bufferingEvent);
		this.seekEvent.setPauseEvent(lastPauseEvent);

		this.isBuffering = true;
		this.setPlayerState(VideoMonitor.PlayerStates.Buffering);
	} catch (err) {
		Log.e("VideoMonitor", err);
	}
}

/**
 * Sent when a seek operation completes. It ends the BufferingEvent of type Seek.
 * @param event
 */
VideoMonitor.prototype.mediaEvent_seeked = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "seeked(): Error: this.videoSession == null");
		if (this.bufferingEvent != null && this.bufferingEvent.type == BufferingEvent.types.manResSwitch.value) {
			Log.d("VideoMonitor", "seeked(): Returning. There is a BufferingEvent of type 'manResSwitch' on progress.");
			return;
		}
		if (this.seekEvent == null) {
			Log.e("VideoMonitor", "seeked(): Error: this.seekEvent == null");
			return;
		}
		if (this.bufferingEvent == null)
			Log.e("VideoMonitor", "seeked(): Error: this.bufferingEvent == null");
		if (/*this.bufferingEvent != null && */ this.bufferingEvent.type != BufferingEvent.types.seek.value)
			Log.e("VideoMonitor", "seeked(): Error: this.bufferingEvent.type is not 'Seek'" + " " + this.bufferingEvent.type + " != " + BufferingEvent.types.seek.value);

		this.bufferingEvent.end(event.timeStamp / 1000, false);
		this.dataset.BufferingEvents.push(this.bufferingEvent);
		this.bufferingEvent = null;

		this.seekEvent.to_video_time = this.videoElement.currentTime;
		this.dataset.SeekEvents.push(this.seekEvent);
		this.seekEvent = null;

		this.isBuffering = false;
		this.setPlayerState(VideoMonitor.PlayerStates.Playing);

	} catch (err) {
		Log.e("VideoMonitor", err);
	}
}

/**
 * The timeupdate event is fired when the time indicated by the currentTime attribute has been updated.
 * The event frequency is dependant on the system load, but will be thrown between about 4Hz and 66Hz
 * (assuming the event handlers don't take longer than 250ms to run).
 * @param event
 */
VideoMonitor.prototype.mediaEvent_timeupdate = function (event) {
	//Log.d("VideoMonitor", "Event fired. event.type="+event.type);
}

VideoMonitor.prototype.mediaEvent_pause = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "pause(): Error: this.videoSession == null");
		if (this.pauseEvent != null)
			Log.e("VideoMonitor", "pause(): Error: this.pauseEvent != null");
		this.pauseEvent = new PauseEvent(event.timeStamp / 1000, this.videoSession);
		this.setPlayerState(VideoMonitor.PlayerStates.Paused);
		//Log.i("VideoMonitor", myPerformance.getReport());

		//this.testNewFuncs(); // TODO: Remove this!!
	} catch (err) {
		Log.e("VideoMonitor", "pause(): " + err);
	}
}

VideoMonitor.prototype.mediaEvent_ratechange = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
}

VideoMonitor.prototype.mediaEvent_resize = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type + " resolution= " + this.videoElement.videoWidth + "x" + this.videoElement.videoHeight + ".");
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "resize(): Error: this.videoSession == null");
		if (this.videoResolution != null) {
			this.videoResolution.end(event.timeStamp / 1000);
			this.dataset.VideoResolutions.push(this.videoResolution);
			this.videoResolution = null;
		}
		if (this.hasStartupBuffCompleted == false) {
			// This is the first video resolution of the video. Pass the videoSession.start_timestamp
			this.videoResolution = new VideoResolution(this.videoSession.start_timestamp, this.videoElement.videoWidth, this.videoElement.videoHeight, this.videoSession);
		} else {
			this.videoResolution = new VideoResolution(event.timeStamp / 1000, this.videoElement.videoWidth, this.videoElement.videoHeight, this.videoSession);
		}
	} catch (err) {
		Log.e("VideoMonitor", "pause(): " + err);
	}
}

VideoMonitor.prototype.mediaEvent_volumechange = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
}

VideoMonitor.prototype.mediaEvent_ended = function (event) {
	try {
		Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "ended(): Error: this.videoSession == null");
		// End the BufferingEvent obj
		if (this.bufferingEvent != null) {
			this.bufferingEvent.end(event.timeStamp / 1000, false);
			// this.dataset.BufferingEvents.push(this.bufferingEvent); TODO: It should be already pushed in the Dataset by the waiting(...), right?
			this.bufferingEvent = null;
		}
		if (this.offScreenEvent != null) {
			this.offScreenEvent.end(event.timeStamp / 1000);
			this.dataset.OffScreenEvents.push(this.offScreenEvent);
			this.offScreenEvent = null;
		}
		if (this.videoResolution != null) {
			this.videoResolution.end(event.timeStamp / 1000);
			this.dataset.VideoResolutions.push(this.videoResolution);
			this.videoResolution = null;
		}
		if (this.playerSize != null) {
			this.playerSize.end(event.timeStamp / 1000);
			this.dataset.PlayerSizes.push(this.playerSize);
			this.playerSize = null;
		}
		this.stopPolling();
		// End the VideoSession obj
		this.videoSession.end(event.timeStamp / 1000, VideoSession.EndReasons.ended.value);
		this.videoSession = null;
		this.resetVariables();
		this.setPlayerState(VideoMonitor.PlayerStates.Off);
	} catch (err) {
		Log.e("VideoMonitor", "ended(): " + err);
	}
}

VideoMonitor.prototype.mediaEvent_suspend = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
	this.setPlayerState(VideoMonitor.PlayerStates.Off);
}

VideoMonitor.prototype.mediaEvent_abort = function (event) {
	Log.d("VideoMonitor", "abort(): Event fired. event.type=" + event.type);
	try {
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "abort(): Error: this.videoSession == null");
		// End the BufferingEvent obj
		if (this.bufferingEvent != null) {
			this.bufferingEvent.end(event.timeStamp / 1000, true);
			// this.dataset.BufferingEvents.push(this.bufferingEvent);
			this.bufferingEvent = null;
		}
		if (this.offScreenEvent != null) {
			this.offScreenEvent.end(event.timeStamp / 1000);
			this.dataset.OffScreenEvents.push(this.offScreenEvent);
			this.offScreenEvent = null;
		}
		if (this.videoResolution != null) {
			this.videoResolution.end(event.timeStamp / 1000);
			this.dataset.VideoResolutions.push(this.videoResolution);
			this.videoResolution = null;
		}
		if (this.playerSize != null) {
			this.playerSize.end(event.timeStamp / 1000);
			this.dataset.PlayerSizes.push(this.playerSize);
			this.playerSize = null;
		}
		this.stopPolling();
		// End the VideoSession obj
		this.videoSession.end(event.timeStamp / 1000, VideoSession.EndReasons.abort.value);
		this.videoSession = null;
		this.resetVariables();
		this.setPlayerState(VideoMonitor.PlayerStates.Off);
	} catch (err) {
		Log.e("VideoMonitor", "abort(): " + err);
	}
}

VideoMonitor.prototype.mediaEvent_error = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type);
	try {
		// Checks
		if (this.videoSession == null)
			Log.e("VideoMonitor", "error(): Error: this.videoSession == null");
		// End the BufferingEvent obj
		if (this.bufferingEvent != null) {
			this.bufferingEvent.end(event.timeStamp / 1000, false);
			// this.dataset.BufferingEvents.push(this.bufferingEvent);
			this.bufferingEvent = null;
		}
		if (this.offScreenEvent != null) {
			this.offScreenEvent.end(event.timeStamp / 1000);
			this.dataset.OffScreenEvents.push(this.offScreenEvent);
			this.offScreenEvent = null;
		}
		if (this.videoResolution != null) {
			this.videoResolution.end(event.timeStamp / 1000);
			this.dataset.VideoResolutions.push(this.videoResolution);
			this.videoResolution = null;
		}
		if (this.playerSize != null) {
			this.playerSize.end(event.timeStamp / 1000);
			this.dataset.PlayerSizes.push(this.playerSize);
			this.playerSize = null;
		}
		this.stopPolling();
		// End the VideoSession obj
		this.videoSession.end(event.timeStamp / 1000, VideoSession.EndReasons.error.value);
		this.videoSession = null;
		this.resetVariables();
		this.setPlayerState(VideoMonitor.PlayerStates.Off);
	} catch (err) {
		Log.e("VideoMonitor", "error(): " + err);
	}
}

/*
 * Custom events
 */

VideoMonitor.prototype.mediaEvent_videoPlayerResize = function (event) {
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type + " " + JSON.stringify(event));
	// Checks
	if (this.videoSession == null)
		Log.e("VideoMonitor", "videoPlayerResize(): Error: this.videoSession == null");

	if (this.playerSize != null) {
		this.playerSize.end(event.timeStamp / 1000);
		this.dataset.PlayerSizes.push(this.playerSize);
		this.playerSize = null;
	}

	if (this.hasStartupBuffCompleted == false) {
		// This is the first player size of the video. Pass the videoSession.start_timestamp
		this.playerSize = new PlayerSize(this.videoSession.start_timestamp, event.playerWidth, event.playerHeight, event.isOnFullScreen, this.videoSession);
	} else {
		this.playerSize = new PlayerSize(event.timeStamp / 1000, event.playerWidth, event.playerHeight, event.isOnFullScreen, this.videoSession);
	}
}

VideoMonitor.prototype.mediaEvent_visibilityChange = function (event) {
	this.isHidden = isTabHidden(); // Must be the first sentence of the method.
	Log.d("VideoMonitor", "Event fired. event.type=" + event.type + ". this.isHidden=" + this.isHidden); // TODO: values seem to be reverse.
	//Log.i("VideoMonitor", "visibilityChange(): New value= "+this.isHidden);
	// Checks
	if (this.videoSession == null)
		Log.e("VideoMonitor", "visibilityChange(): Error: this.videoSession == null");
	if (this.isHidden == true && this.offScreenEvent != null)
		Log.e("VideoMonitor", "visibilityChange(): Error: this.offScreenEvent is not null");
	if (this.isHidden == false && this.offScreenEvent == null)
		Log.e("VideoMonitor", "visibilityChange(): Error: this.offScreenEvent is null");

	// Manipulate the this.offScreenEvent property
	if (this.isHidden == true) {
		// Create a new OffScreenEvent
		this.offScreenEvent = new OffScreenEvent(event.timeStamp / 1000, this.videoSession);
	} else {
		// End the existing OffScreenEvent
		this.offScreenEvent.end(event.timeStamp / 1000);
		this.dataset.OffScreenEvents.push(this.offScreenEvent);
		this.offScreenEvent = null;

		if (this.videoSession.current_src == null || this.videoSession.current_src == "") {
			this.parseServiceTypeAndProps();
		}
	}
}

VideoMonitor.prototype.mediaEvent_userClickedYtpSettingsBtn= function (event) {
	try {
		Log.d("VideoMonitor", "userClickedYtpSettingsBtn(): User clicked the Settings button");
		if (this.userClickedQualityBtnRegistered==false) {
			var ytpMenuItemLabels = document.getElementsByClassName('ytp-menuitem-label');
			var qualityDiv = null;
			for (var i = 0; i < ytpMenuItemLabels.length; i++) {
				//Log.v("VideoMonitor", "userClickedYtpSettingsBtn(): div.nodeValue= " + ytpMenuItemLabels[i].textContent);
				if (ytpMenuItemLabels[i].textContent == "Quality") {
					qualityDiv = ytpMenuItemLabels[i];
					qualityDiv.addEventListener('click', this.mediaEvent_userClickedQualityBtn.bind(this));
					this.userClickedQualityBtnRegistered = true;
					Log.v("VideoMonitor", "userClickedYtpSettingsBtn(): qualityDiv found. userClickedQualityBtn() registered.");
					break;
				}
			}
		}
	} catch (err) {
		Log.e("VideoMonitor", "userClickedYtpSettingsBtn(): Error: "+err);
	}
}

VideoMonitor.prototype.mediaEvent_userClickedQualityBtn= function (event) {
	try {
		this.whenUserSwitchedRes= new Date().getTime();
		Log.d("VideoMonitor", "userClickedQualityBtn(): User clicked the Quality button. t="+this.whenUserSwitchedRes);
	} catch (err) {
		Log.e("VideoMonitor", "userClickedQualityBtn(): Error: "+err);
	}
}


/*
 * Static properties
 */

/** @type {VideoMonitor} */
VideoMonitor.object = null;

/** @type {string[]} */
VideoMonitor.mediaEventNames = [
	"loadstart",
	"progress",
	"suspend",
	"abort",
	"error",
	"emptied",
	"stalled",
	"loadedmetadata",
	"loadeddata",
	"canplay",
	"canplaythrough",
	"playing",
	"waiting",
	"seeking",
	"seeked",
	"ended",
	"durationchange",
	"timeupdate",
	"play",
	"pause",
	"ratechange",
	"resize",
	"volumechange"
];

/**
 * Enumeration of video player states. String values help logging.
 * @type {{Off: string, Buffering: string, Playing: string, Paused: string}}
 */
VideoMonitor.PlayerStates = {
	A: "a",
	Off: "off",
	Buffering: "buffering",
	Playing: "playing",
	Paused: "paused"
};

/*
 * Static methods
 */
// None yet.