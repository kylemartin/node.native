#ifndef __DETAIL_HTTP_H__
#define __DETAIL_HTTP_H__

#include <sstream>
#include "base.h"
#include "stream.h"
#include "utility.h"
#include "buffers.h"

namespace native {
namespace detail {

class url_obj {
public:
  url_obj();
  ~url_obj();

  bool parse(const char* buffer, std::size_t length, bool is_connect = false);
  void reset();

public:
  std::string schema() const;
  std::string host() const;
  int port() const;
  std::string path_query_fragment() const;
  std::string path() const;
  std::string query() const;
  std::string fragment() const;

  bool has_schema() const;
  bool has_host() const;
  bool has_port() const;
  bool has_path() const;
  bool has_query() const;
  bool has_fragment() const;

private:
  http_parser_url handle_;
  std::string buf_;
};

enum http_version {
  HTTP_1_0, HTTP_1_1, HTTP_UNKNOWN_VERSION
};

typedef std::map<std::string, std::string, util::text::ci_less> headers_type;

std::string http_status_text(int status_code);

/**
 * The http_start_line encapsulates the fields in an HTTP request or status line.
 */
class http_start_line {
private:
  // Common parameters
  http_version version_;

  // Request Line
  http_method method_;
  url_obj url_;

  // Response Line
  unsigned short status_;

public:
  http_start_line();
  http_start_line(const http_start_line& c);
  http_start_line(http_start_line&& c);
  ~http_start_line();

  const http_version& version() const;
  int version_major() const;
  int version_minor() const;
  std::string version_string() const;
  void version(const http_version& v);
  bool version(unsigned short major, unsigned short minor);

  const url_obj& url() const;
  bool url(const std::string& u, bool is_connect=false);
  bool url(const char* at, std::size_t len, bool is_connect=false);
  void url(const url_obj& u);

  const http_method& method() const;
  void method(const http_method& m);

  unsigned short status() const;
  void status(const unsigned short& s);

  void reset();
};

} // namespace detail
} // namespace native

#endif

