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
 * A polling mechanism will create objects of this class often, and these objects will be serialized in the JSON file,
 * and POSTed to the server. To keep memory overhead and JSON file size low, the property names of this class are kept
 * really short.
 *
 *
 * @param timestamp
 * @param totalVideoFrames
 * @param droppedVideoFrames
 * @param corruptedVideoFrames
 * @param totalFrameDelay
 * @param mozParsedFrames
 * @param mozDecodedFrames
 * @param mozPresentedFrames
 * @param mozPaintedFrames
 * @param mozFrameDelay
 * @param video_session
 * @constructor
 */
VideoPlaybackQualitySample= function(timestamp, totalVideoFrames, droppedVideoFrames, corruptedVideoFrames,
                               totalFrameDelay, mozParsedFrames, mozDecodedFrames, mozPresentedFrames, mozPaintedFrames,
                               mozFrameDelay, video_session) {
    /**@type {number} */
    this.id = VideoPlaybackQualitySample.idSeq++;

    /** @type {number} */
    this.t = timestamp;

    this.tvf= totalVideoFrames ? totalVideoFrames : -1;
    this.dvf= droppedVideoFrames ? droppedVideoFrames : -1;
    this.cvf= corruptedVideoFrames ? corruptedVideoFrames : -1;
    this.tfd= totalFrameDelay ? totalFrameDelay : -1;
    this.mprsf= mozParsedFrames ? mozParsedFrames : -1;
    this.mdf= mozDecodedFrames ? mozDecodedFrames : -1;
    this.mpf= mozPresentedFrames ? mozPresentedFrames : -1;
    this.mpntf= mozPaintedFrames ? mozPaintedFrames : -1;
    this.mfd= mozFrameDelay ? mozFrameDelay : -1;

    this.video_session= video_session;
}

/** @type {number} */
VideoPlaybackQualitySample.idSeq=0;