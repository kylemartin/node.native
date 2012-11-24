#include "cctest.h"
#include "common.h"

#include "native.h"

using namespace native;

TEST(Socket) {
	int clientConnected = 0;
	run([&](){
		auto cb = [&](net::Socket* socket){
			socket->connect("127.0.0.1", COMMON_PORT, [&](){
				std::cout << "client connected" << std::endl;
				++clientConnected;
			});
		};
		net::createSocket(cb);
		net::createSocket(cb);
//		net::createSocket([&](net::Socket* socket){
//			socket->on<event::drain>([](){
//				std::cout << "drain" << std::endl;
//			});
//			socket->on<event::connect>([](){
//				std::cout << "connect" << std::endl;
//			});
//			socket->on<event::error>([](const Exception& e){
//				std::cout << "error: " << e.message() << std::endl;
//			});
//			socket->on<event::close>([](){
//				std::cout <<  "close" << std::endl;
//			});
//			socket->on<event::data>([](const Buffer& b){
//				std::cout << "data: " << std::string(b.base(), b.size()) << std::endl;
//			});
//			socket->on<event::end>([](){
//				std::cout << "end" << std::endl;
//			});
//			socket->connect("127.0.0.1", common::PORT, cb);
//		});
	});

	CHECK(clientConnected == 2);
}



// server.listen(tcpPort, 'localhost', function() {
//   function cb() {
//     ++clientConnected;
//   }

//   net.createConnection(tcpPort).on('connect', cb);
//   net.createConnection(tcpPort, 'localhost').on('connect', cb);
//   net.createConnection(tcpPort, cb);
//   net.createConnection(tcpPort, 'localhost', cb);
// });
