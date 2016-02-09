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
 * @param type {number}
 * @param video_session {VideoSession}
 * @constructor
 */
BufferingEvent= function(start_timestamp, type, video_session) {
    /**@type {number} */
    this.id= BufferingEvent.idSeq++;

    /** @type {number} */
    this.start_timestamp=start_timestamp;

    /** @type {number} */
    this.end_timestamp=0;

    /** @type {number} */
    this.type=type;

    /** @type {boolean} */
    this.ended_by_abort=false;

    /** @type {VideoSession} */
    this.video_session= video_session;
}


BufferingEvent.prototype.end= function(end_timestamp, ended_by_abort) {
    this.end_timestamp= end_timestamp;
    this.ended_by_abort=ended_by_abort;
}


/*BufferingEvent.prototype.toJSON= function () {
    var retObj= JSON.parse(JSON.stringify(this));
    //return "{id:"+this.id+"}";
    return JSON.stringify(this, this.toJSONModFunc, '\t');
}*/



BufferingEvent.prototype.toJSONModFunc= function(key, value) {
    for (var ref_prop in Dataset.reference_properties) {
        if (key == ref_prop) {
            return key['id'];
        }
    }
    return value;
}

/** @type {BufferingEvent} */
BufferingEvent.object= null;

/** @type {number} */
BufferingEvent.idSeq=0;

/** @type {Object} */
BufferingEvent.types= {
    plain: {name:"plain", value:0},
    startup: {name:"startup", value:1},
    seek: {name:"seek", value:2},
    manResSwitch: {name:"manResSwitch", value:3}
}