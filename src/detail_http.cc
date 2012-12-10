
#include "native/base.h"
#include "native/detail/http.h"

#undef DEBUG_ENABLED
#define DEBUG_ENABLED 1

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

void url_obj::reset() {
  handle_ = http_parser_url();
  buf_ = std::string();
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

/* http_start_line ************************************************************/

http_start_line::http_start_line()
: version_()
, method_()
, url_()
, status_(0)
{}

http_start_line::http_start_line(const http_start_line& c)
: version_(c.version_)
, method_(c.method_)
, url_(c.url_)
, status_(c.status_)
{}

http_start_line::http_start_line(http_start_line&& c)
: version_(std::move(c.version_))
, method_(std::move(c.method_))
, url_(std::move(c.url_))
, status_(std::move(c.status_))
{}

http_start_line::~http_start_line() {
}

const http_version& http_start_line::version() const {
  return version_;
}
void http_start_line::version(const http_version& v) {
  version_ = v;
}

bool http_start_line::version(unsigned short major, unsigned short minor) {
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

int http_start_line::version_major() const {
  switch (version_) {
  case HTTP_1_0:
  case HTTP_1_1:
    return 1;
  default:
    return -1;
  }
}

int http_start_line::version_minor() const {
  switch (version_) {
  case HTTP_1_0:
    return 0;
  case HTTP_1_1:
    return 1;
  default:
    return -1;
  }
}

std::string http_start_line::version_string() const {
  switch (version_) {
  case HTTP_1_0:
    return "HTTP/1.0";
  case HTTP_1_1:
    return "HTTP/1.1";
  default:
    return "";
  }
}

const url_obj& http_start_line::url() const {
  return url_;
}
bool http_start_line::url(const std::string& u, bool is_connect) {
  return url_.parse(u.c_str(), u.size(), is_connect);
}
bool http_start_line::url(const char* at, std::size_t len, bool is_connect) {
  return url_.parse(at, len, is_connect);
}
void http_start_line::url(const url_obj& u) {
  url_ = u;
}

const http_method& http_start_line::method() const {
  return method_;
}
void http_start_line::method(const http_method& m) {
  method_ = m;
}

unsigned short http_start_line::status() const {
  return status_;
}
void http_start_line::status(const unsigned short& s) {
  status_ = s;
}

void http_start_line::reset() {
  url_.reset();
  version_ = http_version();
  method_ = http_method();
  status_ = 0;
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

