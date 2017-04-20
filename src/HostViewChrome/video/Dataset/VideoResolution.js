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
 * @param start_timestamp {number}
 * @param res_x {number}
 * @param res_y {number}
 * @param video_session {VideoSession}
 * @constructor
 */
VideoResolution= function(start_timestamp, res_x, res_y, video_session) {
    /**@type {number} */
    this.id= VideoResolution.idSeq++;

    /** @type {number} */
    this.start_timestamp=start_timestamp;

    /** @type {number} */
    this.end_timestamp=0;

    /** @type {number} */
    this.res_x=res_x;

    /** @type {number} */
    this.res_y=res_y;

    /** @type {VideoSession} */
    this.video_session= video_session;
}


VideoResolution.prototype.end= function(end_timestamp) {
    this.end_timestamp= end_timestamp;
}

VideoResolution.prototype.setResolution= function(res_x, res_y) {
    this.res_x=res_x;
    this.res_y=res_y;
}

/*VideoResolution.prototype.toString= function() {
    return JSON.stringify(this);
}*/

/*VideoResolution.prototype.toJSON= function () {
    return "{id:"+this.id+"}";
    //return JSON.stringify(this, this.toJSONModFunc, '\t');
}*/

VideoResolution.prototype.toJSONModFunc= function(key, value) {
    for (var ref_prop in Dataset.reference_properties) {
        if (key == ref_prop) {
            return key['id'];
        }
    }
    return value;
}

/** @type {VideoResolution} */
VideoResolution.object= null;

/** @type {number} */
VideoResolution.idSeq=0;