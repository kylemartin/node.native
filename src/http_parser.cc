
#include "native/http.h"

namespace native {
namespace http {

#undef DBG
#define DBG(msg) DEBUG_PRINT("[Parser] " << msg)

/*
 * Public
 */
Parser* Parser::create(
    http_parser_type type,
    net::Socket* socket,
    on_incoming_type incoming_cb)
{
  DBG("creating parser");
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
  DBG("constructing parser");

  registerEvent<event::error>();
  registerEvent<event::end>();
  registerEvent<event::close>();

  registerContextCallbacks();
  registerSocketEvents();
}

void Parser::registerContextCallbacks() {

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
}

void Parser::registerSocketEvents() {
  // Register net::Socket event handlers
  socket_->on<native::event::data>([=](const Buffer& buf) {
    DBG("socket data");
      if(context_.feed_data(buf.base(), 0, buf.size()))
      {
        DBG("parsing finished, pausing socket");
        // parse end
        socket_->pause();
      }
      else
      {
        DBG("parsing not finished, continue reading socket");
        // more reads required: keep reading!
      }
  });

  socket_->on<native::event::end>([=](){
    DBG("socket end");
    // EOF
    if(!context_.is_finished())
    {
      DBG("socket end before parsing finished");
      // HTTP request was not properly parsed.
      emit<event::error>(Exception("socket end before parsing finished"));
    }
    emit<event::end>();
  });

  socket_->on<native::event::error>([=](const native::Exception& e){
    DBG("socket error");
    emit<event::error>(e);
  });

  socket_->on<native::event::close>([=](){
    DBG("socket close");
    // EOF
    if(!context_.is_finished())
    {
      DBG("socket close before parsing finished");
      // HTTP request was not properly parsed.
      emit<event::error>(Exception("socket close before parsing finished"));
    }
    emit<event::close>();
  });

  socket_->on<event::connect>([=]() {
    DBG("socket connect");
  });
  socket_->on<event::timeout>([=](){
    DBG("socket timeout");
  });
  socket_->on<event::drain>([=](){
    DBG("socket drain");
  });
}

int Parser::on_error(const native::Exception& e) {
  DBG("error");
  emit<event::error>(e);
  return 0;
}

int Parser::on_headers_complete() {
  DBG("headers complete");
  assert(on_incoming_);
  incoming_ = on_incoming_(socket_, message_);
  return 0;
}

int Parser::on_body(const char* buf, size_t off, size_t len) {
  DBG("body");
  assert(incoming_);
  incoming_->emit<event::data>(Buffer(buf+off,len));
  return 0;
}

int Parser::on_message_complete() {
  DBG("message complete");
  assert(incoming_);
  incoming_->end();
  return 0;
}

} //namespace http
} //namespace native
