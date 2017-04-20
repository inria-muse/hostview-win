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
 *
 * @param timestamp {number}
 * @param to_video_time {number}
 * @param video_session {VideoSession}
 * @constructor
 */
SeekEvent= function(timestamp, to_video_time, video_session) {
    /**@type {number} */
    this.id= SeekEvent.idSeq++;

    /** @type {number} */
    this.timestamp=timestamp;

    /** @type {number} */
    this.to_video_time=to_video_time;

    /** @type {VideoSession} */
    this.video_session= video_session;

    this.buffering_event= null;

    this.pause_event= null;
}

/**
 *
 * @param buffering_event {BufferingEvent}
 */
SeekEvent.prototype.setBufferingEvent= function(buffering_event) {
    this.buffering_event= buffering_event;
}

/**
 *
 * @param buffering_event {BufferingEvent}
 */
SeekEvent.prototype.setPauseEvent= function(pause_event) {
    this.pause_event= pause_event;
}

/*SeekEvent.prototype.toString= function() {
    return JSON.stringify(this);
}*/

/** @type {SeekEvent} */
SeekEvent.object= null;

/** @type {number} */
SeekEvent.idSeq=0;
