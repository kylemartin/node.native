
#include <regex>
#include <ctime>

#include "native.h"

namespace native { namespace http {

std::regex connectionExpression ("connection");
std::regex transferEncodingExpression ("transfer-encoding");
std::regex closeExpression ("close");
std::regex chunkExpression ("chunk");
std::regex contentLengthExpression ("content-length");
std::regex dateExpression ("date");
std::regex expectExpression ("expect");
std::regex continueExpression ("100-continue");


static std::string utcDate() {
  char date[128];
  std::time_t now = std::time(nullptr);
  std::strftime(date, 128, "%c %Z", std::gmtime(&now));
  return std::string(date);
}

void OutgoingMessage::_storeHeader(const std::string& firstLine, const detail::http_message::headers_type& headers) {

  bool sentConnectionHeader = false;
  bool sentContentLengthHeader = false;
  bool sentTransferEncodingHeader = false;
  bool sentDateHeader = false;
  bool sentExpect = false;

  // firstLine in the case of request is: 'GET /index.html HTTP/1.1\r\n'
  // in the case of response it is: 'HTTP/1.1 200 OK\r\n'
  std::string messageHeader = firstLine;

  if (!headers.empty()) {
    headers_type::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
      const std::string& field = it->first;
      const std::string& value = it->second;

      messageHeader += field + ": " + value + "\r\n";

      if (std::regex_match(field, connectionExpression, std::regex_constants::icase)) {
        sentConnectionHeader = true;
        if (std::regex_match(field, closeExpression, std::regex_constants::icase)) {
          this->last_ = true;
        } else {
          this->shouldKeepAlive_ = true;
        }
      } else if (std::regex_match(field, transferEncodingExpression, std::regex_constants::icase)) {
        sentTransferEncodingHeader = true;
        if (std::regex_match(value, chunkExpression, std::regex_constants::icase)) {
          this->chunkedEncoding_ = true;
        }
      } else if (std::regex_match(field, contentLengthExpression, std::regex_constants::icase)) {
        sentContentLengthHeader = true;
      } else if (std::regex_match(field, dateExpression, std::regex_constants::icase)) {
        sentDateHeader = true;
      } else if (std::regex_match(field, expectExpression, std::regex_constants::icase)) {
        sentExpect = true;
      }
    }
  }

  // Date header
  if (this->sendDate_ == true && sentDateHeader == false) {
    messageHeader += "Date: " + utcDate() + "\r\n";
  }

  // keep-alive logic
  if (sentConnectionHeader == false) {
    if (this->shouldKeepAlive_ &&
        (sentContentLengthHeader || this->useChunkedEncodingByDefault_)) {
      messageHeader += "Connection: keep-alive\r\n";
    } else {
      this->last_ = true;
      messageHeader += "Connection: close\r\n";
    }
  }

  if (sentContentLengthHeader == false && sentTransferEncodingHeader == false) {
    if (this->hasBody_) {
      if (this->useChunkedEncodingByDefault_) {
        messageHeader += "Transfer-Encoding: chunked\r\n";
        this->chunkedEncoding_ = true;
      } else {
        this->last_ = true;
      }
    } else {
      // Make sure we don't end the 0\r\n\r\n at the end of the message.
      this->chunkedEncoding_ = false;
    }
  }

  this->header_ = Buffer(messageHeader + "\r\n");
  this->headerSent_ = false;

  // wait until the first body chunk, or close(), is sent to flush,
  // UNLESS we're sending Expect: 100-continue.
  if (sentExpect) this->_send(Buffer(""));
}

// TODO: handle encoding
void OutgoingMessage::_send(const Buffer& buf) {

  // This is a shameful hack to get the headers and first body chunk onto
  // the same packet. Future versions of Node are going to take care of
  // this at a lower level and in a more general way.
//  if (!this->headerSent_) {
//    if (typeof data === 'string') {
//      data = this._header + data;
//    } else {
//      this.output.unshift(this._header);
//      this.outputEncodings.unshift('ascii');
//    }
//    this._headerSent = true;
//  }
//  return this._writeRaw(data, encoding);
}

// TODO: handle encoding
void OutgoingMessage::_writeRaw(const Buffer& buf) {

//  if (data.length === 0) {
//    return true;
//  }
//
//  if (this.connection &&
//      this.connection._httpMessage === this &&
//      this.connection.writable) {
//    // There might be pending data in the this.output buffer.
//    while (this.output.length) {
//      if (!this.connection.writable) {
//        this._buffer(data, encoding);
//        return false;
//      }
//      var c = this.output.shift();
//      var e = this.outputEncodings.shift();
//      this.connection.write(c, e);
//    }
//
//    // Directly write to socket.
//    return this.connection.write(data, encoding);
//  } else {
//    this._buffer(data, encoding);
//    return false;
//  }
}

// TODO: handle encoding
void OutgoingMessage::_buffer(const Buffer& buf) {

//  if (data.length === 0) return;
//
//  var length = this.output.length;
//
//  if (length === 0 || typeof data != 'string') {
//    this.output.push(data);
//    this.outputEncodings.push(encoding);
//    return false;
//  }
//
//  var lastEncoding = this.outputEncodings[length - 1];
//  var lastData = this.output[length - 1];
//
//  if ((encoding && lastEncoding === encoding) ||
//      (!encoding && data.constructor === lastData.constructor)) {
//    this.output[length - 1] = lastData + data;
//    return false;
//  }
//
//  this.output.push(data);
//  this.outputEncodings.push(encoding);
//
//  return false;
}


void OutgoingMessage::setHeader(const std::string& name, const std::string& value) {}

OutgoingMessage::OutgoingMessage(net::Socket* socket_)
  : socket_(socket_),
    output_(),
    writable_(true),
    last_(false),
    chunkedEncoding_(false),
    useChunkedEncodingByDefault_(true),
    sendDate_(false),
    hasBody_(false),
    expectContinue_(false),
    sent100_(false),
    shouldKeepAlive_(false),
    trailer_(),
    finished_(false),
    message_(),
    header_(),
    headerSent_(false)
{}

void OutgoingMessage::destroy(const Exception& e) {
  socket_->destroy(e);
}

std::string OutgoingMessage::getHeader(const std::string& name) {
  return message_.get_header(name);
}

void OutgoingMessage::removeHeader(const std::string& name) {
  message_.remove_header(name);
}

detail::http_message::headers_type OutgoingMessage::_renderHeaders() {
  return message_.headers();
}

// TODO: handle encoding
void OutgoingMessage::write(const Buffer& buf) {}

void OutgoingMessage::addTrailers(const detail::http_message::headers_type& headers) {}

bool OutgoingMessage::end() { return false; }
/**
 * Signal the end of the outgoing message
 * @param buf
 * @return
 */
bool OutgoingMessage::end(const Buffer& buf) {
        CRUMB();
  return false;
#if 0
  // TODO: handle encoding
  if (finished_) { return false; }
  if (!_header) { // headers not yet sent
    // _storeHeader not yet called
    _implicitHeader();
  }
  if (data && !_hasBody) {
    // This type of message should not have a body
    data.clear();
  }

  bool hot = _headerSent == false
      && data.size() > 0
      && output.size() == 0
      && socket_
      && socket_->writable()
      && socket_->_httpMessage == this;

  if (hot) {
    // Hot path. They're doing
    //   res.writeHead()
    //   res.end(blah)
    // HACKY.

    if (chunkedEncoding_) {
      // TODO: do this without stringstream
      std::stringstream ss;
      ss << _header << std::hex << data.size() << "\r\n"
        << data << "\r\n0\r\n"
        << _trailer << "\r\n";
      ret - socket_->write(ss.str());
    } else {
      ret = socket_->write(header_ + data);
    }
    _headerSent = true;
  } else if (data.size() > 0) {
    // Normal body write
    ret = write(data);
  }

  if (!hot) {
    if (chunkedEncoding_) {
      ret = _send("0\r\n" + _trailer + "\r\n");
    } else {
      // Force a flush, HACK.
      ret = _send("");
    }
  }

  _finished = true;
  if (output.size() == 0 && socket_->_httpMessage == this) {
    _finish();
  }

  return ret;
#endif
}

void OutgoingMessage::_finish() {
  assert(socket_);
  // TODO: Port DTrace
//        if (this instanceof ServerResponse) {
//        DTRACE_HTTP_SERVER_RESPONSE(this.connection);
//        } else {
//        assert(this instanceof ClientRequest);
//        DTRACE_HTTP_CLIENT_REQUEST(this, this.connection);
//        }
  emit<native::event::http::finish>();
}

void OutgoingMessage::_flush() {}

} //namespace http
} //namespace native
