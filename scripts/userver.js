// simple node.js server for receiving test uploads

var http = require('http');
var fs = require('fs');
var url = require('url');

var PORT;

if(process.argv.length > 2){
	PORT = parseInt(process.argv[2]);
} else {
	PORT = 3000;
}

var server = http.createServer(function(req, res) {
	console.log("req from " + req.socket.remoteAddress + ":" + req.socket.remotePort);
	console.log(req.method + " " + req.url + " HTTP/" + req.httpVersion);
	console.log(req.headers);

	var outst = fs.createWriteStream("./tmp/" + decodeURIComponent(url.parse(req.url).pathname).split('/').slice(-1)[0]);

	outst.on("error", function(err) {
		console.error('stream errors: ' + err);
		res.writeHead(500, {'Content-Type' : 'text/plain'});
		res.end('failed');
	});

	outst.on("finish", function() {
		console.log('stream end');
		res.writeHead(200, {'Content-Type' : 'text/plain'});
		res.end('ok');
	});

	req.pipe(outst);
});

server.listen(PORT, function() {
	console.log("http server listening on *:"+PORT);
});