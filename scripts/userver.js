// simple node.js server for receiving test uploads

var http = require('http');

const PORT = 3000;

var server = http.createServer(function(req, res) {
	console.log("req from " + req.socket.remoteAddress + ":" + req.socket.remotePort);
	console.log(req.method + " " + req.url + " HTTP/" + req.httpVersion);
	console.log(req.headers);

	var data = '';
	req.socket.on('data', function(chunk) {
		console.log("got chunk with " + chunk.length + " bytes of data")
		data += chunk;
		if (data.length === parseInt(req.headers['content-length'])) {
			res.writeHead(200, {'Content-Type' : 'text/plain'});
			res.end('ok');
			data = '';
		}		
	});

	req.socket.on('end', function(chunk) {
		console.log("client closed the socket")
	});	

	req.on('error', function(err) {	
		console.log('error ' + err);
		res.writeHead(500, {'Content-Type' : 'text/plain'});
		res.end('failed');
	});

	req.on('end', function() {
		console.log('req end');
	});
});

server.listen(PORT, function() {
	console.log("http server listening on *:"+PORT);
});