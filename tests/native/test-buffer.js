common = require('../common/common.js');

common.catch_errors(function(){

var child;

var kill = function() {
  if (child) {
    console.log('Killing children!');
    child.kill();
  }
};

// start test
var child = common.runTest(process.argv[2],process.argv.slice(3));

console.log("Running test: " + process.argv.slice(2).join(" "));

timeout = setTimeout(function(){
  console.log("Test timeout!");
  kill();
  process.exit(-1);
}, 5000);

// process.on('uncaughtException', cleanup);
process.on('exit', function(){
  kill();
});

});