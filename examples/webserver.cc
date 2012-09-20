#include "native.h"
using namespace native;

int main(int argc, char** argv) {
	DBG("testing");
    return run([=]() {
        http::Server* server = http::createServer();

		server->on<event::connection>([](net::Socket* socket){
			DBG("server->on<event::connection>");
		});
		server->on<http::event::request>([](http::ServerRequest* req, http::ServerResponse* res){
			DBG("server->on<http::event::request>");
			res->end(Buffer(std::string("response!\r\n")));
		});
		server->on<event::error>([](const Exception& e){
			DBG("server->on<event::error>: " << e.message());
		});
        server->on<http::event::checkContinue>([](http::ServerRequest* req, http::ServerResponse* res){
        	DBG("server->on<http::event::checkContinue>");
        });
        server->on<http::event::upgrade>([](http::ServerRequest* req, net::Socket* socket, const Buffer& buf){
        	DBG("server->on<http::event::upgrade>");
        });
        server->on<native::event::close>([](){
        	DBG("server->on<native::event::close>");
        });
        server->on<native::event::error>([](const Exception& e){
        	DBG("server->on<native::event::error>");
        });
		server->listen(1337, "127.0.0.1");
    });
}
