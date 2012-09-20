#!/usr/bin/env node

var http = require('http');
var server = http.createServer();

server.on('request', function (req, res) {
	console.log('[server] on request');
	console.log('METHOD: ' + req.method);
	console.log('URL: ' + req.url);
	console.log('VERSION: ' + req.httpVersion);
	console.log('HEADERS: ' + JSON.stringify(req.headers));

	req.on('data', function(chunk) {
		console.log('[req] on data: ' + chunk);
	});

	req.on('end', function() {
		console.log('[req] on end');
		console.log('TRAILERS: ' + JSON.stringify(req.trailers));
	});

	req.on('error', function(e) {
		console.log('[req] on error');
	});

	req.on('close', function() {
		console.log('[req] on close');
	});

	res.on('drain', function() {
		console.log('[res] on drain');
	});

	res.on('error', function(e) {
	  console.log('[res] on error: ' + e.message);
	});

	res.on('close', function() {
		console.log('[res] on close');
	})

	res.on('pipe', function(src) {
		console.log('[res] on pipe');
	});

	res.writeHead(200, {'Content-Type': 'text/plain'});
	res.end('Hello World\n');
});

server.on('connection', function(socket) {
	console.log('[server] on connection');
});

server.on('close', function() {
	console.log('[server] on close');
});

server.on('checkContinue', function(request, response) {
	console.log('[server] on checkContinue');
});

server.on('connect', function (request, socket, head) {
	console.log('[server] on connect');
});

server.on('upgrade', function (request, socket, head) {
	console.log('[server] on upgrade');
});

server.on('clientError', function (e) {
	console.log('[server] on clientError: ' + e.message);
});

server.listen(1337, '127.0.0.1');

console.log('Server running at http://127.0.0.1:1337/');