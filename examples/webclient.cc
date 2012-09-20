/*
 * webclient.cc
 *
 *  Created on: Jul 14, 2012
 *      Author: kmartin
 */

#include "native.h"
using namespace native;

int main(int argc, char** argv) {
    return run([=]() {
    	http::ClientRequest* req = http::request("http://127.0.0.1:1337/");

		req->on<http::event::socket>([](net::Socket* socket){
			std::cout << "[req] on socket" << std::endl;
		});
		req->on<http::event::response>([](http::ClientResponse* res){
			std::cout << "[req] on response" << std::endl;
			res->on<event::data>([](const Buffer& buf){
				std::cout << "[res] on data: " << std::string(buf.base(), buf.size());
			});
//                res->end(Buffer(std::string("path: ")+req->path()+"\r\n"));
		});
		req->on<event::error>([](const Exception& e){
			std::cout << "[req] on error: " << e.message() << std::endl;
		});

		req->end();
    });
}
