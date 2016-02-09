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

Dataset= function() {
    this.VideoSessions= new Array();
    this.BufferingEvents= new Array();
    this.PauseEvents= new Array();
    this.VideoResolutions= new Array();
    this.SeekEvents= new Array();
    this.OffScreenEvents= new Array();
    this.PlayerSizes= new Array();

    this.VideoPlaybackQualitySamples= new Array();
    this.BufferedPlayTimeSamples= new Array();

    this.VideoSessionSummaries= new Array();

    this.Logs= new Array();
}

/*Dataset.prototype.toString= function() {
    var str="";
    str+= "Dataset: [";
    str+= "VideoSessions.length="+this.VideoSessions.length+", ";
    str+= "BufferingEvents.length="+this.BufferingEvents.length+", ";
    str+= "PauseEvents.length="+this.PauseEvents.length+", ";
    str+= "VideoResolutions.length="+this.VideoResolutions.length;
    str+="]";
    return str;
}*/

/*Dataset.prototype.toJSON= function() {
    Log.d("Dataset", "toJSON() was called. ");
    //return "--no JSON here--";
    //return "{VideoSessions:"+this.VideoSessions+"}";
    //return JSON.stringify(this, ["VideoSessions", "BufferingEvents"], '\t');
    //return JSON.stringify(this, this.toJSONModFunc, '\t');
    return this.toString(); // JSON.stringify(this);
}*/

Dataset.reference_properties= ['video_session', 'buffering_event', 'pause_event', 'video_resolution'];

Dataset.prototype.toJSONModFunc= function(key, value) {
    //if (typeof value === "string") {
    //    return undefined;
    //}
    var ref_prop="";
    for (var i=0; i < Dataset.reference_properties.length; i++) {
        ref_prop= Dataset.reference_properties[i];
        if (key == ref_prop) {
            //Log.d("Dataset", "toJSONModFunc(): in for-in loop. key="+key+", value="+value+", ref_prop="+ref_prop+", key['id']="+value['id']);
            return value['id'];
        }
    }
    return value;
}

Dataset.send= function(dataset) {
    try {
	    var data = JSON.stringify(dataset, dataset.toJSONModFunc, '\t');
	    Log.i("Dataset", "dataset.toJSON= " + data);
	    if (VIDEO_MON_CONF.USE_CUSTOM_UPLOAD_FUNC == true) {
		    Log.i("Dataser", "send(): Calling the CUSTOM_UPLOAD_FUNC defined in the configuration obj (main.js).");
		    VIDEO_MON_CONF.CUSTOM_UPLOAD_FUNC(data);
	    } else {
		    Log.i("Dataser", "send(): Calling Dataset.postToServer(...).");
		    Dataset.postToServer(data);
	    }
    }catch (err) {
        Log.e("Dataser", "send(): Error: "+err);
    }
}

Dataset.postToServer= function (jsonString) {
	try {
		//Log.i("Dataset", "jsonString= " + jsonString);
		var http = new XMLHttpRequest();

		Log.d("Dataset", "postToServer(): SRV_POST_URL= "+VIDEO_MON_CONF.SRV_POST_URL);
		http.open("POST", VIDEO_MON_CONF.SRV_POST_URL, true);

		//Send the proper header information along with the request
		http.setRequestHeader("Content-type", "application/json");
		http.setRequestHeader("Content-length", jsonString.length);
		http.setRequestHeader("Connection", "close");

		http.onreadystatechange = function () { //Call a function when the state changes.
			if (http.readyState == 4 && http.status == 200) {
				//alert(http.responseText);
				Log.i("Dataset.js", "postToServer(): Dataset sent. Server says: " + http.responseText);
			}
		}
		http.send(jsonString);
	} catch (err) {
		Log.e("Dataser", "postToServer(): Error: "+err);
	}
}