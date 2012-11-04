
#include "native.h"

namespace native {
namespace http {

Parser::Parser(http_parser_type type, net::Socket* socket)
  : on_incoming_()
  , context_(type)
  , socket_(socket)
  , incoming_(nullptr)
  , alloc_incoming_(nullptr)
{
  // wrap callbacks in closures
  context_.on_headers_complete(
    [this](const native::detail::http_message& result){
      on_headers_complete(result);
    }
  );
  context_.on_body(
    [this](const char* buf, size_t off, size_t len){
      on_body(buf, off, len);
    }
  );
  context_.on_message_complete(
    [this](const native::detail::http_message& result){
      on_message_complete(result);
    }
  );
  context_.on_error(
    [this](const native::Exception& e){
      on_error(e);
    }
  );

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

Parser* Parser::create(
    http_parser_type type,
    net::Socket* socket,
    on_incoming_type callback)
{
  CRUMB();
  Parser* parser(new Parser(type, socket));

  if (callback) parser->on_incoming(callback);

  return parser;
}

void Parser::on_incoming(on_incoming_type callback) {
  on_incoming_ = callback;
}

void Parser::alloc_incoming(alloc_incoming_type callback) {
  alloc_incoming_ = callback;
}

int Parser::on_body(const char* buf, size_t off, size_t len) {
  CRUMB();
  return 0;
}

int Parser::on_error(const native::Exception& e) {
  CRUMB();
  return 0;
}


int Parser::on_headers_complete(const native::detail::http_message& result) {
  CRUMB();
  if (!alloc_incoming_ || !on_incoming_) return 0;

  incoming_ = alloc_incoming_(socket_);

  on_incoming_(incoming_);

  return 0;
}

int Parser::on_message_complete(const native::detail::http_message& result) {
  CRUMB();
  if (incoming_) {
    incoming_->end();
  }
  return 0;
}

} //namespace http
} //namespace native
