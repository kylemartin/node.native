
#include "native/http.h"

namespace native {
namespace http {

using detail::url_obj;
using detail::resval;

#undef DBG
#define DBG(msg) DEBUG_PRINT("[Parser] " << msg)


/******************************************************************************
 * Public
 ******************************************************************************/

/**
 * Create a parser of the specified type attached to the given socket
 * which invokes the given callback to create an IncomingMessage instance.
 *
 * @param socket
 * @param type
 * @param callback
 * @return
 */
Parser* Parser::create(
    http_parser_type type,
    net::Socket* socket,
    on_incoming_type incoming_cb)
{
  DBG("creating parser");
  Parser* parser(new Parser(type, socket));

  if (incoming_cb) parser->register_on_incoming(incoming_cb);
  return parser;
}

/**
 * Set the callback for allocating an incoming message
 * @param callback [description]
 */
void Parser::register_on_incoming(on_incoming_type callback) {
  on_incoming_ = callback;
}

void Parser::register_on_error(on_error_type callback) {
  on_error_ = callback;
}

void Parser::register_on_close(on_close_type callback) {
  on_close_ = callback;
}

void Parser::register_on_end(on_end_type callback) {
  on_end_ = callback;
}

/* Accessors ******************************************************************/

bool Parser::parsing() const {
  // there was an error or parse completed.
  return !error_ && parsing_;
}

bool Parser::upgrade() const {
  return upgrade_;
}

bool Parser::should_keep_alive() const {
  return should_keep_alive_;
}

const http_start_line& Parser::start_line() const {
  return start_line_;
}

/******************************************************************************
 * Private
 ******************************************************************************/

/* Initialization *************************************************************/

/**
 * Private constructor
 * @param type   http_parser_type value (HTTP_{REQUEST,RESPONSE,BOTH})
 * @param socket net::Socket to be read by parser
 */
Parser::Parser(http_parser_type type, net::Socket* socket)
: socket_(socket)
, on_incoming_()
, incoming_(nullptr)
, parser_()
, settings_()
, was_header_value_(true)
, last_header_field_()
, last_header_value_()
, error_()
, parsing_(false)
{
  DBG("constructing parser");

  registerSocketEvents();

  http_parser_init(&parser_, type);
  parser_.data = this;
  settings_.on_message_begin = on_message_begin_;
  settings_.on_url = on_url_;
  settings_.on_header_field = on_header_field_;
  settings_.on_header_value = on_header_value_;
  settings_.on_headers_complete = on_headers_complete_;
  settings_.on_body = on_body_;
  settings_.on_message_complete = on_message_complete_;
}

Parser::~Parser() {
}
/**
 * Register socket event handlers
 */
void Parser::registerSocketEvents() {
  // Register net::Socket event handlers
  socket_->on<native::event::data>([=](const Buffer& buf) {
    DBG("socket data");
      if(feed_data(buf.base(), 0, buf.size()))
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
    if(parsing())
    {
      DBG("socket end before parsing finished");
      // HTTP request was not properly parsed.
//      emit<event::error>(Exception("socket end before parsing finished"));
      on_close_();
    }
    on_end_();
  });

  socket_->on<native::event::error>([=](const native::Exception& e){
    DBG("socket error");
    on_error_(e);
  });

  socket_->on<native::event::close>([=](){
    DBG("socket close");
    // EOF
    if(parsing())
    {
      DBG("socket close before parsing finished");
      // HTTP request was not properly parsed.
      on_error_(Exception("socket close before parsing finished"));
    }
    on_close_();
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

/* Headers ********************************************************************/

/**
 * Add a header to the buffer, flushing if MAX_HEADERS_COUNT reached
 * @param name
 * @param value
 * @param append
 */
void Parser::add_header(const std::string& name, const std::string& value) {
  // If no IncomingMessage created yet
  if (!incoming_) {
    // Prepare http_start_line and parser state
    /* Common */

    // HTTP version
    start_line_.version(parser_.http_major, parser_.http_minor);

    /* Response */

    // HTTP status
    start_line_.status(parser_.status_code);

    /* Request */

    // TODO: handle host header HTTP 1.1
    //  std::string host("");
    //  int port = 80; // default port
    //  // HTTP 1.1 requires "Host" header
    //  if (start_line_.version() == detail::HTTP_1_1) {
    //    // get host and port info from header entry "Host"
    //    if (has_header("host")) {
    //      auto s = get_header("host");
    //      auto colon = s.find_last_of(':');
    //      if (colon == s.npos) {
    //        host = s;
    //      } else {
    //        host = s.substr(0, colon);
    //        port = std::stoi(s.substr(colon + 1));
    //      }
    //    }
    //  }

    // HTTP method
    start_line_.method(static_cast<http_method>(parser_.method));

    /* Headers */

    // HTTP keep-alive
    should_keep_alive_ = http_should_keep_alive(&parser_);

    // HTTP upgrade
    upgrade_ = parser_.upgrade > 0;

    // We must have an IncomingMessage factory function registered
    // TODO: replace asserts with errors if IncomingMessage factory not registered or fails to create IncomingMessage
    assert(on_incoming_);
    // Create a new IncomingMessage
    incoming_ = on_incoming_(socket_, this);
    assert(incoming_);
  }

  incoming_->parser_add_header(name, value);
}

/* http_parser Management *****************************************************/

/**
 * Feed data read from socket into http_parser
 * @param data
 * @param offset
 * @param length
 * @return
 */
bool Parser::feed_data(const char* data, std::size_t offset,
    std::size_t length) {
  ::http_parser_execute(&parser_, &settings_, &data[offset], length);
  if (parser_.http_errno != HPE_OK) {
    // there was an error
    on_error(resval(parser_.http_errno));
    return true;
  } else if (!parsing()) {
    // finished, should have invoked on_message_complete already
    return true;
  } else {
    // need more data
    return false;
  }
}

/**
 * Reset the parser state for a new message
 */
void Parser::reset() {
  // Reset parser state
  start_line_.reset();

  parsing_ = true;
}

/* http_parser Callbacks ******************************************************/

/**
 * When http_parser has started receiving a new message reset the Parser state
 * @param parser
 * @return
 */
int Parser::on_message_begin_(http_parser* parser) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);
  self->reset();
  return 0;
}

/**
 * When http_parser has received an url, set the url on the http_start_line
 * @param parser
 * @param at
 * @param len
 * @return
 */
int Parser::on_url_(http_parser* parser, const char *at,
    size_t len) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);

  assert(at && len);

  if (!self->start_line_.url(at, len)) {
    self->error_ = resval(error::http_parser_url_fail);
    return 1;
  }

  return 0;
}

/**
 * When http_parser receives a header field, store it in the headers buffer
 * @param parser
 * @param at
 * @param len
 * @return
 */
int Parser::on_header_field_(http_parser* parser, const char* at,
    size_t len) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);

  if (self->was_header_value_) {
    // new field started
    if (!self->last_header_field_.empty()) {
      // add new entry
      self->add_header(self->last_header_field_, self->last_header_value_);
      self->last_header_value_.clear();
    }

    self->last_header_field_ = std::string(at, len);
    self->was_header_value_ = false;
  } else {
    // appending
    self->last_header_field_ += std::string(at, len);
  }

  return 0;
}

/**
 * When http_parser receives a header value, store it in the headers buffer
 * @param parser
 * @param at
 * @param len
 * @return
 */
int Parser::on_header_value_(http_parser* parser, const char* at,
    size_t len) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);

  if (!self->was_header_value_) {
    self->last_header_value_ = std::string(at, len);
    self->was_header_value_ = true;
  } else {
    // appending
    self->last_header_value_ += std::string(at, len);
  }

  return 0;
}

/**
 * When http_parser finishes receiving headers, add last header to buffer and
 * signal that headers complete
 * @param parser
 * @return
 */
int Parser::on_headers_complete_(http_parser* parser) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);
  assert(&self->parser_ == parser);

  // add last entry if any
  if (!self->last_header_field_.empty()) {
    // add new entry
    self->add_header(self->last_header_field_,
        self->last_header_value_);
  }

  assert(self->incoming_);
  self->incoming_->parser_on_headers_complete();

  return 0;
}

/**
 * When http_parser receives a body chunk, signal that body received
 * @param parser
 * @param at
 * @param len
 * @return
 */
int Parser::on_body_(http_parser* parser, const char* at,
    size_t len) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);

  // Call handle_body
  self->on_body(at, 0, len);
  return 0;
}

/**
 * When http_parser finished receiving a message, signal message complete
 * @param parser
 * @return
 */
int Parser::on_message_complete_(http_parser* parser) {
  auto self = reinterpret_cast<Parser*>(parser->data);
  assert(self);

  // Call handle_message_complete
  self->on_message_complete();

  self->parsing_ = false;

  return 0;
}

/* Events *********************************************************************/

int Parser::on_error(const native::Exception& e) {
  DBG("error");
  if (incoming_) incoming_->parser_on_error(e);
  else on_error_(e);
  return 0;
}

int Parser::on_body(const char* buf, size_t off, size_t len) {
  DBG("body");
  assert(incoming_);
  incoming_->parser_on_body(Buffer(buf+off,len));
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
