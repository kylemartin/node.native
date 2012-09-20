#!/usr/bin/env node

var http = require('http');
var options = {
  host: '127.0.0.1',
  port: 1337,
  path: '/',
  method: 'GET'
};

var req = http.request(options);

/**
 * WritableStream Interface Events
 */

req.on('drain', function() {
	console.log('[req] on drain');
});

req.on('error', function(e) {
  console.log('[req] on error: ' + e.message);
});

req.on('close', function() {
	console.log('[req] on close');
})

req.on('pipe', function(src) {
	console.log('[req] on pipe');
});

/**
 * ClientRequest Interface Events
 */

req.on('socket', function(socket) {
	console.log('[req] on socket');
});

req.on('connect', function(response, socket, head) {
	console.log('[req] on connect');
});

req.on('upgrade', function(response, socket, head) {
	console.log('[req] on upgrade');
});

req.on('continue', function() {
	console.log('[req] on continue');
});

req.on('response', function(res) {
	console.log('[req] on response');
	console.log('STATUS: ' + res.statusCode);
	console.log('HEADERS: ' + JSON.stringify(res.headers));
	res.setEncoding('utf8');

	/**
	 * ReadableStream Interface Events
	 */

	res.on('data', function (chunk) {
		console.log('[res] on data: ' + chunk);
	});

	res.on('end', function() {
		console.log('[res] on end');
	});

	res.on('error', function(e) {
	  console.log('[res] on error: ' + e.message);
	});

	res.on('close', function() {
		console.log('[res] on close');
	})
});


req.end();