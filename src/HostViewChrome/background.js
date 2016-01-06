function StripURL(url) {
	var res = url.replace("https://", "").replace("http://", "");
	if (res.indexOf('/') > -1) {
		res = res.substring(0, res.indexOf('/'));
	}
	return res;
}

function getProtocol(url) {
	if (url.indexOf("https") > -1) {
		return "https";
	} else {
		return "http";
	}
}

function getCurrentTab(callback) {
	var queryInfo = {
		active: true,
		currentWindow: true
	};

	chrome.tabs.query(queryInfo, function(tabs) {
		var tab = tabs[0];
		callback(getProtocol(tab.url), StripURL(tab.url));
	});
}


function sendTabInfoInternal(productUrl, protocol, url, event, userId) {
	if (url.indexOf('chrome:') > -1) {
		return;
	}
	
	var http = new XMLHttpRequest();
	
	http.open("POST", productUrl, true);
	http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");

	http.onreadystatechange = function() {
		// do something?
	}
	http.send("location=" + encodeURIComponent(url) + "&browser=chrome&event=" + event
		+ "&time=" + new Date().getTime() + "&protocol=" + protocol + "&user=" + encodeURIComponent(userId));
}

// userId as seen here: http://stackoverflow.com/questions/23822170/getting-unique-clientid-from-chrome-extension
function getRandomToken() {
    // E.g. 8 * 32 = 256 bits token
    var randomPool = new Uint8Array(32);
    crypto.getRandomValues(randomPool);
    var hex = '';
    for (var i = 0; i < randomPool.length; ++i) {
        hex += randomPool[i].toString(16);
    }
    // E.g. db18458e2782b2b77e36769c569e263a53885a9944dd0a861e5064eac16f1a
    return hex;
}

function getUserId(callback) {
	chrome.storage.sync.get('userid', function(items) {
		var userid = items.userid;
		if (userid) {
			callback(userid);
		} else {
			userid = getRandomToken();
			chrome.storage.sync.set({userid: userid}, function() {
				callback(userid);
			});
		}
	});
}
// end of userId

function sendTabInfo(protocol, url, event) {
	getUserId(function(userId) {
		sendTabInfoInternal("http://localhost:27015/", protocol, url, event, userId);
		sendTabInfoInternal("https://ucnproject.uk/hostview/extensions", protocol, url, event, userId);
	});
}


var tabs = {};
var protocols = {};

chrome.tabs.onActivated.addListener(function(activeInfo) {
	getCurrentTab(function(protocol, url) {
		sendTabInfo(protocol, url, "active");
		
		tabs[activeInfo.tabId] = url;
		protocols[activeInfo.tabId] = protocol;
	});
});

chrome.tabs.onUpdated.addListener(function(tabId, changeInfo, updatedTab) {
	getCurrentTab(function(protocol, url) {
		sendTabInfo(protocol, url, "update");
		
		tabs[tabId] = url;
		protocols[activeInfo.tabId] = protocol;
	});
});

chrome.tabs.onRemoved.addListener(function(tabId, removeInfo) {
	sendTabInfo(protocols[activeInfo.tabId], tabs[tabId], "close");
});