
#include "native/http.h"

namespace native { namespace http {

/* ServerRequest **************************************************************/
#undef DBG
#define DBG(msg) DEBUG_PRINT("[ServerRequest] " << msg)

ServerRequest::ServerRequest(net::Socket* socket, detail::http_message* message)
    : IncomingMessage(socket, message) // TODO: use move constructor
{
  DBG("constructing");

  socket_->on<native::event::data>([this](const Buffer& buffer){ emit<native::event::data>(buffer); });
  socket_->on<native::event::end>([this](){ emit<native::event::end>(); });
  socket_->on<native::event::close>([this](){ emit<native::event::close>(); });
}

/* ServerResponse *************************************************************/

#undef DBG
#define DBG(msg) DEBUG_PRINT("[ServerResponse] " << msg)
ServerResponse::ServerResponse(net::Socket* socket)
    : OutgoingMessage(socket)
{
  DBG("constructing");
    assert(socket_);

    socket_->on<native::event::close>([this](){ emit<native::event::close>(); });

    this->status(200);
}

void ServerResponse::_implicitHeader() {
  this->writeHead(this->message_.status());
};

void ServerResponse::writeContinue() {
  DBG("writing continue");

//  this._writeRaw('HTTP/1.1 100 Continue' + CRLF + CRLF, 'ascii');
//  this._sent100 = true;
  this->_writeRaw(Buffer("HTTP/1.1 100 Continue" CRLF CRLF));
}

void ServerResponse::writeHead(int statusCode, const headers_type& given_headers) {
  writeHead(statusCode, "", given_headers);
}

void ServerResponse::writeHead(int statusCode, const std::string& given_reasonPhrase
    , const headers_type& given_headers)
{
  DBG("writing headers");

  std::string reasonPhrase(!given_reasonPhrase.empty()
      ? given_reasonPhrase : detail::http_status_text(statusCode));

  // only progressive api is used
  headers_type headers = this->_renderHeaders();

  this->status(statusCode);
  if (!given_headers.empty() && !this->message_.headers().empty()) {
    // Slow-case: when progressive API and header fields are passed.
    headers.insert(given_headers.begin(), given_headers.end());
  } else if (this->message_.headers().empty()) {
    // only writeHead() called
    headers = given_headers;
  }

  std::string statusLine = "HTTP/1.1 " + std::to_string(statusCode) + " " + reasonPhrase + CRLF;

  if (statusCode == 204 || statusCode == 304 ||
      (100 <= statusCode && statusCode <= 199)) {
      // RFC 2616, 10.2.5:
      // The 204 response MUST NOT include a message-body, and thus is always
      // terminated by the first empty line after the header fields.
      // RFC 2616, 10.3.5:
      // The 304 response MUST NOT contain a message-body, and thus is always
      // terminated by the first empty line after the header fields.
      // RFC 2616, 10.1 Informational 1xx:
      // This class of status code indicates a provisional response,
      // consisting only of the Status-Line and optional headers, and is
      // terminated by an empty line.
      this->hasBody_ = false;
  }

  if (this->expectContinue_ && !this->sent100_) {
    this->shouldKeepAlive_ = false;
  }
  this->_storeHeader(statusLine, headers);
}

#undef DBG
#define DBG(msg) DEBUG_PRINT("[Server] " << msg)
Server::Server()
    : net::Server()
{
  DBG("constructing");
    registerEvent<native::event::http::server::request>();
    registerEvent<native::event::http::server::checkContinue>();
    registerEvent<native::event::http::server::upgrade>();
    registerEvent<native::event::connect>();
    registerEvent<native::event::clientError>();
    // Inherited from net::Server
    // registerEvent<native::event::connection>();
    // registerEvent<native::event::close>();
    // registerEvent<native::event::error>();

    // Register net::Server connection listener
    on<native::event::connection>([&](net::Socket* socket){
      return this->on_connection(socket);
    });
}

void Server::on_connection(net::Socket* socket) {
  DBG("on connection");
    assert(socket);

    // Create a Parser to handle incoming message
    // TODO: check if allocated Parser are leaking
    Parser* parser(Parser::create(HTTP_REQUEST, socket));

    // The parser will start reading from the socket and parsing data

    // After receiving headers it will call this to create an IncomingMessage
    parser->on_incoming([=](net::Socket* socket, detail::http_message* message) {
      return this->on_incoming(parser, socket, message);
    });

    parser->on<event::error>([this](const Exception& e) {
      emit<event::error>(e);
    });
}

//js: // The following callback is issued after the headers have been read on a
//js: // new message. In this callback we setup the response object and pass it
//js: // to the user.
//js: parser.onIncoming = function(req, shouldKeepAlive) {
//js:   incoming.push(req);
//js:
//js:   var res = new ServerResponse(req);
//js:   debug('server response shouldKeepAlive: ' + shouldKeepAlive);
//js:   res.shouldKeepAlive = shouldKeepAlive;
//js:   DTRACE_HTTP_SERVER_REQUEST(req, socket);
//js:
//js:   if (socket._httpMessage) {
//js:     // There are already pending outgoing res, append.
//js:     outgoing.push(res);
//js:   } else {
//js:     res.assignSocket(socket);
//js:   }
//js:
//js:   // When we're finished writing the response, check if this is the last
//js:   // respose, if so destroy the socket.
//js:   res.on('finish', function() {
//js:     // Usually the first incoming element should be our request.  it may
//js:     // be that in the case abortIncoming() was called that the incoming
//js:     // array will be empty.
//js:     assert(incoming.length == 0 || incoming[0] === req);
//js:
//js:     incoming.shift();
//js:
//js:     res.detachSocket(socket);
//js:
//js:     if (res._last) {
//js:       socket.destroySoon();
//js:     } else {
//js:       // start sending the next message
//js:       var m = outgoing.shift();
//js:       if (m) {
//js:         m.assignSocket(socket);
//js:       }
//js:     }
//js:   });
//js:
//js:   if ('expect' in req.headers &&
//js:       (req.httpVersionMajor == 1 && req.httpVersionMinor == 1) &&
//js:       continueExpression.test(req.headers['expect'])) {
//js:     res._expect_continue = true;
//js:     if (self.listeners('checkContinue').length) {
//js:       self.emit('checkContinue', req, res);
//js:     } else {
//js:       res.writeContinue();
//js:       self.emit('request', req, res);
//js:     }
//js:   } else {
//js:     self.emit('request', req, res);
//js:   }
//js:   return false; // Not a HEAD response. (Not even a response!)
//js: };

/*
 * Parser received headers, create ServerRequest for parser to emit body and
 * completion events on, and create ServerResponse and emit for users to write
 */
IncomingMessage* Server::on_incoming(Parser* parser, net::Socket* socket, detail::http_message* message) {
  DBG("creating ServerRequest for Parser");
  // Create ServerRequest from IncomingMessage
  ServerRequest* req = new ServerRequest(socket, message);

  assert(req);

  req->parser(parser);

  // TODO: Do early header processing and decide whether body should be received

  // Emit request event so user can prepare for receiving body

  // Create ServerResponse
  ServerResponse* res = new ServerResponse(socket);
  assert(res);
  res->on<native::event::http::finish>([=](){
    DBG("response finished, ending socket");
    res->socket_->end();
  });
  // Pass request and response to listener
  emit<native::event::http::server::request>(req, res);

  return req;
}

Server* createServer()
{
    auto server = new Server();

    return server;
}

} //namespace http
} //namespace native
