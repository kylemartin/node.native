
#include "native/http.h"

namespace native {
namespace http {

#undef DBG
#define DBG(msg) DEBUG_PRINT("[IncomingMessage] " << msg)

IncomingMessage::IncomingMessage(net::Socket* socket,
    detail::http_message* message) :
    socket_(socket), parser_(nullptr), complete_(false), readable_(true), paused_(
        false), pendings_(), endEmitted_(false), message_(message) {
  DBG("constructing");
  assert(socket);
  // ReadableStream events
  registerEvent<native::event::data>();
  registerEvent<native::event::end>();
  registerEvent<native::event::error>();
  registerEvent<native::event::close>();
}

// ReadbleStream interface

bool IncomingMessage::readable() {
  return false;
}

void IncomingMessage::pause() {
  CRUMB();
}
void IncomingMessage::resume() {
  CRUMB();
}

void IncomingMessage::destroy(const Exception& e) {
  socket_->destroy(e);
}
// ReadbleStream interface

void IncomingMessage::_emitPending(std::function<void()> callback) {
  CRUMB();
}
void IncomingMessage::_emitData(const Buffer& buf) {
  CRUMB();
}
void IncomingMessage::_emitEnd() {
  CRUMB();
}
void IncomingMessage::_addHeaderLine(const std::string& field,
    const std::string& value) {
  CRUMB();
}

/*
 * Accessors
 */

void IncomingMessage::parser(Parser* parser) {
  parser_ = parser;
}
Parser* IncomingMessage::parser() {
  return parser_;
}

net::Socket* IncomingMessage::socket() {
  return socket_;
}

// Read-only HTTP message
int IncomingMessage::statusCode() {
  return message_->status();
}
const http_method& IncomingMessage::method() {
  return message_->method();
}
const detail::http_version& IncomingMessage::httpVersion() {
  return message_->version();
}
std::string IncomingMessage::httpVersionString() {
  return message_->version_string();
}
int IncomingMessage::httpVersionMajor() {
  return message_->version_major();
}
int IncomingMessage::httpVersionMinor() {
  return message_->version_minor();
}
const detail::url_obj& IncomingMessage::url() {
  return message_->url();
}

const headers_type& IncomingMessage::headers() {
  return message_->headers();
}
const headers_type& IncomingMessage::trailers() {
  return message_->trailers();
}

bool IncomingMessage::shouldKeepAlive() {
  return message_->should_keep_alive();
}
bool IncomingMessage::upgrade() {
  return message_->upgrade();
}

const std::string& IncomingMessage::getHeader(const std::string& name) {
  return message_->get_header(name);
}
const std::string& IncomingMessage::getTrailer(const std::string& name) {
  return message_->get_trailer(name);
}

void IncomingMessage::end() {
  CRUMB();
  if (haveListener<native::event::end>()) {
    emit<native::event::end>();
  }
}

} //namespace http
} //namespace native
