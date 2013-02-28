#include "native.h"
using namespace native;

int main(int argc, char** argv) {
    return process::run([=]() {
        net::Server* server = net::createServer();
        server->on<event::connection>([](net::Socket* socket){
            socket->pipe(socket);
        });
        server->on<event::error>([=](Exception e){
            server->close();
        });
        server->listen(1337, "127.0.0.1");
    });
}
