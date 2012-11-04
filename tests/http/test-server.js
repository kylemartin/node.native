var common = require('../common/common.js');
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

child = common.runTest(process.argv[2],process.argv.slice(3));

console.log("Running test: " + process.argv.slice(2).join(" "));


var responses_recvd = 0;
var body0 = '';
var body1 = '';

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
    console.log('Got /hello response');
  });
}, 1000);

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
    console.log('Got /world response');
  });
  req.end();
}, 2000);

timeout = setTimeout(function(){
	console.log("Process timeout!");
	kill();
	process.exit(-1);
}, 5000);

process.on('exit', function() {
  console.log('responses_recvd: ' + responses_recvd);
  assert.equal(2, responses_recvd);

  assert.equal('The path was /hello', body0);
  assert.equal('The path was /world', body1);

  kill();
});

});

