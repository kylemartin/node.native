common = require('../common/common.js');

common.catch_errors(function(){

var dgram = require('dgram');
var assert = require('assert');

var child;

var kill = function() {
  if (child) {
    console.log('Killing children!');
    child.kill();
  }
};

switch (process.argv.slice(3,4).toString()) {
case "test-udp/UDPSend": {
	var received = 0;
	
	console.log("XXX: testing UDPSend");

	var socket = dgram.createSocket('udp4', function(buf, addr) { //'connection' listener
	  console.log("incoming datagram: " + buf);
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

	//process.on('uncaughtException', cleanup);
	process.on('exit', function(){
	 kill();
	 assert.equal(received,2);
	});

}	break;
case "test-udp/UDPReceive": {
	var sent = 0;
	console.log("XXX: testing UDPReceive");

	var socket = dgram.createSocket('udp4', function(buf, addr) { //'connection' listener
	  console.log("should not have received datagram! : " + buf);
	});

	var interval = setInterval(function(){
		var message = new Buffer("message" + sent);
		console.log("sending datagram: " + message);
		socket.send(message, 0, message.length, common.PORT, "127.0.0.1", function(err, bytes) {
			if ((sent = sent+1) >= 2) {
				clearInterval(interval);
				socket.close();
			}
		});
	}, 100);
	
  console.log('udp socket listening');
  // start test
  var child = common.runTest(process.argv[2],process.argv.slice(3));
  
  console.log("Running test: " + process.argv.slice(2).join(" "));
  
  socket.bind();

	//process.on('uncaughtException', cleanup);
	process.on('exit', function(){
	 kill();
	 assert.equal(sent,2);
	});
}	break;
default:
	console.log("XXX: testing ?");
}


timeout = setTimeout(function(){
	console.log("Test timeout!");
	kill();
	process.exit(-1);
}, 5000);


});