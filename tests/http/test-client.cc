#include "cctest.h"
#include "common.h"

#include "native.h"

using namespace native;

TEST(Client) {
  run([]() {

    http::ClientRequest* req =
        http::get("http://127.0.0.1:" COMMON_PORT_STRING "/foo?bar");

    /**
     * WritableStream Events
     */
    req->on<event::drain>([]() {
          std::cerr << "[req] on drain" << std::endl;
        });
    req->on<event::error>([](const Exception& e) {
          std::cerr << "[req] on error: " << e.message() << std::endl;
        });
    req->on<event::close>([]() {
          std::cerr << "[req] on close" << std::endl;
        });
    req->on<event::pipe>([](Stream* socket) {
          std::cerr << "[req] on pipe" << std::endl;
        });

    /**
     * ClientRequest Events
     */
    req->on<native::event::http::socket>([](net::Socket* socket) {
          std::cerr << "[req] on socket" << std::endl;
        });
    req->on<event::http::client::connect>([](http::ClientResponse* response,
        net::Socket* socket, const Buffer& head) {
          std::cerr << "[req] on connect" << std::endl;
        });
    req->on<event::http::client::upgrade>([](http::ClientResponse* response,
        net::Socket* socket, const Buffer& head) {
          std::cerr << "[req] on upgrade" << std::endl;
        });
    req->on<native::event::http::Continue>([]() {
          std::cerr << "[req] on continue" << std::endl;
        });

    req->on<native::event::http::client::response>(
        [](http::ClientResponse* res) {
          std::cerr << "[req] on response" << std::endl;

          /**
           * ReadableStream Events
           */
          res->on<event::data>([](const Buffer& buf) {
                std::cerr << "[res] on data: "
                    << buf.str() << std::endl;
              });
          res->on<event::end>([]() {
                std::cerr << "[res] on end" << std::endl;
              });
          res->on<event::error>([](const Exception& e) {
                std::cerr << "[res] on error: " << e.message() << std::endl;
              });
          res->on<event::close>([]() {
                std::cerr << "[res] on close" << std::endl;
              });
        });

//    std::cerr << "[req] ending" << std::endl;
//    req->end();

  });
}
