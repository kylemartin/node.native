
#include "native.h"

namespace native { namespace http {

ServerRequest::ServerRequest(const IncomingMessage& msg)
    : IncomingMessage(msg) // TODO: use move constructor
{
  CRUMB();
    assert(socket_);

//                registerEvent<native::event::data>();
//                registerEvent<native::event::end>();
//                registerEvent<native::event::close>();
//
//                socket_->on<native::event::data>([this](const Buffer& buffer){ emit<native::event::data>(buffer); });
//                socket_->on<native::event::end>([this](){ emit<native::event::end>(); });
//                socket_->on<native::event::close>([this](){ emit<native::event::close>(); });
}

http_method ServerRequest::method() {
  return message_.method();
}
detail::url_obj ServerRequest::url() {
  return message_.url();
}

ServerResponse::ServerResponse(net::Socket* socket)
    : OutgoingMessage(socket)
{
  CRUMB();
    assert(socket_);

    registerEvent<native::event::close>();
    registerEvent<native::event::error>();

    socket_->on<native::event::close>([this](){ emit<native::event::close>(); });
}

void ServerResponse::_implicitHeader() {};

void ServerResponse::writeContinue() {}

void ServerResponse::writeHead(int statusCode, const headers_type& headers) {}

void ServerResponse::writeHead(int statusCode, const std::string& given_reasonPhrase
    , const headers_type& given_headers)
{

  //  var reasonPhrase, headers, headerIndex;
  //
  //  if (typeof arguments[1] == 'string') {
  //    reasonPhrase = arguments[1];
  //    headerIndex = 2;
  //  } else {
  //    reasonPhrase = STATUS_CODES[statusCode] || 'unknown';
  //    headerIndex = 1;
  //  }
  //  this.statusCode = statusCode;
  std::string reasonPhrase(!given_reasonPhrase.empty()
      ? given_reasonPhrase : detail::http_status_text(statusCode));

  //
  //  var obj = arguments[headerIndex];
  //
  //  if (obj && this._headers) {
  //    // Slow-case: when progressive API and header fields are passed.
  //    headers = this._renderHeaders();
  //
  //    if (Array.isArray(obj)) {
  //      // handle array case
  //      // TODO: remove when array is no longer accepted
  //      var field;
  //      for (var i = 0, len = obj.length; i < len; ++i) {
  //        field = obj[i][0];
  //        if (field in headers) {
  //          obj.push([field, headers[field]]);
  //        }
  //      }
  //      headers = obj;
  //
  //    } else {
  //      // handle object case
  //      var keys = Object.keys(obj);
  //      for (var i = 0; i < keys.length; i++) {
  //        var k = keys[i];
  //        if (k) headers[k] = obj[k];
  //      }
  //    }
  //  } else if (this._headers) {
  //    // only progressive api is used
  //    headers = this._renderHeaders();
  //  } else {
  //    // only writeHead() called
  //    headers = obj;
  //  }

  // only progressive api is used
  headers_type headers = this->_renderHeaders();

  this->statusCode(statusCode);
  if (!given_headers.empty() && !this->message_.headers().empty()) {
    // Slow-case: when progressive API and header fields are passed.
    headers.insert(given_headers.begin(), given_headers.end());
  } else if (this->message_.headers().empty()) {
    // only writeHead() called
    headers = given_headers;
  }

  //
  //  var statusLine = 'HTTP/1.1 ' + statusCode.toString() + ' ' +
  //                   reasonPhrase + CRLF;
  //
  std::string statusLine = "HTTP/1.1" + std::to_string(statusCode) + " " + reasonPhrase + "\r\n";

//  if (statusCode === 204 || statusCode === 304 ||
//      (100 <= statusCode && statusCode <= 199)) {
//    // RFC 2616, 10.2.5:
//    // The 204 response MUST NOT include a message-body, and thus is always
//    // terminated by the first empty line after the header fields.
//    // RFC 2616, 10.3.5:
//    // The 304 response MUST NOT contain a message-body, and thus is always
//    // terminated by the first empty line after the header fields.
//    // RFC 2616, 10.1 Informational 1xx:
//    // This class of status code indicates a provisional response,
//    // consisting only of the Status-Line and optional headers, and is
//    // terminated by an empty line.
//    this._hasBody = false;
//  }

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

//
//  // don't keep alive connections where the client expects 100 Continue
//  // but we sent a final status; they may put extra bytes on the wire.
//  if (this._expect_continue && ! this._sent100) {
//    this.shouldKeepAlive = false;
//  }
//
  if (this->expectContinue_ && !this->sent100_) {
    this->shouldKeepAlive_ = false;
  }
//  this._storeHeader(statusLine, headers);
  this->_storeHeader(statusLine, headers);
}

int ServerResponse::statusCode() const { return 0; }
void ServerResponse::statusCode(int value) {} // calling this after response was sent is error

void ServerResponse::setHeader(const std::string& name, const std::string& value) {}
std::string ServerResponse::getHeader(const std::string& name, const std::string& default_value) { return default_value; }
bool ServerResponse::getHeader(const std::string& name, std::string& value) { return false; }
bool ServerResponse::removeHeader(const std::string& name) { return false; }

void ServerResponse::write(const Buffer& data)
{
  CRUMB();
    socket_->write(data);
}

void ServerResponse::write(const std::string& data)
{
  CRUMB();
  socket_->write(Buffer(data));
}

void ServerResponse::addTrailers(const headers_type& headers) {}

void ServerResponse::end(const Buffer& data)
{
  CRUMB();
    assert(socket_);

    if(socket_->end(data))
    {
      CRUMB();
        socket_ = nullptr;
    }
    else
    {
      CRUMB();
        emit<native::event::error>(Exception("Failed to close the socket."));
    }
}

void ServerResponse::end()
{
  end(Buffer(nullptr));
}

Server::Server()
    : net::Server()
{
  CRUMB();
    registerEvent<native::event::http::server::request>();
    registerEvent<native::event::http::server::checkContinue>();
    registerEvent<native::event::http::server::upgrade>();
    registerEvent<native::event::connection>();
    registerEvent<native::event::close>();
    registerEvent<native::event::error>();

    // Register connection listener
    on<native::event::connection>([&](net::Socket* socket){
      CRUMB();
        assert(socket);

        // Create a Parser to handle incoming message
        // TODO: check if allocated Parser are leaking
        Parser* parser(Parser::create(HTTP_REQUEST, socket));

        // The parser will start reading from the socket and parsing data

        parser->alloc_incoming([](net::Socket* socket){
          return new ServerRequest(socket);
        });

        // After receiving headers it will create an IncomingMessage and invoke this callback
        parser->on_incoming([=](IncomingMessage* msg) {
          CRUMB();
          // Create ServerRequest from IncomingMessage
          ServerRequest* req = static_cast<ServerRequest*>(msg);

          assert(req);

          // Do early header processing and decide whether body should be received

          // Request isn't finished yet, when it ends, emit request event
          req->on<native::event::end>([=](){
            CRUMB();
            // Create ServerResponse
            ServerResponse* res = new ServerResponse(msg->socket());
            assert(res);
            // Pass request and response to listener
            emit<native::event::http::server::request>(req, res);

              // TODO: handle cleanup of ServerRequest better
//                        delete req;
          });

//                      return 0; // Don't skip body
//                      return 1; // Skip body
        });
    });
}

Server* createServer()
{
  CRUMB();
    auto server = new Server();

    return server;
}

} //namespace http
} //namespace native
