

/**
@class OutgoingMessage

# Events #
## Emitted ##
native::event::http::finish
native::event::drain

## Registered ##
native::event::close
native::event::error
native::event::pipe
 */

#include <regex>
#include <ctime>

#include "native/http.h"

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

#undef DBG
#define DBG(msg) DEBUG_PRINT("[OutgoingMessage] " << msg)

OutgoingMessage::OutgoingMessage(net::Socket* socket_)
  : socket_(socket_),
    output_(),
    writable_(true),
    last_(false),
    chunkedEncoding_(false),
    useChunkedEncodingByDefault_(true),
    sendDate_(false),
    hasBody_(true),
    expectContinue_(false),
    sent100_(false),
    shouldKeepAlive_(false),
    trailer_(),
    finished_(false),
    start_line_(),
    headers_(),
    trailers_(),
    header_(),
    headerSent_(false)
{
CRUMB();

  // WritableStream event interface
  registerEvent<native::event::drain>();
  registerEvent<native::event::error>();
  registerEvent<native::event::close>();
  registerEvent<native::event::pipe>();

  registerEvent<native::event::http::finish>();
}

// TODO: handle encoding
void OutgoingMessage::write(const Buffer& buf) {
CRUMB();

//js:  if (!this._header) {
//js:    this._implicitHeader();
//js:  }
  // if headers not yet sent or no headers set send implicit header
  if (!this->headerSent_ && this->header_.size() <= 0) {
    this->_implicitHeader();
  }

//js:  if (!this._hasBody) {
//js:    debug('This type of response MUST NOT have a body. ' +
//js:          'Ignoring write() calls.');
//js:    return true;
//js:  }
  // ignore if this type of response MUST NOT have a body
  if (!this->hasBody_) {
    // TODO: error or ignore and log
    return;
  }

//js:  if (chunk.length === 0) return false;
//js:
//js:  var len, ret;
//js:  if (this.chunkedEncoding) {
  // if chunked encoding enabled
  if (this->chunkedEncoding_) {
//js:    if (typeof(chunk) === 'string') {
//js:      len = Buffer.byteLength(chunk, encoding);
//js:      chunk = len.toString(16) + CRLF + chunk + CRLF;
//js:      ret = this._send(chunk, encoding);
//js:    } else {
//js:      // buffer
//js:      len = chunk.length;
//js:      this._send(len.toString(16) + CRLF);
//js:      this._send(chunk);
//js:      ret = this._send(CRLF);
//js:    }
    // _send(...) length of buffer in hex followed by CRLF
    // TODO: optimize prepending chunk size to Buffer
    std::stringstream ss;
    ss << std::hex << buf.size() << CRLF;
    this->_send(Buffer(ss.str()).append(buf).append(Buffer(CRLF)));

  } else {
//js:  } else {
//js:    ret = this._send(chunk, encoding);
//js:  }
  // otherwise just _send(...) buffer
    this->_send(buf);
  }
//js:  debug('write ret = ' + ret);
//js:  return ret;
  // return result of send(s)
}

void OutgoingMessage::write(const std::string& str) {
  this->write(Buffer(str));
}

void OutgoingMessage::end()
{
  end(Buffer(nullptr));
}
/**
 * Signal the end of the outgoing message
 * @param buf
 * @return
 */
void OutgoingMessage::end(const Buffer& buf) {
CRUMB();

//        assert(socket_);
//
//        if(socket_->end(buf))
//        {
//            socket_ = nullptr;
//        }
//        else
//        {
//            emit<native::event::error>(Exception("Failed to close the socket."));
//        }
//  return false;
        Buffer data(buf);
//js:  if (this.finished) {
//js:    return false;
//js:  }
        // TODO: handle encoding
        if (this->finished_) {
          return;
        }
//js:  if (!this._header) {
//js:    this._implicitHeader();
//js:  }
        if (this->header_.size() <= 0) {
          // headers not yet sent
          // _storeHeader not yet called
          this->_implicitHeader();
        }
//js:  if (data && !this._hasBody) {
//js:    debug('This type of response MUST NOT have a body. ' +
//js:          'Ignoring data passed to end().');
//js:    data = false;
//js:  }
        if (data.size() > 0 && !this->hasBody_) {
          DBG("Response MUST NOT have a body");
          data = Buffer();
        }
//js:  var ret;
//js:
//js:  var hot = this._headerSent === false &&
//js:            typeof(data) === 'string' &&
//js:            data.length > 0 &&
//js:            this.output.length === 0 &&
//js:            this.connection &&
//js:            this.connection.writable &&
//js:            this.connection._httpMessage === this;
        bool hot = !this->headerSent_ &&
            data.size() > 0 &&
            this->output_.size() == 0 &&
            this->socket_ &&
            this->socket_->writable();
//js:  if (hot) {

        if (hot) {
//js:    // Hot path. They're doing
//js:    //   res.writeHead();
//js:    //   res.end(blah);
//js:    // HACKY.
//js:
//js:    if (this.chunkedEncoding) {
//js:      var l = Buffer.byteLength(data, encoding).toString(16);
//js:      ret = this.connection.write(this._header + l + CRLF +
//js:                                  data + '\r\n0\r\n' +
//js:                                  this._trailer + '\r\n', encoding);
//js:    } else {
//js:      ret = this.connection.write(this._header + data, encoding);
//js:    }
//js:    this._headerSent = true;
//js:
//js:  } else if (data) {
//js:    // Normal body write.
//js:    ret = this.write(data, encoding);
//js:  }
          // Hot path. They're doing
          //   res.writeHead()
          //   res.end(blah)
          // HACKY.

          if (this->chunkedEncoding_) {
            // TODO: do this without stringstream
            std::stringstream ss;
            ss << header_.str()
              << std::hex << data.size() << CRLF
              << data.str() << CRLF << 0 << CRLF
              << trailer_.str() << CRLF;
            socket_->write(Buffer(ss.str()));
          } else {
            socket_->write(Buffer(std::string(header_.base(), header_.size()) +
                std::string(data.base(),data.size())));
          }
          this->headerSent_ = true;
        } else if (data.size() > 0) {
          // Normal body write
          write(data);
        }

//js:  if (!hot) {
//js:    if (this.chunkedEncoding) {
//js:      ret = this._send('0\r\n' + this._trailer + '\r\n'); // Last chunk.
//js:    } else {
//js:      // Force a flush, HACK.
//js:      ret = this._send('');
//js:    }
//js:  }
//js:  this.finished = true;
        if (!hot) {
          if (this->chunkedEncoding_) {
            this->_send(Buffer(std::string("0") + CRLF +
                std::string(this->trailer_.base(), this->trailer_.size()) + CRLF
            )); // Last chunk
          } else {
            // Force a flush, HACK.
            this->_send(Buffer());
          }
        }
        this->finished_ = true;
//js:  // There is the first message on the outgoing queue, and we've sent
//js:  // everything to the socket.
//js:  debug('outgoing message end.');
//js:  if (this.output.length === 0 && this.connection._httpMessage === this) {
//js:    this._finish();
//js:  }
//js:
//js:  return ret;

        if (this->output_.size() == 0) {
          this->_finish();
        }
}

void OutgoingMessage::destroy(const Exception& e) {
CRUMB();
  socket_->destroy(e);
}

void OutgoingMessage::method(const http_method& method) {
  this->start_line_.method(method);
}

void OutgoingMessage::status(int value) { 
  // calling this after response was sent is error
  this->start_line_.status(value);
}

void OutgoingMessage::version(const detail::http_version& version) {
  this->start_line_.version(version);
}

void OutgoingMessage::url(const detail::url_obj& url) {
  this->start_line_.url(url);
}

const headers_type& OutgoingMessage::headers() const {
  return headers_;
}

void OutgoingMessage::set_header(const std::string& name,
    const std::string& value) {
  headers_[name] = value;
}

void OutgoingMessage::add_headers(const headers_type& value, bool append) {
  for (headers_type::const_iterator it = value.begin(); it != value.end();
      ++it) {
    add_header(it->first, it->second, append);
  }
}

void OutgoingMessage::add_header(const std::string& name, const std::string& value,
    bool append) {
  headers_[name] =
      (has_header(name) && append) ? headers_[name] + "," + value : value;
}

bool OutgoingMessage::has_header(const std::string& name) {
  return headers_.count(name) > 0;
}

const std::string& OutgoingMessage::get_header(const std::string& name) {
  headers_type::iterator it = headers_.find(name);
  if (it == headers_.end()) {
    // TODO: throw proper error
//    throw Exception("header not found");
  }
  return it->second;
}

void OutgoingMessage::remove_header(const std::string& name) {
  headers_.erase(name);
}

const headers_type& OutgoingMessage::trailers() const {
  return trailers_;
}

void OutgoingMessage::set_trailer(const std::string& name,
    const std::string& value) {
  trailers_[name] = value;
}

void OutgoingMessage::add_trailers(const headers_type& value, bool append) {
  for (headers_type::const_iterator it = value.begin(); it != value.end();
      ++it) {
    add_trailer(it->first, it->second, append);
  }
}
void OutgoingMessage::add_trailer(const std::string& name,
    const std::string& value, bool append) {
  trailers_[name] =
      (has_header(name) && append) ? trailers_[name] + "," + value : value;
}

bool OutgoingMessage::has_trailer(const std::string& name) {
  return trailers_.count(name) > 0;
}

const std::string& OutgoingMessage::get_trailer(const std::string& name) {
  headers_type::iterator it = trailers_.find(name);
  if (it == trailers_.end()) {
    // TODO: throw proper error
//    throw Exception("header not found");
  }
  return it->second;
}

void OutgoingMessage::remove_trailer(const std::string& name) {
  trailers_.erase(name);
}

void OutgoingMessage::_storeHeader(const std::string& firstLine, const headers_type& headers) {
CRUMB();
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

headers_type OutgoingMessage::_renderHeaders() {
  return headers_;
}

void OutgoingMessage::_flush() {
  // This logic is probably a bit confusing. Let me explain a bit:
  //
  // In both HTTP servers and clients it is possible to queue up several
  // outgoing messages. This is easiest to imagine in the case of a client.
  // Take the following situation:
  //
  //    req1 = client.request('GET', '/');
  //    req2 = client.request('POST', '/');
  //
  // When the user does
  //
  //   req2.write('hello world\n');
  //
  // it's possible that the first request has not been completely flushed to
  // the socket yet. Thus the outgoing messages need to be prepared to queue
  // up data internally before sending it on further to the socket's queue.
  //
  // This function, outgoingFlush(), is called by both the Server and Client
  // to attempt to flush any pending messages out to the socket.

//js:  if (!this.socket) return;
  if (!this->socket_) return;
//js:  var ret;
  bool ret = false;
//js:  while (this.output.length) {
  for (std::vector<Buffer>::iterator it = this->output_.begin();
      it != this->output_.end(); ++it) {
//js:
//js:    if (!this.socket.writable) return; // XXX Necessary?
    if (!this->socket_->writable()) return;
//js:
//js:    var data = this.output.shift();
//js:    var encoding = this.outputEncodings.shift();
//js:
//js:    ret = this.socket.write(data, encoding);
    ret = this->socket_->write(*it);
//js:  }
  }
//js:
//js:  if (this.finished) {
//js:    // This is a queue to the server or client to bring in the next this.
//js:    this._finish();
//js:  } else if (ret) {
//js:    // This is necessary to prevent https from breaking
//js:    this.emit('drain');
//js:  }
  if (this->finished_) {
    this->_finish();
  } else if (ret) {
    this->emit<event::drain>();
  }
}

// TODO: handle encoding
void OutgoingMessage::_send(const Buffer& buf) {
  // This is a shameful hack to get the headers and first body chunk onto
  // the same packet. Future versions of Node are going to take care of
  // this at a lower level and in a more general way.
//js:  if (!this._headerSent) {
  if (!this->headerSent_) {
    // TODO: handle output buffering
//js:    if (typeof data === 'string') {
//js:      data = this._header + data;
//js:    } else {
//js:      this.output.unshift(this._header);
//js:      this.outputEncodings.unshift('ascii');
//js:    }
//js:    this._headerSent = true;

    Buffer out(this->header_);
    out.append(buf);

    this->headerSent_ = true;

    return this->_writeRaw(out);
//js:  }
  }
//js:  return this._writeRaw(data, encoding);
    return this->_writeRaw(buf);
}

// TODO: handle encoding
void OutgoingMessage::_writeRaw(const Buffer& buf) {
//  DBG("writing:\n" << buf.str());
//js:  if (data.length === 0) {
//js:    return true;
//js:  }
  if (buf.size() <= 0) return;

//js:  if (this.connection &&
//js:      this.connection._httpMessage === this &&
//js:      this.connection.writable) {
  if (this->socket_ && this->socket_->writable()) {
//js:    // There might be pending data in the this.output buffer.
//js:    while (this.output.length) {
//js:      if (!this.connection.writable) {
//js:        this._buffer(data, encoding);
//js:        return false;
//js:      }
//js:      var c = this.output.shift();
//js:      var e = this.outputEncodings.shift();
//js:      this.connection.write(c, e);
//js:    }
      // TODO: handle output buffering
//js:    // Directly write to socket.
//js:    return this.connection.write(data, encoding);
    this->socket_->write(buf);
//js:  } else {
  } else {
//js:    this._buffer(data, encoding);
//js:    return false;
    this->_buffer(buf);
//js:  }
  }
}

// TODO: handle encoding
void OutgoingMessage::_buffer(const Buffer& buf) {

//js:  if (data.length === 0) return;
  if (buf.size() <= 0) return;
//js:  var length = this.output.length;
//js:
//js:  if (length === 0 || typeof data != 'string') {
  if (this->output_.size() == 0) {
//js:    this.output.push(data);
//js:    this.outputEncodings.push(encoding);
//js:    return false;
    this->output_.push_back(buf);
    return;
//js:  }
  }

//js:  var lastEncoding = this.outputEncodings[length - 1];
//js:  var lastData = this.output[length - 1];
//js:
//js:  if ((encoding && lastEncoding === encoding) ||
//js:      (!encoding && data.constructor === lastData.constructor)) {
      // If same encoding as last, append to previous buffer
//js:    this.output[length - 1] = lastData + data;
//js:    return false;
//js:  }
  // TODO: handle multiple encodings in output

//js:  this.output.push(data);
//js:  this.outputEncodings.push(encoding);
//js:
//js:  return false;
  this->output_.push_back(buf);
  return;
}

void OutgoingMessage::_finish() {
  assert(socket_);
  // TODO: Port DTrace
//js:        if (this instanceof ServerResponse) {
//js:        DTRACE_HTTP_SERVER_RESPONSE(this.connection);
//js:        } else {
//js:        assert(this instanceof ClientRequest);
//js:        DTRACE_HTTP_CLIENT_REQUEST(this, this.connection);
//js:        }
  emit<native::event::http::finish>();
}

} //namespace http
} //namespace native
