
#include "native/http.h"

namespace native {
namespace http {

IncomingMessage::IncomingMessage(net::Socket* socket, detail::http_message* message)
: socket_(socket)
, complete_(false)
, readable_(true)
, paused_(false)
, pendings_()
, endEmitted_(false)
, message_(message)
{
  CRUMB();
  registerEvent<native::event::end>();
}

//      IncomingMessage::IncomingMessage(const IncomingMessage& o)
//      : socket_(o.socket_)
//      , statusCode_(o.statusCode_)
//      , httpVersion_(o.httpVersion_)
//      , httpVersionMajor_(o.httpVersionMajor_)
//      , httpVersionMinor_(o.httpVersionMinor_)
//      , complete_(o.complete_)
//      , headers_(o.headers_)
//      , trailers_(o.trailers_)
//      , readable_(o.readable_)
//      , paused_(o.paused_)
//      , pendings_(o.pendings_)
//      , endEmitted_(o.endEmitted_)
//      , url_(o.url_)
//      , method_(o.method_)
//      {}

void IncomingMessage::pause() { CRUMB(); }
void IncomingMessage::resume() { CRUMB(); }
void IncomingMessage::_emitPending(std::function<void()> callback) { CRUMB(); }
void IncomingMessage::_emitData(const Buffer& buf) { CRUMB(); }
void IncomingMessage::_emitEnd() { CRUMB(); }
void IncomingMessage::_addHeaderLine(const std::string& field, const std::string& value) { CRUMB(); }

void IncomingMessage::end() {
  CRUMB();
  if (haveListener<native::event::end>()) {
    TRACE(emit<native::event::end>())
  }
}

} //namespace http
} //namespace native
