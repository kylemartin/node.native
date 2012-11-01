var spawn = require('child_process').spawn,
    child;

child = spawn(process.argv[2],process.argv.slice(3));

child.stdout.on('data',function(data) {
	console.log("stdout: " + data);	
});
child.stderr.on('data',function(data) {
	console.log("stderr: " + data);	
});
child.on('exit',function(code){
 	console.log('child process exited with code ' + code);
 	process.exit(-1);
});

console.log("Running: " + process.argv.slice(2).join(" "));

timer = setInterval(function(){
	console.log(".");
}, 1000);

timeout = setTimeout(function(){
	console.log("Process timeout!");
}, 10000);