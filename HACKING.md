# Using Eclipse with Non-Standard Compiler #

Must set PATH, CC, and CXX environment variables in Project Properties > C/C++ Build > Environment

Must also set executable for each configuration and tool in Project Properties > C/C++ Build > Discovery Options, though this may or may not discover standard library include paths (for syntax checking), so it might still be necessary to manually specify include paths in for each copnfiguration and tool in Project Properties > C/C++ General > Paths and Symbols.

# Debugging with Eclipse (tested with Juno) #

Must install recent gdb (tested with 7.5.50) to debug with recent gcc (tested with 4.7.2).

To debug standard library code must install gcc from source or tell Fink not to remove intermediary files:

	fink -k install gcc47

If necessary customize Eclipse CDT to specify path to gdb:
	globally in Preferences > C/C++ > Debug > GDB
	per debug configuration in Run > Debug Configuration > [selected configuration] > Debugger

Note that recent gdb doesn't get along well with recent gcc and Eclipse CDT is doesn't work perfectly well with recent gdb either. Problems observed:
	Cannot suspend while running
	Cannot insert breakpoints while running (and cannot suspend so have to restart)

# Compiling GDB from source #

Checkout, configure and make:
	$ git clone git://sourceware.org/git/gdb.git
	$ cd gdb
	$ ./configure
	$ make

Then follow http://sourceware.org/gdb/wiki/BuildingOnDarwin to create a key and sign the executable (./gdb/gdb).
Then do install gdb and supporting code (like python libraries needed by Eclipse):
	$ sudo make install

# Manually Testing HTTP Servers #

## Curl ##

Pass the -v option to enable verbose logging of headers:
	$ curl -v localhost:1337
	* About to connect() to localhost port 1337 (#0)
	*   Trying 127.0.0.1... connected
	* Connected to localhost (127.0.0.1) port 1337 (#0)
	> GET / HTTP/1.1
	> User-Agent: curl/7.19.7 (universal-apple-darwin10.0) libcurl/7.19.7 OpenSSL/0.9.8r zlib/1.2.3
	> Host: localhost:1337
	> Accept: */*
	> 
	< HTTP/1.1 200 OK
	< Content-Type: text/plain
	< Connection: close
	< 
	* Closing connection #0

Other options:
	-0 option to use HTTP/1.0

## Netcat ##

	$ echo -e "GET / HTTP/1.0\r\n Post Data \r\n" | nc localhost 1337

# OSX Notes #

## Show processes with open ports ##
lsof -i | grep LISTEN