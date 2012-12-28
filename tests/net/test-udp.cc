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

	  socket = udp::createSocket(UDP4,[](const Buffer& buf, std::shared_ptr<detail::net_addr> address){
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
		  Buffer buf ("message" + std::to_string(sent));
		  std::cout << "sending datagram: " << buf.str() << std::endl << std::flush;
		  try{
		    socket->send(buf,0,buf.size(),COMMON_PORT,"127.0.0.1",
		        [&](const detail::resval& r){
		          std::cout << "send completed with status: " << r.str() << std::endl << std::flush;
              if(++sent >= 2) {
                clearInterval(interval);
                socket->close();
              }
		        }
		    );
		  } catch(Exception* e) {
		    std::cerr << e->message() << std::endl << std::flush;
		  }
		}, 100);
	});

	CHECK(sent == 2);
}


TEST(UDPReceive) {
  int received = 0;
  timer* interval = nullptr;
  udp* socket = nullptr;
  process::run([&](){
    std::cout << "Running UDPReceive" << std::endl << std::flush;

    socket = udp::createSocket(UDP4,[&](const Buffer& buf, std::shared_ptr<detail::net_addr> address){
      std::cout << "received datagram: " << buf.str() << std::endl << std::flush;
      if(++received >= 2) {
        socket->close();
      }
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

    socket->bind(COMMON_PORT);
  });

  CHECK(received == 2);
}
