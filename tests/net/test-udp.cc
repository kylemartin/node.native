#include "cctest.h"
#include "common.h"

#include "native.h"

using namespace native;

TEST(UDPSend) {
	int sent = 0;
	timer* interval = nullptr;
	udp* socket = nullptr;
	process::run([&](){
	  std::cout << "Running UDPSend" << std::endl << std::flush;

	  socket = udp::createSocket(UDP4,[](){
	    CHECK(false == "should not have received datagram!");
	  });

	  socket->on<event::listening>([](){
	    std::cout << "socket listening" << std::endl << std::flush;
	  });

	  socket->on<event::error>([](const Exception& e){
	    std::cout << "error: " << e.message() << std::endl << std::flush;
	  });

	  socket->on<event::close>([](){
	    std::cout << "socket closed" << std::endl << std::flush;
	  });

	  socket->bind();

		interval = setInterval([&](){
		  std::cout << "sent = " << sent << std::endl << std::flush;
		  Buffer buf ("message" + std::to_string(sent));
		  try{
		    socket->send(buf,0,buf.size(),COMMON_PORT,"127.0.0.1");
		  } catch(Exception* e) {
		    std::cerr << e->message() << std::endl << std::flush;
		  }
			if(++sent >= 2) {
			  clearInterval(interval);
			}
		}, 100);
	});

	CHECK(sent == 2);
}
