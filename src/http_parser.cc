
#define DEBUG_ENABLED

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

  registerEvent<native::event::error>();

  // Wrap callbacks registered with detail::http_parser_context in closures
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

  // Register net::Socket event handlers
  socket->on<native::event::data>([=](const Buffer& buf) {
    DBG("socket data");
      if(context_.feed_data(buf.base(), 0, buf.size()))
      {
        DBG("parsing finished, pausing socket");
        // parse end
        socket->pause();
      }
      else
      {
        DBG("parsing not finished, continue reading socket");
        // more reads required: keep reading!
      }
  });

  socket->on<native::event::end>([=](){
    DBG("socket end");
    // EOF
    if(!context_.is_finished())
    {
      DBG("socket end before parsing finished");
      // HTTP request was not properly parsed.
      emit<event::error>(Exception("socket end before parsing finished"));
    }
  });

  socket->on<native::event::error>([=](const native::Exception& e){
    DBG("socket error");
    emit<event::error>(e);
  });

  socket->on<native::event::close>([=](){
    DBG("socket close");
    // EOF
    if(!context_.is_finished())
    {
      DBG("socket close before parsing finished");
      // HTTP request was not properly parsed.
      emit<event::error>(Exception("socket close before parsing finished"));
    }
  });

  socket->on<event::connect>([=]() {
    DBG("socket connect");
  });
  socket->on<event::timeout>([=](){
    DBG("socket timeout");
  });
  socket->on<event::drain>([=](){
    DBG("socket drain");
  });

}

int Parser::on_body(const char* buf, size_t off, size_t len) {
  DBG("parser body");
  return 0;
}

int Parser::on_error(const native::Exception& e) {
  DBG("parser error");
  return 0;
}


int Parser::on_headers_complete() {
  DBG("parser headers complete");
  if (!on_incoming_) return 0;

  incoming_ = on_incoming_(socket_, message_);

  return 0;
}

int Parser::on_message_complete() {
  DBG("parser message complete");
  if (incoming_) {
    incoming_->end();
  }
  return 0;
}

} //namespace http
} //namespace native
