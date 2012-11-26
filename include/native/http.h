#ifndef __HTTP_H__
#define __HTTP_H__

#include <sstream>
#include "base.h"
#include "detail.h"
#include "events.h"
#include "net.h"
#include "process.h"

#define CRLF "\r\n"

/*

# HTTP #

Sockets are created by a Server for incoming connections or by a ClientRequest
for a new connection. Parser factory manages pool of http_parser_context. Each
Parser takes ownership of a Socket and begins to read and feed data to its
http_parser_context. Users of a Parser register callbacks for allocating and
handling an IncomingMessage once headers have been received. For servers, a
ServerRequest is allocated, for clients, a ClientResponse. Users can attach data
event handlers to the ServerRequest or ClientResponse to receive post or body
data respectively.

Since HTTP request/response messages are structurally similar, the
detail::http_message class represents common features of both. It holds a status
line, sets of leading and trailing headers, and an optional body.
IncomingMessage and OutgoingMessage own an http_message and provide interfaces
for read-only incoming messages and write-only outgoing messages, independent of
the type of message (request/response).

IncomingMessage implements the ReadableStream event interface and encapsulates
the detail::http_message and logic common to the ServerRequest and
ClientResponse sub-classes. ServerRequest and ClientResponse are further
specialized for use in server and client contexts.

OutgoingMessage implements the WritableStream event interface and encapsulates
the detail::http_message and logic common to ServerResponse and ClientRequest
sub-classes. ServerResponse and ClientResponse are further specialized for use
in server and client contexts.

For incoming messages, instances of detail::http_message are passed between
the Parser, detail::http_parser_context and allocated IncomingMessage. They
are created by the Parser and passed to its detail::http_parser_context to
receive first line and initial headers. After headers are parsed by the
http_parser_context the Parser's registered on_headers_complete callback is
invoked to pass ownership of the http_message (though it will continue to
update it as parsing continues). The Parser allocates an IncomingMessage at
that time and passes ownership of the message to it. Since outgoing messages
do not involve parsing a detail::http_message instance is owned by its
associated OutgoingMessage.

 */
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
  }}

  namespace http {
    using detail::headers_type;

    /**
     * Factory managing pool of Parser instances composed of a net::Socket and a
     * detail::http_parser_context constructing a detail::http_message.
     *
     * Parser instances register event handlers on the net::Socket to receive
     * and feed it to the http_parser_context. They are also responsible for
     * managing the socket state in relation to the parser state (errors, etc.).
     *
     * Parser instances expect to process incoming messages from clients or 
     * servers. Status line and headers are stored in an http_message
     * and passed on to the on_incoming callback to construct an IncomingMessage
     * and determine if the body needs to be parsed. The on_incoming callback
     * MUST be registered before starting to receive a message and is expected
     * to implement the the logic for handling an incoming request/response and
     * processing the body as it is received.
     */
class Parser : public EventEmitter {
    public:
      /**
       * Callback that must be registered to allow construction of classes
       * derived from IncomingMessage. This is necessary because we want to
       * emit events on the derived type.
       * Callback for handling request and response IncomingMessages. This gets
       * called from on_handle_headers_complete to determine if we should skip 
       * the body and to pass the IncomingMessage constructed from the headers, 
       * which can in turn be passed in a request/response event so the user can
       * register event handlers on the IncomingMessage.
       */
      // TODO: Is there a better way to emit on IncomingMessage subtypes?
      typedef std::function<IncomingMessage*(net::Socket*,
          detail::http_message*)> on_incoming_type;

      typedef std::shared_ptr<Parser> ptr;

      /**
       * Create a parser of the specified type attached to the given socket
       * which invokes the given callback to create an IncomingMessage instance.
       *
       * @param socket
       * @param type
       * @param callback
       * @return
       */
      static Parser* create(
          http_parser_type type,
          net::Socket* socket,
          on_incoming_type incoming_cb = nullptr);

      /**
       * Set the callback for allocating an incoming message
       * @param callback [description]
       */
      void on_incoming(on_incoming_type callback);

    private:

      detail::http_message* message_; // Constructed by Parser
      detail::http_parser_context context_;
      net::Socket* socket_; // Passed to constructor
      IncomingMessage* incoming_; // Allocated via registered callback
      on_incoming_type on_incoming_;

      /**
       * Private constructor
       * @param type   http_parser_type value (HTTP_{REQUEST,RESPONSE,BOTH})
       * @param socket net::Socket to be read by parser
       */
      Parser(http_parser_type type, net::Socket* socket);

      /**
       * Registered with detail::http_parser_context to process headers and 
       * setup an IncomingMessage to pass to the on_incoming callback registered
       * with this Parser
       * @param  result [description]
       * @return        [description]
       */
      int on_headers_complete();

      /**
       * Registered with detail::http_parser_context to handle a chunk or 
       * complete body of a message
       * @param  buf Data buffer
       * @param  off Offset into buffer
       * @param  len Length of buffer
       * @return     [description]
       */
      int on_body(const char* buf, size_t off, size_t len);

      /**
       * Registered with detail::http_parser_context to handle a finished 
       * message (received body and trailers)
       * @param  result [description]
       * @return        [description]
       */
      int on_message_complete();

      /**
       * Registered with detail::http_parser_context to handle an error in 
       * parsing data received on socket
       * @param  e [description]
       * @return   [description]
       */
      int on_error(const native::Exception& e);

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
      headers_type pendings_;
      bool endEmitted_;
      detail::http_message* message_;

    public:
      IncomingMessage(net::Socket* socket, detail::http_message* message);

      // TODO: add move constructor


      void pause();
      void resume();

      void destroy(const Exception& e);

      /*
       * Accessors
       */

      void parser(Parser* parser);
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

      const headers_type& headers();
      const headers_type& trailers();

      bool shouldKeepAlive();
      bool upgrade();

      const std::string& getHeader(const std::string& name);
      const std::string& getTrailer(const std::string& name);

      /**
       * Called from parser to signal end of message
       */
      void end();

      // TODO: handle encoding
      //void setEncoding();
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
      detail::http_message message_;
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

      void addHeaders(const headers_type& value);
      void setHeader(const std::string& name, const std::string& value);
      void hasHeader(const std::string& name);
      const std::string& getHeader(const std::string& name);
      void removeHeader(const std::string& name);

      void addTrailers(const headers_type& value);
      void setTrailer(const std::string& name, const std::string& value);
      void hasTrailer(const std::string& name);
      const std::string& getTrailer(const std::string& name);
      void removeTrailer(const std::string& name);

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

    class ServerRequest : public IncomingMessage
    {
        friend class Server;

    protected:
        ServerRequest(net::Socket* socket, detail::http_message* message);

        virtual ~ServerRequest() {}
    public:
    };

    class ServerResponse : public OutgoingMessage
    {
        friend class Server;

    protected:
        ServerResponse(net::Socket* socket);

        virtual ~ServerResponse() {}

    public:
        void _implicitHeader();
        void writeContinue();
        void writeHead(int statusCode, const std::string& reasonPhrase,
            const headers_type& headers);
        void writeHead(int statusCode,
            const headers_type& headers = headers_type());
    };

    class Server : public net::Server
    {
    public:
        Server();

        virtual ~Server() {}

    public:
        //void close();
        //void listen(port, hostname, callback);
        //void listen(path, callback);
    };

    class ClientResponse : public IncomingMessage
    {
    public:
      ClientResponse(net::Socket* socket, detail::http_message* message);
    };

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

      void init_socket();

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
