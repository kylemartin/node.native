common = require('../common/common.js');
common.catch_errors(function(){

var spawn = require('child_process').spawn,
    child;

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

// timer = setInterval(function(){
// 	console.log(".");
// }, 1000);

timeout = setTimeout(function(){
	console.log("Process timeout!");
	if (child) child.kill();
	process.exit(-1);
}, 5000);


var cleanup = function() {
  console.log('Killing Children!');
  if (child)
    child.kill();
};
// process.on('uncaughtException', cleanup);
process.on('exit', cleanup);

});