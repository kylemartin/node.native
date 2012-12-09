#ifndef __HTTP_H__
#define __HTTP_H__

#include <sstream>
#include "base.h"
#include "detail.h"
#include "events.h"
#include "net.h"
#include "process.h"

#define CRLF "\r\n"

namespace native
{
  namespace http
  {
    // forward declarations
    class IncomingMessage;
    class OutgoingMessage;
    class ServerRequest;
    class ServerResponse;
    class ClientRequest;
    class ClientResponse;
  }

  namespace event { namespace http {
    namespace client {
      struct connect
          : public util::callback_def<native::http::ClientResponse*,
            net::Socket*, const Buffer&> {};
      struct upgrade
          : public util::callback_def<native::http::ClientResponse*,
            net::Socket*, const Buffer&> {};
      struct response
          : public util::callback_def<native::http::ClientResponse*> {};
    }

    namespace server {
      struct connect
          : public util::callback_def<native::http::ServerRequest*,
            net::Socket*, const Buffer&> {};
      struct upgrade
          : public util::callback_def<native::http::ServerRequest*,
            net::Socket*, const Buffer&> {};
      struct request
          : public util::callback_def<native::http::ServerRequest*,
            native::http::ServerResponse*> {};
      struct checkContinue
          : public util::callback_def<native::http::ServerRequest*,
            native::http::ServerResponse*> {};
    }
    struct socket : public util::callback_def<net::Socket*> {};
    struct Continue : public util::callback_def<>{};
    struct finish : public util::callback_def<> {};
    struct headers : public util::callback_def<const detail::headers_type&>{};
    struct trailers : public util::callback_def<const detail::headers_type&>{};
  }}

  namespace http {
    using detail::headers_type;
    using detail::http_start_line;

    class Parser {
    public:
      /**
       * Callback that must be registered to allow construction of classes
       * derived from IncomingMessage. This is necessary because we want to
       * emit events on the derived type.
       */
      typedef std::function<IncomingMessage*(net::Socket*,
          Parser*)> on_incoming_type;

      typedef std::function<void(const Exception&)> on_error_type;
      typedef std::function<void()> on_close_type;
      typedef std::function<void()> on_end_type;

      static Parser* create(
          http_parser_type type,
          net::Socket* socket,
          on_incoming_type incoming_cb = nullptr);

      void register_on_incoming(on_incoming_type callback);
      void register_on_error(on_error_type callback);
      void register_on_close(on_close_type callback);
      void register_on_end(on_end_type callback);

      // Accessors
      bool parsing() const;
      bool upgrade() const;
      bool should_keep_alive() const;
      const http_start_line& start_line() const;

      ~Parser();

    private:

      net::Socket* socket_; // Passed to constructor
      on_incoming_type on_incoming_;
      on_error_type on_error_;
      on_close_type on_close_;
      on_end_type on_end_;

      IncomingMessage* incoming_; // Allocated via on_incoming_ callback

      http_parser parser_;
      http_parser_settings settings_;
      http_start_line start_line_;

      bool was_header_value_;
      std::string last_header_field_;
      std::string last_header_value_;

      detail::resval error_;
      bool parsing_;

      bool upgrade_;
      bool should_keep_alive_;

      Parser(http_parser_type type, net::Socket* socket);

      void registerSocketEvents();

      int on_headers_complete();
      int on_body(const char* buf, size_t off, size_t len);
      int on_message_complete();
      int on_error(const native::Exception& e);

      bool feed_data(const char* data, std::size_t offset, std::size_t length);

      void add_header(const std::string& name, const std::string& value);

      void reset();

    public:
      // http_parser_settings callbacks
      static int on_message_begin_(http_parser* parser);
      static int on_url_(http_parser* parser, const char *at, size_t len);
      static int on_header_field_(http_parser* parser, const char* at, size_t len);
      static int on_header_value_(http_parser* parser, const char* at, size_t len);
      static int on_headers_complete_(http_parser* parser);
      static int on_body_(http_parser* parser, const char* at, size_t len);
      static int on_message_complete_(http_parser* parser);

    };

    /**
     * This is the base class for incoming http messages (requests or responses 
     * for servers or clients respectively). These will be created by a Parser 
     * after parsing headers and will be passed to a request/response handler 
     * for further processing and registering event listeners. It implements 
     * the ReadableStream interface and read-only access to encapsulated
     * instance of detail::http_message.
     *
     * Emits the following events:
     *   data  - when a chunk of the body is received
     *   end   - the body is finished and no more data events to come
     *   close - the connection was terminated before finishing
     *   error - there was an error with the connection or HTTP parsing
     */
    class IncomingMessage : public EventEmitter
    {
    protected:
      net::Socket* socket_;
      Parser* parser_;
      bool complete_;
      bool readable_;
      bool paused_;
      std::vector<Buffer> pendings_;
      bool endEmitted_;
      http_start_line message_;

      headers_type headers_;
      headers_type trailers_;

      bool body_;

    public:
      IncomingMessage(net::Socket* socket, Parser* parser);

      virtual ~IncomingMessage() {}

      // TODO: add move constructor

      bool readable();
      void pause();
      void resume();

      void destroy(const Exception& e);

      /*
       * Accessors
       */

      Parser* parser();

      net::Socket* socket();

      // Read-only HTTP message
      int statusCode();
      const http_method& method();
      const detail::http_version& httpVersion();
      std::string httpVersionString();
      int httpVersionMajor();
      int httpVersionMinor();
      const detail::url_obj& url();

      bool shouldKeepAlive();
      bool upgrade();

      /**
       * Called from parser to signal end of message
       */
      void end();

      // TODO: handle encoding
      //void setEncoding();

      const headers_type& headers() const;
      bool has_header(const std::string& name);
      const std::string& get_header(const std::string& name);

      const headers_type& trailers() const;
      bool has_trailer(const std::string& name);
      const std::string& get_trailer(const std::string& name);

      void parser_add_header(const std::string& name, const std::string& value);
      void parser_on_error(const Exception& e);
      void parser_on_body(const Buffer& body);
      virtual void parser_on_headers_complete();

    protected:
      void _emitPending(std::function<void()> callback);
      void _emitData(const Buffer& buf);
      void _emitEnd();
      void _addHeaderLine(const std::string& field, const std::string& value);

    };

    /**
     * This is the base class for outgoing messages (responses or requests
     * for servers or clients respectively).
     * It implements the WritableStream interface
     *
     * Events emitted:
     *   drain
     *   error
     *   close - indicates connection was terminated before calling end()
     *   pipe
     */
    class OutgoingMessage : public EventEmitter
    {
    protected:
      net::Socket* socket_;
      // TODO: make Buffer vector-like for easier output buffering
      std::vector<Buffer> output_;
      // TODO: handle output encodings
      bool writable_;
      bool last_;
      bool chunkedEncoding_;
      bool useChunkedEncodingByDefault_;
      bool sendDate_;
      bool hasBody_;
      bool expectContinue_;
      bool sent100_;
      bool shouldKeepAlive_;
      Buffer trailer_;
      bool finished_;
      http_start_line start_line_;
      headers_type headers_;
      headers_type trailers_;
      Buffer header_;
      bool headerSent_;

    public:
      OutgoingMessage(net::Socket* socket_);

      /*
       * WritableStream Interface
       */
      // TODO: handle encoding
      void write(const Buffer& buf);
      void write(const std::string& str);

      /**
       * Signal the end of the outgoing message
       * @param buf
       * @return
       */
      void end(const Buffer& buf);
      void end();
      void destroy(const Exception& e);

      /*
       * Write-only HTTP message interface
       */

      void method(const http_method& method);
      void status(int code);
      void version(const detail::http_version& version);
      void url(const detail::url_obj& url);

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


    protected:
      virtual void _implicitHeader() {}
      /**
       *
       * @param firstLine
       * @param headers
       */
      void _storeHeader(const std::string& firstLine,
          const headers_type& headers);

      headers_type _renderHeaders();

      void _flush();

      // TODO: handle encoding
      void _writeRaw(const Buffer& buf);

    private:
      /*
       * Internal
       */
      // TODO: handle encoding
      void _send(const Buffer& buf);


      // TODO: handle encoding
      void _buffer(const Buffer& buf);

      void _finish();
    };

    class Server;

    /**
     * ServerRequest event interface
     * 'data': function (chunk) { }
     * 'end': function () { }
     * 'close': function () { }
     */
    class ServerRequest : public IncomingMessage
    {
    private:
        Server* server_;
    public:
        ServerRequest(net::Socket* socket, Parser* parser);

        ~ServerRequest() {}

        void parser_on_headers_complete();
        void server(Server* server);

    };

    /**
     * ServerResponse event interface
     * 'close': function () { }
     */
    class ServerResponse : public OutgoingMessage
    {
    public:
        ServerResponse(net::Socket* socket);

        virtual ~ServerResponse() {}

        void _implicitHeader();
        void writeContinue();
        void writeHead(int statusCode, const std::string& reasonPhrase,
            const headers_type& headers);
        void writeHead(int statusCode,
            const headers_type& headers = headers_type());
    };

    /**
     * Server event interface:
     * 'request': function (request, response) { }
     * 'connection': function (socket) { }
     * 'close': function () { }
     * 'checkContinue': function (request, response) { }
     * 'connect': function (request, socket, head) { }
     * 'upgrade': function (request, socket, head) { }
     * 'clientError': function (exception) { }
     */
    class Server : public net::Server
    {
    public:
        Server();

        virtual ~Server() {}

        void on_connection(net::Socket* socket);

        IncomingMessage* on_incoming(net::Socket* socket, Parser* parser);
    };

    /**
     * ClientResponse event interface
     * 'data': function (chunk) { }
     * 'end': function () { }
     * 'close': function () { }
     */
    class ClientResponse : public IncomingMessage
    {
    public:
      ClientResponse(net::Socket* socket, Parser* message);
    };

    /**
     * ClientRequest event interface:
     * 'response': function (response) { }
     * 'socket': function (socket) { }
     * 'connect': function (response, socket, head) { }
     * 'upgrade': function (response, socket, head) { }
     * 'continue': function () { }
     */
    class ClientRequest : public OutgoingMessage
    {
    private:
      http_method method_;
      headers_type headers_;
      std::string path_;

      listener_t socket_error_listener_;
      listener_t socket_close_listener_;

      bool received_response_ = false;
    public:
      /**
       * Construct a ClientRequest for the given url
       *
       * Unlike node.js, we do not accept an options map. Instead users socket
       * must set parameters like method, headers, etc. on the request socket
       * object itself.
       *
       * @param url
       * @param callback
       */
      ClientRequest(detail::url_obj url,
          std::function<void(ClientResponse*)> callback = nullptr);

      virtual ~ClientRequest() {}

      void abort();
      void setTimeout(int64_t timeout, std::function<void()> callback=nullptr);
      void setNoDelay(bool noDelay);
      void setSocketKeepAlive(bool enable, int64_t initialDelay);

    private:
      void _implicitHeader();

      void _deferToConnect(std::function<void()> callback);

      void init_socket(net::Socket* socket);

      void on_incoming_message(IncomingMessage* msg);

      void on_response_end();

      void on_socket_connect();

      void on_socket_drain();

      void on_socket_error(const Exception& e);

      void on_socket_data(const Buffer& buf);

      void on_socket_end();

      void on_socket_close();
    };

    Server* createServer();

    ClientRequest* request(const std::string& url_string,
        std::function<void(ClientResponse*)> callback = nullptr);

    ClientRequest* get(const std::string& url_string,
        std::function<void(ClientResponse*)> callback = nullptr);
  }
}

#endif
