common = require('../common/common.js');

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

// timer = setInterval(function(){
// 	console.log(".");
// }, 1000);

timeout = setTimeout(function(){
	console.log("Process timeout!");
	kill();
	process.exit(-1);
}, 5000);


// process.on('uncaughtException', cleanup);
process.on('exit', kill);

});