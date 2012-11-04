
#include "native.h"

namespace native {
namespace http {

ClientResponse::ClientResponse(net::Socket* socket)
  : IncomingMessage(socket)
{
    CRUMB();
  assert(socket);
  registerEvent<native::event::data>();
  registerEvent<native::event::end>();
  registerEvent<native::event::close>();
}


ClientRequest::ClientRequest(detail::url_obj url, std::function<void(ClientResponse*)> callback)
  : OutgoingMessage(net::createSocket())
  , method_(HTTP_GET)
  , headers_()
  , path_(url.has_path() ? url.path() : "/")
{
    CRUMB();
  registerEvent<native::event::http::socket>();
  registerEvent<native::event::http::client::response>();
  registerEvent<native::event::error>();
  registerEvent<event::connect>();
  registerEvent<native::event::http::client::upgrade>();
  registerEvent<native::event::http::Continue>();

  int port = url.has_port() ? url.port() : 80;
  std::string host = url.has_host() ? url.host() : "127.0.0.1"; // "localhost"; // TODO: resolve hostname

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
  // Connect socket
  socket_->connect(host, port);
  // Setup socket
  init_socket();

  _deferToConnect([this](){
        CRUMB();
    _flush(); // flush output once connected
  });
}

  void ClientRequest::abort() {
      CRUMB();
    if (socket_) { // in-progress
      socket_->destroy();
    } else {
      // not yet assigned a socket
      // TODO: remove from pending requests
      _deferToConnect([this](){
              CRUMB();
        socket_->destroy();
      });
    }
  }
  void ClientRequest::setTimeout(int64_t timeout, std::function<void()> callback) {}
  void ClientRequest::setNoDelay(bool noDelay) {}
  void ClientRequest::setSocketKeepAlive(bool enable, int64_t initialDelay) {}

  void ClientRequest::_implicitHeader() {
      CRUMB();
    _storeHeader(
        std::string(http_method_str(method_)) + " " + path_ + " HTTP/1.1\r\n",
        _renderHeaders());
  }

  void ClientRequest::_deferToConnect(std::function<void()> callback) {
      CRUMB();
    // if no socket then setup socket event handler
    // if not yet connected setup connect event handler
    // otherwise already connected so run callback
  }

void ClientRequest::init_socket() {
  CRUMB();
  process::nextTick([this](){
    // TODO: check if allocated Parser is leaking
    Parser* parser = Parser::create(HTTP_RESPONSE, socket_);

    // The parser will start reading from the socket and parsing data

    // Setup parser for handling response
    parser->alloc_incoming([](net::Socket* socket){
      return new ClientResponse(socket);
    });

    // TODO: set parser max. headers
    // TODO: Setup drain event
    // TODO: remove drain before setting it
    socket_->on<native::event::connect>([this](){
      this->on_socket_connect();
    });
    socket_->on<native::event::drain>(on_socket_drain);
    socket_->on<native::event::error>(on_socket_error);
    socket_->on<native::event::data>([this](const Buffer& buf){
      this->on_socket_data(buf);
    });
    socket_->on<native::event::end>(on_socket_end);
    socket_->on<native::event::close>(on_socket_close);

    // set on incoming callback on parser
    parser->on_incoming([this](IncomingMessage* res){
      this->on_incoming_message(res);
    });

    // Emit socket event
    emit<native::event::http::socket>(socket_);
  });
}

void ClientRequest::on_incoming_message(IncomingMessage* msg) {
  CRUMB();
  ClientResponse* res = static_cast<ClientResponse*>(msg);

  // if already created response then server sent double response
  // TODO: destroy socket on double response
//            this->response = res;

  // TODO: handle response to CONNECT as UPGRADE

  // TODO: handle response to HEAD request

  // TODO: handle CONTINUE

  // TODO: handle keep-alive

  // Emit response event
  emit<native::event::http::client::response>(res);
  res->on<native::event::end>([this](){
    this->on_response_end();
  });
}

void ClientRequest::on_response_end() {
  // TODO: handle keep-alive after response end
}

void ClientRequest::on_socket_connect() {
    CRUMB();
      assert(socket_);
}

void ClientRequest::on_socket_drain() {
    CRUMB();
}

void ClientRequest::on_socket_error(const Exception& e) {
    CRUMB();
}

void ClientRequest::on_socket_data(const Buffer& buf) {
    CRUMB();
}

void ClientRequest::on_socket_end() {
    CRUMB();
}

void ClientRequest::on_socket_close() {
    CRUMB();
}


ClientRequest* request(const std::string& url_string, std::function<void(ClientResponse*)> callback)
{
  CRUMB();
  detail::url_obj url;
  url.parse(url_string.c_str(), url_string.length());

  if (url.schema() != "http") {
    throw Exception("Protocol: " + url.schema() + " not supported.");
  }

  ClientRequest* req = new ClientRequest(url, callback);

  return req;
}

ClientRequest* get(const std::string& url_string, std::function<void(ClientResponse*)> callback)
{
  CRUMB();
  ClientRequest* req = request(url_string, callback);
  req->end();
  return req;
}

} //namespace http
} //namespace native
