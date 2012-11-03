common = require('../common/common.js');

common.catch_errors(function(){

var net = require('net');
var assert = require('assert');

var serverConnected = 0;
var child;

var server = net.createServer(function(socket) { //'connection' listener
	console.log("incoming connection");
  socket.end();
  if (++serverConnected === 2) {
    server.close();
  }
});

var kill = function() {
  if (child) {
    console.log('Killing children!');
    child.kill();
  }
};

server.listen(common.PORT, 'localhost', function() { //'listening' listener
  console.log('server bound');
  // start test
  var child = common.runTest(process.argv[2],process.argv.slice(3));
  
  console.log("Running test: " + process.argv.slice(2).join(" "));

  timeout = setTimeout(function(){
    console.log("Test timeout!");
    kill();
    process.exit(-1);
  }, 5000);
});

// process.on('uncaughtException', cleanup);
process.on('exit', function(){
  kill();
  assert.equal(serverConnected,2);
});

});