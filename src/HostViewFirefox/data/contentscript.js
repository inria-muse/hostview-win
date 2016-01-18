/**
 * Hostview addon main content script.
 */

/* Get timezone name from a date object. */
function gettzname(ts) {
	if (!ts) return null;
	
    var tzinfo = ts.toString().match(/([-\+][0-9]+)\s\(([A-Za-z\s].*)\)/);
    if (!tzinfo)
        tzinfo = ts.toString().match(/([-\+][0-9]+)/);
	return (tzinfo && tzinfo.length > 1 ? tzinfo[2] : null);	
}

/** Get pageload stats. */
self.port.on("getPageloadStats", function() { 
	setTimeout(function() { 
		var ts = new Date();
		self.port.emit("plt", {
            ts : ts.getTime(), 							// UTC timestamp
            timezoneoffset : ts.getTimezoneOffset(),    // offset
            timezonename : gettzname(ts),               // timezone
            location : window.location.href,
			restiming : window.performance.getEntries(),// page resources
			timing : window.performance.timing,         // main pageload info
			navinfo : window.performance.navigation     // navigation info
		}); 
	}, 10); 
});

// TODO: add similar methods for video content