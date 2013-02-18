#include "native/http.h"

namespace native {
namespace http {

/* Client Response ************************************************************/
#undef DBG
#define DBG(msg) DEBUG_PRINT("[ClientResponse] " << msg)
ClientResponse::ClientResponse(net::Socket* socket, Parser* parser) :
    IncomingMessage(socket, parser) {
  DBG("constructing");
}

/* Client Request *************************************************************/
#undef DBG
#define DBG(msg) DEBUG_PRINT("[ClientRequest] " << msg)

void ClientRequest::do_connect(ClientRequest* self, const std::string& host, int port) {
  DBG("connecting to: " << host << ":" << port);

  net::Socket* socket = net::createSocket();

  // Connect socket
  assert(socket);
  socket->connect(host, port);
  // Setup socket
  self->init_socket(socket);

}

ClientRequest::ClientRequest(detail::url_obj url,
    std::function<void(ClientResponse*)> callback) :
    OutgoingMessage(nullptr), method_(HTTP_GET), headers_(),
    path_(url.path_query_fragment()) {
  DBG("constructing");
  registerEvent<native::event::http::socket>();
  registerEvent<event::connect>();
  registerEvent<native::event::http::client::upgrade>();
  registerEvent<native::event::http::Continue>();
  registerEvent<native::event::http::client::response>();
  registerEvent<native::event::error>();

  int port = url.has_port() ? url.port() : 80;
  std::string host = url.has_host() ? url.host() : "localhost";
  DBG("connecting to: " << host << ":" << port);
  // resolve hostname

  if (!detail::dns::is_ip(host)) {
    detail::dns::QueryGetHostByName(host, [=](int status, const std::vector<std::string>& results, int family){
      if (results.size()) {
        DBG("resolved host: " << host << " to: " << results[0]);
        do_connect(this, results[0], port);
      } else {
        DBG("could not resolve host: " << host);
      }
    });
  } else {
    do_connect(this, host, port);
  }

  if (callback) {
    once<native::event::http::client::response>(callback);
  }

  // TODO: set headers
  // TODO: set default host header
  // TODO: set authorization header if requested
  // TODO: disable chunk encoding by defauult based on method (GET, HEAD, CONNECT)
  // TODO: setup request header based on method and expect header
  // Setup socket
  // TODO: handle unix socket
  // TODO: utilize user agent if provided
  // TODO: utilize user connection method if provided

  _deferToConnect([this]() {
    DBG("deferred flush");
    _flush(); // flush output once connected
    });
}

void ClientRequest::abort() {
  DBG("abort");
  if (socket_) { // in-progress
    socket_->destroy();
  } else {
    // not yet assigned a socket
    // TODO: remove from pending requests
    _deferToConnect([this]() {
      DBG("deferred destroy");
      socket_->destroy();
    });
  }
}
void ClientRequest::setTimeout(int64_t timeout,
    std::function<void()> callback) {
}
void ClientRequest::setNoDelay(bool noDelay) {
}
void ClientRequest::setSocketKeepAlive(bool enable, int64_t initialDelay) {
}

void ClientRequest::_implicitHeader() {
  DBG("sending implicit header");
  _storeHeader(
      std::string(http_method_str(method_)) + " " + path_ + " HTTP/1.1\r\n",
      _renderHeaders());
}

void ClientRequest::_deferToConnect(std::function<void()> callback) {
  DBG("defer to connect");
  /*
   * Right now the socket is created when the request is constructed, but in the
   * future it may be assigned from a pool, and the request will be queued, so
   * we will need to register a socket event handler
   */
//js:    // This function is for calls that need to happen once the socket is
//js:    // connected and writable. It's an important promisy thing for all the socket
//js:    // calls that happen either now (when a socket is assigned) or
//js:    // in the future (when a socket gets assigned out of the pool and is
//js:    // eventually writable).
//js:    var self = this;
//js:    var onSocket = function() {
//js:      if (self.socket.writable) {
//js:        if (method) {
//js:          self.socket[method].apply(self.socket, arguments_);
//js:        }
//js:        if (cb) { cb(); }
//js:      } else {
//js:        self.socket.once('connect', function() {
//js:          if (method) {
//js:            self.socket[method].apply(self.socket, arguments_);
//js:          }
//js:          if (cb) { cb(); }
//js:        });
//js:      }
//js:    }
//js:    if (!self.socket) {
//js:      self.once('socket', onSocket);
//js:    } else {
//js:      onSocket();
//js:    }

  if (callback) {
    auto handle_on_socket = [=](net::Socket* socket){
      if (!socket->connected()) {
        // if not yet connected setup connect event handler
        socket->once<native::event::connect>(callback);
      } else {
        // otherwise already connected so run callback
        callback();
      }
    };

    if (!this->socket_) {
      // if socket not yet available
      once<native::event::http::socket>(handle_on_socket);
    } else {
      handle_on_socket(this->socket_);
    }
  }
}

void ClientRequest::init_socket(net::Socket* socket) {
//js:  ClientRequest.prototype.onSocket = function(socket) {
//js:    var req = this;
//js:
//js:    process.nextTick(function() {
//js:      var parser = parsers.alloc();
//js:      req.socket = socket;
//js:      req.connection = socket;
//js:      parser.reinitialize(HTTPParser.RESPONSE);
//js:      parser.socket = socket;
//js:      parser.incoming = null;
//js:      req.parser = parser;
//js:
//js:      socket.parser = parser;
//js:      socket._httpMessage = req;
//js:
//js:      // Setup "drain" propogation.
//js:      httpSocketSetup(socket);
//js:
//js:      // Propagate headers limit from request object to parser
//js:      if (typeof req.maxHeadersCount === 'number') {
//js:        parser.maxHeaderPairs = req.maxHeadersCount << 1;
//js:      } else {
//js:        // Set default value because parser may be reused from FreeList
//js:        parser.maxHeaderPairs = 2000;
//js:      }
//js:
//js:      socket.on('error', socketErrorListener);
//js:      socket.ondata = socketOnData;
//js:      socket.onend = socketOnEnd;
//js:      socket.on('close', socketCloseListener);
//js:      parser.onIncoming = parserOnIncomingClient;
//js:      req.emit('socket', socket);
//js:    });
//js:
//js:  };
  process::nextTick([=]() {
    DBG("initializing socket");
    socket_ = socket;
    // TODO: check if allocated Parser is leaking
    Parser* parser = Parser::create(HTTP_RESPONSE, socket_);

    // The parser will start reading from the socket and parsing data

    // TODO: set parser max. headers
    // TODO: Setup drain event
    // TODO: remove drain before setting it
    socket_->on<native::event::connect>([this]() {
          this->on_socket_connect();
        });
    socket_->on<native::event::drain>([this]() {
          this->on_socket_drain();
        });
    socket_error_listener_ = socket_->on<native::event::error>([this](const Exception& e) {
          this->on_socket_error(e);
        });
    socket_->on<native::event::data>([this](const Buffer& buf) {
          this->on_socket_data(buf);
        });
    socket_->on<native::event::end>([this]() {
          this->on_socket_end();
        });
    socket_close_listener_ = socket_->on<native::event::close>([this]() {
          this->on_socket_close();
        });

    // set on incoming callback on parser
    parser->register_on_incoming([=](net::Socket* socket,
        Parser* parser) {
          DBG("constructing ClientResponse for parser");
          IncomingMessage* result = new ClientResponse(socket, parser);
          this->on_incoming_message(result);
          return result;

        });

    // Emit socket event
    emit<native::event::http::socket>(socket_);
  });
}

void ClientRequest::on_incoming_message(IncomingMessage* msg) {
  DBG("handling incoming message");
  ClientResponse* res = static_cast<ClientResponse*>(msg);
  net::Socket* socket = res->socket();

//js:              if (req.res) {
//js:                // We already have a response object, this means the server
//js:                // sent a double response.
//js:                socket.destroy();
//js:                return;
//js:              }
//js:              req.res = res;
  // if already created response then server sent double response
  if (this->received_response_) {
    // destroy socket on double response
    socket->destroy();
  }
  this->received_response_ = true;

//js:              // Responses to CONNECT request is handled as Upgrade.
//js:              if (req.method === 'CONNECT') {
//js:                res.upgrade = true;
//js:                return true; // skip body
//js:              }
  // TODO: handle response to CONNECT as UPGRADE

//js:              // Responses to HEAD requests are crazy.
//js:              // HEAD responses aren't allowed to have an entity-body
//js:              // but *can* have a content-length which actually corresponds
//js:              // to the content-length of the entity-body had the request
//js:              // been a GET.
//js:              var isHeadResponse = req.method == 'HEAD';
//js:              debug('AGENT isHeadResponse ' + isHeadResponse);
//js:
  // TODO: handle response to HEAD request

//js:              if (res.statusCode == 100) {
//js:                // restart the parser, as this is a continue message.
//js:                delete req.res; // Clear res so that we don't hit double-responses.
//js:                req.emit('continue');
//js:                return true;
//js:              }
  // TODO: handle CONTINUE

//js:              if (req.shouldKeepAlive && !shouldKeepAlive && !req.upgradeOrConnect) {
//js:                // Server MUST respond with Connection:keep-alive for us to enable it.
//js:                // If we've been upgraded (via WebSockets) we also shouldn't try to
//js:                // keep the connection open.
//js:                req.shouldKeepAlive = false;
//js:              }
  // TODO: handle keep-alive

//js:              DTRACE_HTTP_CLIENT_RESPONSE(socket, req);
//js:              req.emit('response', res);
  // Emit response event
  emit<native::event::http::client::response>(res);
//js:              req.res = res;
//js:              res.req = req;
//js:
//js:              res.on('end', responseOnEnd);
  res->on<native::event::end>([this]() {
    this->on_response_end();
  });

}

void ClientRequest::on_response_end() {
//js:            function responseOnEnd() {
//js:              var res = this;
//js:              var req = res.req;
//js:              var socket = req.socket;
//js:
//js:              if (!req.shouldKeepAlive) {
//js:                if (socket.writable) {
//js:                  debug('AGENT socket.destroySoon()');
//js:                  socket.destroySoon();
//js:                }
//js:                assert(!socket.writable);
//js:              } else {
//js:                debug('AGENT socket keep-alive');
//js:                socket.removeListener('close', socketCloseListener);
//js:                socket.removeListener('error', socketErrorListener);
//js:                socket.emit('free');
//js:              }
//js:            }
  // TODO: handle keep-alive after response end
  if (!this->shouldKeepAlive_) {
    if (this->socket_->writable()) {
      DBG("AGENT socket.destroySoon()");
      this->socket_->destroySoon();
    }
    assert(!this->socket_->writable());
  } else {
    DBG("AGENT socket keep-alive");
    this->socket_->removeListener<event::close>(socket_close_listener_);
    this->socket_->removeListener<event::error>(socket_error_listener_);
    this->socket_->emit<native::event::free>();
  }
}

void ClientRequest::on_socket_connect() {
  assert(socket_);
  emit<native::event::connect>();
}

void ClientRequest::on_socket_drain() {
}

void ClientRequest::on_socket_error(const Exception& e) {

//js:  function socketErrorListener(err) {
//js:    var socket = this;
//js:    var parser = socket.parser;
//js:    var req = socket._httpMessage;
//js:    debug('HTTP SOCKET ERROR: ' + err.message + '\n' + err.stack);
//js:
//js:    if (req) {
//js:      req.emit('error', err);
//js:      // For Safety. Some additional errors might fire later on
//js:      // and we need to make sure we don't double-fire the error event.
//js:      req._hadError = true;
//js:    }
//js:
//js:    if (parser) {
//js:      parser.finish();
//js:      freeParser(parser, req);
//js:    }
//js:    socket.destroy();
//js:  }

  this->emit<event::error>(e);
}

void ClientRequest::on_socket_data(const Buffer& buf) {
//js:  function socketOnData(d, start, end) {
//js:    var socket = this;
//js:    var req = this._httpMessage;
//js:    var parser = this.parser;
//js:
//js:    var ret = parser.execute(d, start, end - start);
//js:    if (ret instanceof Error) {
//js:      debug('parse error');
//js:      freeParser(parser, req);
//js:      socket.destroy(ret);
//js:    } else if (parser.incoming && parser.incoming.upgrade) {
//js:      // Upgrade or CONNECT
//js:      var bytesParsed = ret;
//js:      var res = parser.incoming;
//js:      req.res = res;
//js:
//js:      socket.ondata = null;
//js:      socket.onend = null;
//js:      parser.finish();
//js:
//js:      // This is start + byteParsed
//js:      var bodyHead = d.slice(start + bytesParsed, end);
//js:
//js:      var eventName = req.method === 'CONNECT' ? 'connect' : 'upgrade';
//js:      if (req.listeners(eventName).length) {
//js:        req.upgradeOrConnect = true;
//js:
//js:        // detach the socket
//js:        socket.emit('agentRemove');
//js:        socket.removeListener('close', socketCloseListener);
//js:        socket.removeListener('error', socketErrorListener);
//js:
//js:        req.emit(eventName, res, socket, bodyHead);
//js:        req.emit('close');
//js:      } else {
//js:        // Got Upgrade header or CONNECT method, but have no handler.
//js:        socket.destroy();
//js:      }
//js:      freeParser(parser, req);
//js:    } else if (parser.incoming && parser.incoming.complete &&
//js:               // When the status code is 100 (Continue), the server will
//js:               // send a final response after this client sends a request
//js:               // body. So, we must not free the parser.
//js:               parser.incoming.statusCode !== 100) {
//js:      freeParser(parser, req);
//js:    }
//js:  }
}

void ClientRequest::on_socket_end() {
//js:  function socketOnEnd() {
//js:    var socket = this;
//js:    var req = this._httpMessage;
//js:    var parser = this.parser;
//js:
//js:    if (!req.res) {
//js:      // If we don't have a response then we know that the socket
//js:      // ended prematurely and we need to emit an error on the request.
//js:      req.emit('error', createHangUpError());
//js:      req._hadError = true;
//js:    }
//js:    if (parser) {
//js:      parser.finish();
//js:      freeParser(parser, req);
//js:    }
//js:    socket.destroy();
//js:  }
}

void ClientRequest::on_socket_close() {
//js:  function socketCloseListener() {
//js:    var socket = this;
//js:    var parser = socket.parser;
//js:    var req = socket._httpMessage;
//js:    debug('HTTP socket close');
//js:    req.emit('close');
//js:    if (req.res && req.res.readable) {
//js:      // Socket closed before we emitted 'end' below.
//js:      req.res.emit('aborted');
//js:      var res = req.res;
//js:      req.res._emitPending(function() {
//js:        res._emitEnd();
//js:        res.emit('close');
//js:        res = null;
//js:      });
//js:    } else if (!req.res && !req._hadError) {
//js:      // This socket error fired before we started to
//js:      // receive a response. The error needs to
//js:      // fire on the request.
//js:      req.emit('error', createHangUpError());
//js:    }
//js:
//js:    if (parser) {
//js:      parser.finish();
//js:      freeParser(parser, req);
//js:    }
//js:  }
}

ClientRequest* request(const std::string& url_string,
    std::function<void(ClientResponse*)> callback) {
  detail::url_obj url;
  url.parse(url_string.c_str(), url_string.length());

  if (url.schema() != "http") {
    throw Exception("Protocol: " + url.schema() + " not supported.");
  }

  ClientRequest* req = new ClientRequest(url, callback);

  return req;
}

ClientRequest* get(const std::string& url_string,
    std::function<void(ClientResponse*)> callback) {
  ClientRequest* req = request(url_string, callback);
  req->end();
  return req;
}

} //namespace http
} //namespace native
