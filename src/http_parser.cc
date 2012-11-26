
#include "native/http.h"

namespace native {
namespace http {

/*
 * Public
 */
Parser* Parser::create(
    http_parser_type type,
    net::Socket* socket,
    on_incoming_type incoming_cb)
{
  CRUMB();
  Parser* parser(new Parser(type, socket));

  if (incoming_cb) parser->on_incoming(incoming_cb);
  return parser;
}

void Parser::on_incoming(on_incoming_type callback) {
  on_incoming_ = callback;
}

/*
 * Private
 */

Parser::Parser(http_parser_type type, net::Socket* socket)
  : message_(new detail::http_message)
  , context_(type, message_)
  , socket_(socket)
  , incoming_(nullptr)
  , on_incoming_()
{
  CRUMB();
  // wrap callbacks in closures
  context_.on_headers_complete(
    [this](){
      on_headers_complete();
    }
  );
  context_.on_body(
    [this](const char* buf, size_t off, size_t len){
      on_body(buf, off, len);
    }
  );
  context_.on_message_complete(
    [this](){
      on_message_complete();
    }
  );
  context_.on_error(
    [this](const native::Exception& e){
      on_error(e);
    }
  );
  CRUMB();
  // Register socket event handlers
  socket->on<native::event::data>([=](const Buffer& buf) {
    CRUMB();
      if(context_.feed_data(buf.base(), 0, buf.size()))
      {
        CRUMB();
        // parse end
        socket->pause();
      }
      else
      {
        CRUMB();
        // more reads required: keep reading!
      }
  });

  socket->on<native::event::end>([=](){
    CRUMB();
    // EOF
    if(!context_.is_finished())
    {
      // HTTP request was not properly parsed.
    }
  });

  socket->on<native::event::error>([=](const native::Exception& e){
    CRUMB();
  });

  socket->on<native::event::close>([=](){
    CRUMB();
  });
}

int Parser::on_body(const char* buf, size_t off, size_t len) {
  CRUMB();
  return 0;
}

int Parser::on_error(const native::Exception& e) {
  CRUMB();
  return 0;
}


int Parser::on_headers_complete() {
  CRUMB();
  if (!on_incoming_) return 0;

  incoming_ = on_incoming_(socket_, message_);

  return 0;
}

int Parser::on_message_complete() {
  CRUMB();
  if (incoming_) {
    incoming_->end();
  }
  return 0;
}

} //namespace http
} //namespace native
