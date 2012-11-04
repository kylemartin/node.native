
#include <regex>
#include <ctime>

#include "native.h"

namespace native { namespace http {

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

  //  var field, value;
  //  var self = this;
  //
  //  function store(field, value) {
  //    messageHeader += field + ': ' + value + CRLF;
  //
  //    if (connectionExpression.test(field)) {
  //      sentConnectionHeader = true;
  //      if (closeExpression.test(value)) {
  //        self._last = true;
  //      } else {
  //        self.shouldKeepAlive = true;
  //      }
  //
  //    } else if (transferEncodingExpression.test(field)) {
  //      sentTransferEncodingHeader = true;
  //      if (chunkExpression.test(value)) self.chunkedEncoding = true;
  //
  //    } else if (contentLengthExpression.test(field)) {
  //      sentContentLengthHeader = true;
  //    } else if (dateExpression.test(field)) {
  //      sentDateHeader = true;
  //    } else if (expectExpression.test(field)) {
  //      sentExpect = true;
  //    }
  //  }

//
//  if (headers) {
//    var keys = Object.keys(headers);
//    var isArray = (Array.isArray(headers));
//    var field, value;
//
//    for (var i = 0, l = keys.length; i < l; i++) {
//      var key = keys[i];
//      if (isArray) {
//        field = headers[key][0];
//        value = headers[key][1];
//      } else {
//        field = key;
//        value = headers[key];
//      }
//
//      if (Array.isArray(value)) {
//        for (var j = 0; j < value.length; j++) {
//          store(field, value[j]);
//        }
//      } else {
//        store(field, value);
//      }
//    }
//  }
//

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
//  // Date header
//  if (this.sendDate == true && sentDateHeader == false) {
//    messageHeader += 'Date: ' + utcDate() + CRLF;
//  }

  if (this->sendDate_ == true && sentDateHeader == false) {
    messageHeader += "Date: " + utcDate() + "\r\n";
  }

//
//  // keep-alive logic
//  if (sentConnectionHeader === false) {
//    var shouldSendKeepAlive = this.shouldKeepAlive &&
//        (sentContentLengthHeader ||
//         this.useChunkedEncodingByDefault ||
//         this.agent);
//    if (shouldSendKeepAlive) {
//      messageHeader += 'Connection: keep-alive\r\n';
//    } else {
//      this._last = true;
//      messageHeader += 'Connection: close\r\n';
//    }
//  }
//
  if (sentConnectionHeader == false) {
    if (this->shouldKeepAlive_ &&
        (sentContentLengthHeader || this->useChunkedEncodingByDefault_)) {
      messageHeader += "Connection: keep-alive\r\n";
    } else {
      this->last_ = true;
      messageHeader += "Connection: close\r\n";
    }
  }
//  if (sentContentLengthHeader == false && sentTransferEncodingHeader == false) {
//    if (this._hasBody) {
//      if (this.useChunkedEncodingByDefault) {
//        messageHeader += 'Transfer-Encoding: chunked\r\n';
//        this.chunkedEncoding = true;
//      } else {
//        this._last = true;
//      }
//    } else {
//      // Make sure we don't end the 0\r\n\r\n at the end of the message.
//      this.chunkedEncoding = false;
//    }
//  }
//
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
//  this._header = messageHeader + CRLF;
//  this._headerSent = false;

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

} //namespace http
} //namespace native
