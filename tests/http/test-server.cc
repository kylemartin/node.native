#include "cctest.h"
#include "common.h"

#include "native.h"

using namespace native;

TEST(Server) {
  int responses_sent = 0;
  run([&responses_sent]() {
    http::Server* server = http::createServer();

    server->on<native::event::http::server::request>([=, &responses_sent](http::ServerRequest* req, http::ServerResponse* res){
      std::cerr << "[server] on request" << std::endl;

      if (responses_sent == 0) {
    	  CHECK_EQ(req->method(), HTTP_GET);
    	  CHECK_EQ("/hello", req->url().path().c_str());
    	  CHECK(req->headers().count("Accept") == 1);
    	  CHECK(req->headers().find("Accept")->second == "*/*");
    	  CHECK(req->headers().count("Foo") == 1);
    	  CHECK(req->headers().find("Foo")->second == "bar");
        CHECK(req->headers().count("Connection") == 1);
        CHECK(req->headers().find("Connection")->second == "keep-alive");
    	  std::cerr << "headers:" << std::endl;
    	  for (http::headers_type::const_iterator it = req->headers().begin();
    	      it != req->headers().end(); ++it) {
    	    std::cerr << "  " << it->first << " = " << it->second << std::endl;
    	  }
      }

      if (responses_sent == 1) {
    	  CHECK_EQ(req->method(), HTTP_POST);
    	  CHECK_EQ("/world", req->url().path().c_str());
        CHECK(req->headers().count("Connection") == 1);
        CHECK(req->headers().find("Connection")->second == "keep-alive");
        CHECK(req->headers().count("Transfer-Encoding") == 1);
        CHECK(req->headers().find("Transfer-Encoding")->second == "chunked");
        std::cerr << "headers:" << std::endl;
        for (http::headers_type::const_iterator it = req->headers().begin();
            it != req->headers().end(); ++it) {
          std::cerr << "  " << it->first << " = " << it->second << std::endl;
        }
      }

      req->on<event::end>([=, &responses_sent](){
        std::cerr << "[req] on end" << std::endl;
        res->writeHead(200, {{"Content-Type", "text/plain"}});
        res->write("The path was " + req->url().path());
        CRUMB();
        res->end();
        CRUMB();
        responses_sent += 1;
        if (responses_sent == 2)
          server->close();
      });
//
//      /**
//       * ReadableStream Events
//       */
//      req->on<event::data>([](const Buffer& buf){
//        std::cout << "[req] on data: " << std::string(buf.base(), buf.size());
//      });
//      req->on<event::end>([](){
//        std::cout << "[req] on end" << std::endl;
//      });
//      req->on<event::error>([](const Exception& e){
//        std::cout << "[req] on error: " << e.message() << std::endl;
//      });
//      req->on<event::close>([](){
//        std::cout << "[req] on close" << std::endl;
//      });
//
//      /**
//       * WritableStream Events
//       */
//      res->on<event::drain>([](){
//        std::cout << "[res] on drain" << std::endl;
//      });
//      res->on<event::error>([](const Exception& e){
//        std::cout << "[res] on error: " << e.message() << std::endl;
//      });
//      res->on<event::close>([](){
//        std::cout << "[res] on close" << std::endl;
//      });
//      res->on<event::pipe>([](Stream* socket){
//        std::cout << "[res] on pipe" << std::endl;
//      });
//
//      res->writeHead(200,"",http::headers_type({{"Content-Type", "text/plain"}}));
//      res->end(Buffer(std::string("Hello World!\n")));
    });

//    server->on<event::connection>([](net::Socket* socket){
//      std::cout << "[server] on connection" << std::endl;
//    });
//    server->on<native::event::close>([](){
//      std::cout << "[server] on close" << std::endl;
//    });
//    server->on<native::event::http::server::checkContinue>([](http::ServerRequest* req, http::ServerResponse* res){
//      std::cout << "[server] on checkContinue" << std::endl;
//    });
//    server->on<native::event::http::server::connect>([](http::ServerRequest* req, net::Socket* socket, const Buffer& buf){
//      std::cout << "[server] on connect" << std::endl;
//    });
//    server->on<native::event::http::server::upgrade>([](http::ServerRequest* req, net::Socket* socket, const Buffer& buf){
//      std::cout << "[server] on upgrade" << std::endl;
//    });
//    server->on<event::clientError>([](const Exception& e){
//      std::cout << "[server] on clientError: " << e.message() << std::endl;
//    });

    server->listen(COMMON_PORT, "127.0.0.1");
  });
  CHECK(responses_sent == 2);
}

// var server = http.Server(function(req, res) {
//   if (responses_sent == 0) {
//     assert.equal('GET', req.method);
//     assert.equal('/hello', url.parse(req.url).pathname);

//     console.dir(req.headers);
//     assert.equal(true, 'accept' in req.headers);
//     assert.equal('*/*', req.headers['accept']);

//     assert.equal(true, 'foo' in req.headers);
//     assert.equal('bar', req.headers['foo']);
//   }

//   if (responses_sent == 1) {
//     assert.equal('POST', req.method);
//     assert.equal('/world', url.parse(req.url).pathname);
//     this.close();
//   }

//   req.on('end', function() {
//     res.writeHead(200, {'Content-Type': 'text/plain'});
//     res.write('The path was ' + url.parse(req.url).pathname);
//     res.end();
//     responses_sent += 1;
//   });

//   //assert.equal('127.0.0.1', res.connection.remoteAddress);
// });
// server.listen(common.PORT);
