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
 * @param dataset {Dataset}
 * @param video_session {VideoSession}
 * @constructor
 */
VideoSessionSummary= function(dataset) {
    /**@type {number} */
    this.id= VideoSessionSummary.idSeq++;

    /** @type {number} */
    this.startup_delay=0;

    this.buffering_ratio=0;

    this.buffering_event_freq=0;

    this.avg_res_y=0;

    /** @type {VideoSession} */
    this.video_session= dataset.VideoSessions[0];

    this.calc1stVsStats(dataset);

}

VideoSessionSummary.prototype.calc1stVsStats= function(dataset) {
    try {
        var startupBufferingEvent = dataset.BufferingEvents[0];
        if (startupBufferingEvent.type != BufferingEvent.types.startup.value)
            Log.e("VideoSessionSummary", "calc1stVsStats(): BufferingEvent is not of type 'Startup' ");
        this.startup_delay = startupBufferingEvent.end_timestamp - startupBufferingEvent.start_timestamp; // in ms

        var sessionDuration = this.video_session.end_timestamp - this.video_session.start_timestamp;
        var bufferingDuration = 0; // only type==plain
        var bufferingCount=0;
        for (bf in dataset.BufferingEvents) {
            if (bf.type == BufferingEvent.types.plain.value) {
                bufferingDuration += bf.end_timestamp - bf.start_timestamp;
                bufferingCount++;
            }
        }
        this.buffering_ratio = bufferingDuration / sessionDuration;
        this.buffering_event_freq = bufferingCount / (sessionDuration / 1000); // in events/sec

        var avg_y=0;
        var vidres= null;
        for(var i=0; i< dataset.VideoResolutions.length ; i++) {
            vidres= dataset.VideoResolutions[i];
        //}
        //for (vidres in dataset.VideoResolutions) {
            Log.i("VideoSessionSummary", "calc1stVsStats(): vidres= "+JSON.stringify(vidres));
            avg_y+= vidres.res_y*(vidres.end_timestamp -vidres.start_timestamp);
            Log.i("VideoSessionSummary", "calc1stVsStats(): vidres.res_y= "+(vidres.res_y)+", avg_y="+avg_y);
        }
        avg_y= avg_y/sessionDuration;
        this.avg_res_y= avg_y;


    } catch (err) {
        Log.e("VidSesSummary", "calc1stVsStats(): Error: ");
    }


}

/** @type {PauseEvent} */
VideoSessionSummary.object= null;

/** @type {number} */
VideoSessionSummary.idSeq=0;