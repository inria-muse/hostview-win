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
 * @param timestamp
 * @param currentVideoPlaytime
 * @param bufferedMinusCurrent
 * @param video_session
 * @constructor
 */
BufferedPlayTimeSample= function(timestamp, currentVideoPlaytime, bufferedMinusCurrent, video_session) {
    /**@type {number} */
    this.id = BufferedPlayTimeSample.idSeq++;

    /** @type {number} */
    this.t = timestamp;

    /** @type {number} currentVideoPlaytime, in sec. It is a sample of videoElement.currentTime. */
    this.cvpt= currentVideoPlaytime;

    /** @type {number} bufferedVideoPlaytime -currentVideoPlaytime, in sec.
     * It is the amount of video the player can play before stalling, if the connection was lost now. */
    this.bmc= bufferedMinusCurrent;

    this.video_session= video_session;
}

/**
 * Returns the bufferedVideoPlaytime.
 * @returns {number}
 */
BufferedPlayTimeSample.prototype.getBufferedVideoPlayTime= function() {
    return (this.cvpt + this.bmc);
}


/** @type {number} */
BufferedPlayTimeSample.idSeq=0;

