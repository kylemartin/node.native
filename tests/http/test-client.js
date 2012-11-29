
var common = require('../common/common.js');
var assert = require('assert');
var http = require('http');

common.catch_errors(function(){


var spawn = require('child_process').spawn,
    child;

var kill = function() {
  if (child) {
    console.log('Killing children!');
    child.kill();
  }
};

process.on('uncaughtException', function(err) {
	console.log('uncaught: ' + err + '\n' + err.stack);
	kill();
	process.exit(-1);
});

child = common.runTest(process.argv[2],process.argv.slice(3));

console.log("Running test: " + process.argv.slice(2).join(" "));


var responses_recvd = 0;
var body0 = '';
var body1 = '';

function handle_error(err) {
	console.log((('code' in err) ? 'Error code: ' + err.code + '\n' : '') + err); 
  if (typeof req !== 'undefined') req.abort();
	// process.exit(-1);
}

var seen_req = false;

var server = http.createServer(function(req, res) {
	console.log('Received request');
  assert.equal('GET', req.method);
  assert.equal('/foo?bar', req.url);
  res.writeHead(200, {'Content-Type': 'text/plain'});
  res.write('hello\n');
  res.end();
  server.close();
  seen_req = true;
  console.log('Sent response');
});

server.on('connect', function() {
	console.log('received connection');
})

server.listen(common.PORT, function() {
	console.log('Server listening on ' + common.PORT);
});

timeout = setTimeout(function(){
	console.log("Process timeout!");
	kill();
	process.exit(-1);
}, 5000);

process.on('exit', function() {
  assert(seen_req);
  kill();
});

});

