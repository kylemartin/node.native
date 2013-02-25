
#include "native/http.h"

namespace native {
namespace http {

#undef DBG
#define DBG(msg) DEBUG_PRINT("[IncomingMessage] " << msg)

IncomingMessage::IncomingMessage(net::Socket* socket, Parser* parser)
: socket_(socket)
, parser_(parser)
, complete_(false)
, readable_(true)
, paused_(false)
, pendings_()
, endEmitted_(false)
{
CRUMB();
  assert(socket);
  // ReadableStream events
  registerEvent<native::event::data>();
  registerEvent<native::event::end>();
  registerEvent<native::event::error>();
  registerEvent<native::event::close>();

  // IncomingMessage events
  registerEvent<native::event::http::headers>();
  registerEvent<native::event::http::trailers>();
}

// ReadbleStream interface

/**
 * True by default, but turns false after an 'error' occurred, the stream came
 * to an 'end', or destroy() was called.
 * @return
 */
bool IncomingMessage::readable() {
  return readable_;
}

void IncomingMessage::pause() {
  socket_->pause();
  paused_ = true;
}
void IncomingMessage::resume() {
  assert(socket_);
  socket_->resume();

  _emitPending(nullptr);
}

void IncomingMessage::destroy(const Exception& e) {
  socket_->destroy(e);
}

/**
 * Emit data events that were queued after pausing socket
 * @param callback
 */
void IncomingMessage::_emitPending(std::function<void()> callback) {
  if (pendings_.size() > 0) {
    for (std::vector<Buffer>::iterator it = pendings_.begin();
        it != pendings_.end(); ++it) {
      // TODO: do this on next tick
      emit<native::event::data>(*it);
    }
  }
}
void IncomingMessage::_emitData(const Buffer& buf) {
  emit<native::event::data>(buf);
}
void IncomingMessage::_emitEnd() {
  if (!endEmitted_) emit<native::event::end>();
  endEmitted_ = true;
}
void IncomingMessage::_addHeaderLine(const std::string& field,
    const std::string& value) {
}

/*
 * Accessors
 */

Parser* IncomingMessage::parser() {
  return parser_;
}

net::Socket* IncomingMessage::socket() {
  return socket_;
}

// Read-only HTTP message
int IncomingMessage::statusCode() {
  return parser_->start_line().status();
}
const http_method& IncomingMessage::method() {
  return parser_->start_line().method();
}
const detail::http_version& IncomingMessage::httpVersion() {
  return parser_->start_line().version();
}
std::string IncomingMessage::httpVersionString() {
  return parser_->start_line().version_string();
}
int IncomingMessage::httpVersionMajor() {
  return parser_->start_line().version_major();
}
int IncomingMessage::httpVersionMinor() {
  return parser_->start_line().version_minor();
}
const detail::url_obj& IncomingMessage::url() {
  return parser_->start_line().url();
}

bool IncomingMessage::shouldKeepAlive() {
  return parser_->should_keep_alive();
}
bool IncomingMessage::upgrade() {
  return parser_->upgrade();
}

void IncomingMessage::end() {
  if (haveListener<native::event::end>()) {
    emit<native::event::end>();
  }
}

/**
 * Called from a Parser when a header/trailer received
 * @param name
 * @param value
 */
void IncomingMessage::parser_add_header(const std::string& name, const std::string& value) {
  bool append = false; // TODO: check headers to append by default
  if (!body_) {
    headers_[name] =
        (has_header(name) && append) ? headers_[name] + "," + value : value;
  } else {
    trailers_[name] =
        (has_trailer(name) && append) ? trailers_[name] + "," + value : value;
  }
}

const headers_type& IncomingMessage::headers() const {
  return headers_;
}

bool IncomingMessage::has_header(const std::string& name) {
  return headers_.count(name) > 0;
}

const std::string& IncomingMessage::get_header(const std::string& name) {
  headers_type::iterator it = headers_.find(name);
  if (it == headers_.end()) {
    // TODO: throw proper error
//    throw Exception("header not found");
  }
  return it->second;
}

const headers_type& IncomingMessage::trailers() const {
  return headers_;
}

bool IncomingMessage::has_trailer(const std::string& name) {
  return trailers_.count(name) > 0;
}

const std::string& IncomingMessage::get_trailer(const std::string& name) {
  headers_type::iterator it = trailers_.find(name);
  if (it == trailers_.end()) {
    // TODO: throw proper error
//    throw Exception("header not found");
  }
  return it->second;
}

/**
 * Called from the Parser when a body chunk is received
 * @param body
 */
void IncomingMessage::parser_on_body(const Buffer& body) {
  body_ = true;
  if (paused_) {
    // If paused socket, store pending buffer
    pendings_.push_back(body);
  } else {
    // Otherwise emit data event
    emit<native::event::data>(body);
  }
}

/**
 * Called from the Parser when an error occurs
 * @param e
 */
void IncomingMessage::parser_on_error(const Exception& e) {
  emit<native::event::error>(e);
}

void IncomingMessage::parser_on_headers_complete() {
}

} //namespace http
} //namespace native
