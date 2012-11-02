common = require('../common/common.js');

common.catch_errors(function(){
var net = require('net');
var server = net.createServer(function(c) { //'connection' listener
  console.log('server connected');
  c.on('end', function() {
    console.log('server disconnected');
  });
  c.write('hello\r\n');
  c.pipe(c);
});
var child;
server.listen(1337, function() { //'listening' listener
  console.log('server bound');
  // start test
  var spawn = require('child_process').spawn;

  child = spawn(process.argv[2],process.argv.slice(3));

  child.stdout.on('data',function(data) {
    common.logblue(data); 
  });
  child.stderr.on('data',function(data) {
    common.logred(data); 
  });
  child.on('exit',function(code){
    console.log('child process exited with code ' + code);
    process.exit(-1);
  });

  console.log("Running: " + process.argv.slice(2).join(" "));

  timeout = setTimeout(function(){
    console.log("Process timeout!");
    if (child) child.kill();
    process.exit(-1);
  }, 5000);
});

var cleanup = function() {
  console.log('Killing Children!');
  if (child)
    child.kill();
};
// process.on('uncaughtException', cleanup);
process.on('exit', cleanup);

});