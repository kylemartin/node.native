# node.native 

<b>node.native</b> is a [C++11](http://en.wikipedia.org/wiki/C%2B%2B11) (aka C++0x) framework inspired by [node.js](https://github.com/joyent/node). 

Please note that <b>node.native</b> project is <em>under heavy development</em>: currently experimental.

## Sample code

Simple echo server (examples/echo.cc):

    #include "native.h"
    using namespace native;
    
    int main(int argc, char** argv) {
        return process::run([=]() {
            net::createServer([](net::Server* server){
                server->on<ev::connection>([](net::Socket* socket){
                    socket->pipe(socket, {});
                });
                server->on<ev::error>([=](Exception e){
                    server->close();
                });
                server->listen(1337, "127.0.0.1");
            });
        });
    }

For more examples look in examples directory.

## Getting started

To compile <b>node.native</b> example apps and tests: 

    make

To build debug version:

    build/configure --debug

To specify custom compilers:

    build/configure --cc=gcc-mp-4.7 --cxx=g++-mp-4.7 

The code was tested on OSX 10.6 with GCC 4.7.2 (installed from Fink and Macports).

## Other Resources

- [GitHub](http://github.com/kyleamartin/node.native)
- [Mailing list](http://groups.google.com/group/nodenative)