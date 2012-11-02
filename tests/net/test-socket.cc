#include "cctest.h"

#include "native.h"

using namespace native;

TEST(Socket) {
	run([=](){
		net::createSocket([](net::Socket* socket){
			socket->on<event::drain>([](){
				std::cout << "drain" << std::endl;
			});
			socket->on<event::connect>([](){
				std::cout << "connect" << std::endl;
			});
			socket->on<event::error>([](const Exception& e){
				std::cout << "error: " << e.message() << std::endl;
			});
			socket->on<event::close>([](){
				std::cout <<  "close" << std::endl;
			});
			socket->on<event::data>([](const Buffer& b){
				std::cout << "data: " << std::string(b.base(), b.size()) << std::endl;
			});
			socket->on<event::end>([](){
				std::cout << "end" << std::endl;
			});
			socket->connect("127.0.0.1", 1337, [](){
				std::cout << "connected" << std::endl;
			});
		});
	});
}
