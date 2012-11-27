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
var body2 = '';

function handle_error(err) {
	console.log((('code' in err) ? 'Error code: ' + err.code + '\n' : '') + err); 
  if (typeof req !== 'undefined') req.abort();
	// process.exit(-1);
}

var agent = new http.Agent({ port: common.PORT, maxSockets: 1 });
setTimeout(function() {
	console.log('Sending /hello request');
  http.get({
    port: common.PORT,
    path: '/hello',
    headers: {'Accept': '*/*', 'Foo': 'bar'},
    agent: agent
  }, function(res) {
    assert.equal(200, res.statusCode);
    responses_recvd += 1;
    res.setEncoding('utf8');
    res.on('data', function(chunk) { body0 += chunk; });
    res.on('error', handle_error);
    console.log('Got /hello response');
  })
  .on('error',handle_error);
}, 100);

setTimeout(function() {
	console.log('Sending /world request');
  var req = http.request({
    port: common.PORT,
    method: 'POST',
    path: '/world',
    agent: agent
  }, function(res) {
    assert.equal(200, res.statusCode);
    responses_recvd += 1;
    res.setEncoding('utf8');
    res.on('data', function(chunk) { body1 += chunk; });
    res.on('error', handle_error);
    console.log('Got /world response');
  });
  req.on('error', handle_error);
  req.end();
}, 200);

setTimeout(function() {
	console.log('Sending /foo request');
  var req = http.request({
    port: common.PORT,
    method: 'POST',
    path: '/foo',
    agent: agent
  }, function(res) {
    assert.equal(200, res.statusCode);
    responses_recvd += 1;
    res.setEncoding('utf8');
    res.on('data', function(chunk) { body2 += chunk; });
    res.on('error', handle_error);
    console.log('Got /foo response');
  });
  req.on('error', handle_error);
  req.write(common.LOREM);
  req.end();
}, 300);

timeout = setTimeout(function(){
	console.log("Process timeout!");
	kill();
	process.exit(-1);
}, 5000);

process.on('exit', function() {
  console.log('responses_recvd: ' + responses_recvd);
  assert.equal(3, responses_recvd);

  assert.equal('The path was /hello', body0);
  assert.equal('The path was /world', body1);
  assert.equal(common.LOREM, body2);

  kill();
});

});

