common = require('../common/common.js');

common.catch_errors(function(){

var dgram = require('dgram');
var assert = require('assert');

var received = 0;
var child;

var kill = function() {
  if (child) {
    console.log('Killing children!');
    child.kill();
  }
};

var socket = dgram.createSocket('udp4', function(buf, addr) { //'connection' listener
  console.log("incoming datagram");
  if (++received === 2) {
    socket.close();
  }
});

socket.on('listening', function() { //'listening' listener
  console.log('udp socket listening');
  // start test
  var child = common.runTest(process.argv[2],process.argv.slice(3));
  
  console.log("Running test: " + process.argv.slice(2).join(" "));

});

socket.bind(common.PORT);

timeout = setTimeout(function(){
	console.log("Test timeout!");
	kill();
	process.exit(-1);
}, 5000);

// process.on('uncaughtException', cleanup);
process.on('exit', function(){
  kill();
  assert.equal(received,2);
});

});