#ifndef __DETAIL_HTTP_H__
#define __DETAIL_HTTP_H__

#include <sstream>
#include "base.h"
#include "stream.h"
#include "utility.h"
#include "buffers.h"

namespace native {
namespace detail {

class http_message;
typedef std::function<void(const http_message*, resval)>
    http_parse_callback_type;

class http_parser_context;

class url_obj {
public:
  url_obj();
  ~url_obj();

  bool parse(const char* buffer, std::size_t length, bool is_connect = false);

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
/**
 * An http_message encapsulates headers, body, etc. shared by
 * request and response messages. It is used during parsing to hold
 * message details for an incoming message. It is also used to prepare
 * an outgoing message for transmission.
 *
 * Consists of:
 * - the first line (version and method for request, status for response)
 * - leading headers
 * - an optional body
 * - optional trailing headers (for chunked encoding)
 */
class http_message {
public:
private:

  // Common parameters
  http_version version_;
  headers_type headers_;
  headers_type trailers_;
    // Headers received after the body using chunked transfer

  // Request Line
  http_method method_;
  url_obj url_;

  // Response Line
  unsigned short status_;

  // Headers
  bool upgrade_;
  bool should_keep_alive_;

  Buffer body_;

public:
  http_message();
  http_message(const http_message& c);
  http_message(http_message&& c);
  ~http_message();

public:
  const headers_type& headers() const;
  void set_header(const std::string& name, const std::string& value);
  void add_headers(const headers_type& value, bool append = false);
  void add_header(const std::string& name, const std::string& value,
      bool append= false);
  bool has_header(const std::string& name);
  const std::string& get_header(const std::string& name);
  void remove_header(const std::string& name);

  const headers_type& trailers() const;
  void set_trailer(const std::string& name, const std::string& value);
  void add_trailers(const headers_type& value, bool append = false);
  void add_trailer(const std::string& name, const std::string& value,
      bool append = false);
  bool has_trailer(const std::string& name);
  const std::string& get_trailer(const std::string& name);
  void remove_trailer(const std::string& name);

  const http_version& version() const;
  void version(const http_version& v);

  bool version(unsigned short major, unsigned short minor);

  int version_major();
  int version_minor();

  std::string version_string();

  const url_obj& url() const;
  bool url(const std::string& u, bool is_connect=false);
  bool url(const char* at, std::size_t len, bool is_connect=false);
  void url(const url_obj& u);

  const http_method& method() const;
  void method(const http_method& m);

  unsigned short status() const;
  void status(const unsigned short& s);

  bool upgrade() const;
  void upgrade(bool b);

  bool should_keep_alive() const;
  void should_keep_alive(bool b);

  const Buffer& body();
  void body(const Buffer& b);

  /**
   * Prepare headers to be sent over the wire.
   * @return
   */
   Buffer renderHeaders();

  /**
   * Prepare trailers to be sent over the wire.
   * @return
   */
   Buffer renderTrailers();

  /**
   * Append a chunk of data to the body.
   *
   * IncomingMessage and OutgoingMessage override this method to
   * handle chunked transfers properly. When receiving a body this
   * is called once for normal transfers and multiple times for
   * chunked transfers. When sending a body this may be called
   * multiple times until complete, with normal transfer the entire
   * body is sent at once, with chunked transfers a chunk may be sent
   * after each append.
   *
   * @param buf
   */
   void appendBody(const Buffer& buf);
};

  /**
   * Encapsulates an http_parser storing results in an http_message.
   * This also provides callbacks for inspecting the http_message at
   * different stages of parsing.
   */
class http_parser_context {
private:
  http_parser parser_;
  http_parser_settings settings_;
  http_message* message_;

  bool was_header_value_;
  std::string last_header_field_;
  std::string last_header_value_;

  resval error_;
  bool parse_completed_;
  bool have_flushed_;

  // Callbacks for inspecting message as it is parsed
  typedef std::function<void()> on_headers_complete_type;
  typedef std::function<void(const char* buf, size_t off, size_t len)>
      on_body_type;
  typedef std::function<void()> on_message_complete_type;
  typedef std::function<void(const resval&)> on_error_type;

  on_headers_complete_type on_headers_complete_;
  on_body_type on_body_;
  on_message_complete_type on_message_complete_;
  on_error_type on_error_;

public:
  http_parser_context(http_parser_type parser_type, http_message* message);
  ~http_parser_context();
  static int on_message_begin(http_parser* parser);
  static int on_url(http_parser* parser, const char *at, size_t len);
  static int on_header_field(http_parser* parser, const char* at, size_t len);
  static int on_header_value(http_parser* parser, const char* at, size_t len);

  /**
   * Fill in http_message using current parser state
   */
  void prepare_message();
  static int on_headers_complete(http_parser* parser);
  static int on_body(http_parser* parser, const char* at, size_t len);
  static int on_message_complete(http_parser* parser);

public:
  void on_headers_complete(on_headers_complete_type callback);
  void on_body(on_body_type callback);
  void on_message_complete(on_message_complete_type callback);
  void on_error(on_error_type callback);
  bool is_finished() const;
  bool feed_data(const char* data, std::size_t offset, std::size_t length);
};

std::string http_status_text(int status_code);

} // namespace detail
} // namespace native

#endif

