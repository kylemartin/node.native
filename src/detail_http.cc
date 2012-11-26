
#include "native.h"

namespace native {
namespace detail {

url_obj::url_obj() :
    handle_(), buf_() {
}

url_obj::~url_obj() {
}

bool url_obj::parse(const char* buffer, std::size_t length,
    bool is_connect) {
  buf_ = std::string(buffer, length);
  return http_parser_parse_url(buffer, length, is_connect, &handle_) == 0;
}

std::string url_obj::schema() const {
  if (has_schema())
    return buf_.substr(handle_.field_data[UF_SCHEMA].off,
        handle_.field_data[UF_SCHEMA].len);
  return std::string();
}

std::string url_obj::host() const {
  // TODO: if not specified, use host name
  if (has_host())
    return buf_.substr(handle_.field_data[UF_HOST].off,
        handle_.field_data[UF_HOST].len);
  return std::string();
}

int url_obj::port() const {
  if (has_port())
    return static_cast<int>(handle_.port);
  // Return default port based on schema
  else if (has_schema()) {
    if (util::text::compare_no_case(schema(), "HTTPS"))
      return 443;
    else
      return 80;
  }
  return 0;
}

std::string url_obj::path_query_fragment() const {
  std::string res;
  uint16_t beg = 0, end = 0;
  if (has_path()) {
    beg = handle_.field_data[UF_PATH].off;
    end = beg + handle_.field_data[UF_PATH].len;
    if (has_query()) {
      end = handle_.field_data[UF_QUERY].off + handle_.field_data[UF_QUERY].len;
    }
    if (has_fragment()) {
      end = handle_.field_data[UF_FRAGMENT].off +
          handle_.field_data[UF_FRAGMENT].len;
    }
    return buf_.substr(beg,end);
  } else {
    return "/";
  }
}

std::string url_obj::path() const {
  if (has_path())
    return buf_.substr(handle_.field_data[UF_PATH].off,
        handle_.field_data[UF_PATH].len);
  return std::string();
}

std::string url_obj::query() const {
  if (has_query())
    return buf_.substr(handle_.field_data[UF_QUERY].off,
        handle_.field_data[UF_QUERY].len);
  return std::string();
}

std::string url_obj::fragment() const {
  if (has_query())
    return buf_.substr(handle_.field_data[UF_FRAGMENT].off,
        handle_.field_data[UF_FRAGMENT].len);
  return std::string();
}

bool url_obj::has_schema() const {
  return handle_.field_set & (1 << UF_SCHEMA);
}
bool url_obj::has_host() const {
  return handle_.field_set & (1 << UF_HOST);
}
bool url_obj::has_port() const {
  return handle_.field_set & (1 << UF_PORT);
}
bool url_obj::has_path() const {
  return handle_.field_set & (1 << UF_PATH);
}
bool url_obj::has_query() const {
  return handle_.field_set & (1 << UF_QUERY);
}
bool url_obj::has_fragment() const {
  return handle_.field_set & (1 << UF_FRAGMENT);
}

http_message::http_message() :
    version_(), headers_(), trailers_(), method_(), url_(), status_(0),
    upgrade_(false), should_keep_alive_(true), length_() {
}

http_message::http_message(const http_message& c) :
    version_(c.version_), headers_(c.headers_), trailers_(c.trailers_),
    method_(c.method_), url_(c.url_), status_(c.status_), upgrade_(c.upgrade_),
    should_keep_alive_(c.should_keep_alive_), length_(c.length_) {
}

http_message::http_message(http_message&& c)
: version_(std::move(c.version_))
, headers_(std::move(c.headers_))
, trailers_(std::move(c.trailers_))
, method_(std::move(c.method_))
, url_(std::move(c.url_))
, status_(std::move(c.status_))
, upgrade_(std::move(c.upgrade_))
, should_keep_alive_(std::move(c.should_keep_alive_))
, length_(std::move(c.length_))
{}

http_message::~http_message() {
}

const headers_type& http_message::headers() const {
  return headers_;
}

void http_message::set_header(const std::string& name,
    const std::string& value) {
  headers_[name] = value;
}

void http_message::add_headers(const headers_type& value, bool append) {
  for (headers_type::const_iterator it = value.begin(); it != value.end();
      ++it) {
    add_header(it->first, it->second, append);
  }
}

void http_message::add_header(const std::string& name, const std::string& value,
    bool append) {
  headers_[name] =
      (has_header(name) && append) ? headers_[name] + "," + value : value;
}

bool http_message::has_header(const std::string& name) {
  return headers_.count(name) > 0;
}

const std::string& http_message::get_header(const std::string& name) {
  headers_type::iterator it = headers_.find(name);
  if (it == headers_.end()) {
    // TODO: throw proper error
//    throw Exception("header not found");
  }
  return it->second;
}

void http_message::remove_header(const std::string& name) {
  headers_.erase(name);
}

const headers_type& http_message::trailers() const {
  return trailers_;
}

void http_message::set_trailer(const std::string& name,
    const std::string& value) {
  trailers_[name] = value;
}

void http_message::add_trailers(const headers_type& value, bool append) {
  for (headers_type::const_iterator it = value.begin(); it != value.end();
      ++it) {
    add_trailer(it->first, it->second, append);
  }
}
void http_message::add_trailer(const std::string& name,
    const std::string& value, bool append) {
  trailers_[name] =
      (has_header(name) && append) ? trailers_[name] + "," + value : value;
}

bool http_message::has_trailer(const std::string& name) {
  return trailers_.count(name) > 0;
}

const std::string& http_message::get_trailer(const std::string& name) {
  headers_type::iterator it = trailers_.find(name);
  if (it == trailers_.end()) {
    // TODO: throw proper error
//    throw Exception("header not found");
  }
  return it->second;
}

void http_message::remove_trailer(const std::string& name) {
  trailers_.erase(name);
}

const http_version& http_message::version() const {
  return version_;
}
void http_message::version(const http_version& v) {
  version_ = v;
}

bool http_message::version(unsigned short major, unsigned short minor) {
  if (major == 1 && minor == 0) {
    version_ = HTTP_1_0;
  } else if (major == 1 && minor == 1) {
    version_ = HTTP_1_1;
  } else {
    version_ = HTTP_UNKNOWN_VERSION;
    return false;
  }
  return true; // Valid version major/minor
}

int http_message::version_major() {
  switch (version_) {
  case HTTP_1_0:
  case HTTP_1_1:
    return 1;
  default:
    return -1;
  }
}

int http_message::version_minor() {
  switch (version_) {
  case HTTP_1_0:
    return 0;
  case HTTP_1_1:
    return 1;
  default:
    return -1;
  }
}

std::string http_message::version_string() {
  switch (version_) {
  case HTTP_1_0:
    return "HTTP/1.0";
  case HTTP_1_1:
    return "HTTP/1.1";
  default:
    return "";
  }
}

const url_obj& http_message::url() const {
  return url_;
}
bool http_message::url(const std::string& u, bool is_connect) {
  return url_.parse(u.c_str(), u.size(), is_connect);
}
bool http_message::url(const char* at, std::size_t len, bool is_connect) {
  return url_.parse(at, len, is_connect);
}
void http_message::url(const url_obj& u) {
  url_ = u;
}

const http_method& http_message::method() const {
  return method_;
}
void http_message::method(const http_method& m) {
  method_ = m;
}

unsigned short http_message::status() const {
  return status_;
}
void http_message::status(const unsigned short& s) {
  status_ = s;
}

bool http_message::upgrade() const {
  return upgrade_;
}
void http_message::upgrade(bool b) {
  upgrade_ = b;
}

bool http_message::should_keep_alive() const {
  return should_keep_alive_;
}
void http_message::should_keep_alive(bool b) {
  should_keep_alive_ = b;
}

size_t http_message::length() const {
  return length_;
}
void http_message::length(size_t l) {
  length_ = l;
}

Buffer http_message::renderHeaders() {
  // TODO: return headers buffer
  return Buffer();
}

Buffer http_message::renderTrailers() {
  // TODO: return trailers buffer
  return Buffer();
}

http_parser_context::http_parser_context(http_parser_type parser_type,
    http_message* message) :
    parser_(), settings_(), message_(message), was_header_value_(true),
    last_header_field_(), last_header_value_(), error_(),
    parse_completed_(false), have_flushed_(false), on_headers_complete_(),
    on_body_(), on_message_complete_(), on_error_() {
  http_parser_init(&parser_, parser_type);
  parser_.data = this;
  settings_.on_message_begin = on_message_begin;
  settings_.on_url = on_url;
  settings_.on_header_field = on_header_field;
  settings_.on_header_value = on_header_value;
  settings_.on_headers_complete = on_headers_complete;
  settings_.on_body = on_body;
  settings_.on_message_complete = on_message_complete;
}

http_parser_context::~http_parser_context() {
}

int http_parser_context::on_message_begin(http_parser* parser) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
  assert(self);

  // Reset parser state
  self->message_->url(url_obj()); // TODO: add reset function to url_obj
  return 0;
}

int http_parser_context::on_url(http_parser* parser, const char *at,
    size_t len) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
  assert(self);

  assert(at && len);

  if (!self->message_->url(at, len)) {
    self->error_ = resval(error::http_parser_url_fail);
    return 1;
  }

  return 0;
}

int http_parser_context::on_header_field(http_parser* parser, const char* at,
    size_t len) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
  assert(self);

  if (self->was_header_value_) {
    // new field started
    if (!self->last_header_field_.empty()) {
      // add new entry
      self->message_->add_header(self->last_header_field_,
          self->last_header_value_);
      self->last_header_value_.clear();

      // TODO: call Flush if ran out of space for headers
    }

    self->last_header_field_ = std::string(at, len);
    self->was_header_value_ = false;
  } else {
    // appending
    self->last_header_field_ += std::string(at, len);
  }

  return 0;
}

int http_parser_context::on_header_value(http_parser* parser, const char* at,
    size_t len) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
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
 * Fill in http_message using current parser state
 */
void http_parser_context::prepare_message() {
  CRUMB();

  /*
   * Common
   */

  // HTTP version
  message_->version(parser_.http_major, parser_.http_minor);

  /*
   * Response
   */

  std::string host("");
  int port = 80; // default port
  if (port) {
  } // XXX: avoid unused warnings
  // HTTP 1.1 requires "Host" header
  if (message_->version() == HTTP_1_1) {
    // get host and port info from header entry "Host"
    auto x = message_->headers().find("host");
    if (x != message_->headers().end()) {
      auto s = x->second;
      auto colon = s.find_last_of(':');
      if (colon == s.npos) {
        host = s;
      } else {
        host = s.substr(0, colon);
        port = std::stoi(s.substr(colon + 1));
      }
    }
  }

  // HTTP status
  message_->status(parser_.status_code);

  /*
   * Request
   */

  // HTTP method
  message_->method(static_cast<http_method>(parser_.method));

  /*
   * Headers
   */

  // HTTP keep-alive
  message_->should_keep_alive(http_should_keep_alive(&parser_));

  // HTTP upgrade
  message_->upgrade(parser_.upgrade > 0);

}

int http_parser_context::on_headers_complete(http_parser* parser) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
  assert(self);
  assert(&self->parser_ == parser);

  // add last entry if any
  if (!self->last_header_field_.empty()) {
    // add new entry
    self->message_->add_header(self->last_header_field_,
        self->last_header_value_);
  }

  self->prepare_message();

  // Call handle_headers_complete
  if (self->on_headers_complete_) {
    self->on_headers_complete_();
  }
  return 0;
}

int http_parser_context::on_body(http_parser* parser, const char* at,
    size_t len) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
  assert(self);

  // TODO: append body chunks to message

  // Call handle_body
  if (self->on_body_) {
    self->on_body_(at, 0, len);
  }
  return 0;
}

int http_parser_context::on_message_complete(http_parser* parser) {
  CRUMB();
  auto self = reinterpret_cast<http_parser_context*>(parser->data);
  assert(self);

  self->parse_completed_ = true;

  // Call handle_message_complete
  if (self->on_message_complete_) {
    self->on_message_complete_();
  }
  return 0;
}

void http_parser_context::on_headers_complete(
    on_headers_complete_type callback) {
  on_headers_complete_ = callback;
}

void http_parser_context::on_body(on_body_type callback) {
  on_body_ = callback;
}

void http_parser_context::on_message_complete(
    on_message_complete_type callback) {
  on_message_complete_ = callback;
}

void http_parser_context::on_error(on_error_type callback) {
  on_error_ = callback;
}

bool http_parser_context::is_finished() const {
  // there was an error or parse completed.
  return !error_ || parse_completed_;
}

bool http_parser_context::feed_data(const char* data, std::size_t offset,
    std::size_t length) {
  CRUMB();
  ::http_parser_execute(&parser_, &settings_, &data[offset], length);
  if (parser_.http_errno != HPE_OK) {
    // there was an error
    if (on_error_)
      on_error_(resval(parser_.http_errno));
    return true;
  } else if (parse_completed_) {
    // finished, should have invoked on_message_complete already
    return true;
  } else {
    // need more data
    return false;
  }
}

std::string http_status_text(int status_code) {
  switch (status_code) {
  case 100:
    return "Continue";
  case 101:
    return "Switching Protocols";
  case 102:
    return "Processing";                 // RFC 2518; obsoleted by RFC 4918
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 202:
    return "Accepted";
  case 203:
    return "Non-Authoritative Information";
  case 204:
    return "No Content";
  case 205:
    return "Reset Content";
  case 206:
    return "Partial Content";
  case 207:
    return "Multi-Status";               // RFC 4918
  case 300:
    return "Multiple Choices";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Moved Temporarily";
  case 303:
    return "See Other";
  case 304:
    return "Not Modified";
  case 305:
    return "Use Proxy";
  case 307:
    return "Temporary Redirect";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 402:
    return "Payment Required";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 406:
    return "Not Acceptable";
  case 407:
    return "Proxy Authentication Required";
  case 408:
    return "Request Time-out";
  case 409:
    return "Conflict";
  case 410:
    return "Gone";
  case 411:
    return "Length Required";
  case 412:
    return "Precondition Failed";
  case 413:
    return "Request Entity Too Large";
  case 414:
    return "Request-URI Too Large";
  case 415:
    return "Unsupported Media Type";
  case 416:
    return "Requested Range Not Satisfiable";
  case 417:
    return "Expectation Failed";
  case 418:
    return "I\"m a teapot";              // RFC 2324
  case 422:
    return "Unprocessable Entity";       // RFC 4918
  case 423:
    return "Locked";                     // RFC 4918
  case 424:
    return "Failed Dependency";          // RFC 4918
  case 425:
    return "Unordered Collection";       // RFC 4918
  case 426:
    return "Upgrade Required";           // RFC 2817
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 503:
    return "Service Unavailable";
  case 504:
    return "Gateway Time-out";
  case 505:
    return "HTTP Version not supported";
  case 506:
    return "Variant Also Negotiates";    // RFC 2295
  case 507:
    return "Insufficient Storage";       // RFC 4918
  case 509:
    return "Bandwidth Limit Exceeded";
  case 510:
    return "Not Extended";                // RFC 2774
  default:
    assert(false);
    return std::string();
  }
}

} // namespace detail
} // namespace native

