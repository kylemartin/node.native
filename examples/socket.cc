/*
 * socket.cc
 *
 *  Created on: Jul 14, 2012
 *      Author: kmartin
 */

#include "native.h"
using namespace native;

int main(int argc, char** argv) {
	return run([=](){
		net::createSocket([](net::Socket* socket){
			socket->on<event::drain>([](){
				std::cout << "drain" << std::endl;
			});
			socket->on<event::connect>([](){
				std::cout << "connect event" << std::endl;
			});
			socket->on<event::error>([](const Exception& e){
				std::cout << "error: " << e.message() << std::endl;
			});
			socket->on<event::close>([](){
				std::cout <<  "close" << std::endl;
			});
			socket->on<event::data>([](const Buffer& b){
				std::cout << "data: " << b.str() << std::endl;
			});
			socket->on<event::end>([](){
				std::cout << "end" << std::endl;
			});
			socket->connect("127.0.0.1", 1337, [](){
				std::cout << "connect callback" << std::endl;
			});
		});
	});
}
