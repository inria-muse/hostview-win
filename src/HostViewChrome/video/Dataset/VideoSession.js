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

VideoSession= function(start_timestamp) {
    /**@type {number} */
    this.id= VideoSession.idSeq++;

    /** @type {number} */
    this.start_timestamp=start_timestamp;

    /** @type {number} */
    this.end_timestamp=0;

    this.service_type=VideoSession.ServiceTypes.vod.value;

    /** @type {string} */
    this.window_location="";

    /** @type {string} */
    this.current_src="";

    /** @type {number} */
    this.duration=0;

    this.title="";

    /** @type {number} */
    this.end_reason=0;

    /** @type {number} */
    this.qoe_score=0;
}

VideoSession.prototype.end= function(end_timestamp, endReason) {
    this.end_timestamp= end_timestamp;
    this.end_reason= endReason;
}

/*VideoSession.prototype.toString= function() {
    return JSON.stringify(this);
}*/

/*VideoSession.prototype.toJSON= function () {
    //return "{id:"+this.id+"}";
    return JSON.stringify(this, null, '\t');
}*/

/** @type {VideoSession} */
VideoSession.object= null;

/** @type {number} */
VideoSession.idSeq=0;

/** @type {Object} */
VideoSession.EndReasons= {
    ended: {name:"ended", value:0},
    abort: {name:"abort", value:1},
    error: {name:"error", value:2},
    stalled: {name:"stalled", value:3},
    suspend: {name:"suspend", value:4},
}

VideoSession.ServiceTypes= {
    vod: {name: "vod", value: 0},
    live: {name:"live", value: 1}
}

