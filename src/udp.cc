#include "native/udp.h"
#include "native/detail/buffers.h"
#include "native/error.h"

namespace native {

//// Copyright Joyent, Inc. and other Node contributors.
////
//// Permission is hereby granted, free of charge, to any person obtaining a
//// copy of this software and associated documentation files (the
//// "Software"), to deal in the Software without restriction, including
//// without limitation the rights to use, copy, modify, merge, publish,
//// distribute, sublicense, and/or sell copies of the Software, and to permit
//// persons to whom the Software is furnished to do so, subject to the
//// following conditions:
////
//// The above copyright notice and this permission notice shall be included
//// in all copies or substantial portions of the Software.
////
//// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
//// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
//// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
//// USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//var util = require('util');
//var events = require('events');
//
//var UDP = process.binding('udp_wrap').UDP;
//
//// lazily loaded
//var dns = null;
//var net = null;
//
//
//// no-op callback
//function noop() {
//}
//
//
//function isIP(address) {
//  if (!net)
//    net = require('net');
//
//  return net.isIP(address);
//}
//
//
//function lookup(address, family, callback) {
//  if (!dns)
//    dns = require('dns');
//
//  return dns.lookup(address, family, callback);
//}
//
//
//function lookup4(address, callback) {
//  return lookup(address || '0.0.0.0', 4, callback);
//}
//
//
//function lookup6(address, callback) {
//  return lookup(address || '::0', 6, callback);
//}
//
//
//function newHandle(type) {
//  if (type == 'udp4') {
//    var handle = new UDP;
//    handle.lookup = lookup4;
//    return handle;
//  }
//
//  if (type == 'udp6') {
//    var handle = new UDP;
//    handle.lookup = lookup6;
//    handle.bind = handle.bind6;
//    handle.send = handle.send6;
//    return handle;
//  }
//
//  if (type == 'unix_dgram')
//    throw new Error('unix_dgram sockets are not supported any more.');
//
//  throw new Error('Bad socket type specified. Valid types are: udp4, udp6');
//}
//
//
//function Socket(type, listener) {
//  events.EventEmitter.call(this);
//
//  var handle = newHandle(type);
//  handle.owner = this;
//
//  this._handle = handle;
//  this._receiving = false;
//  this._bound = false;
//  this.type = type;
//  this.fd = null; // compatibility hack
//
//  if (typeof listener === 'function')
//    this.on('message', listener);
//}
//util.inherits(Socket, events.EventEmitter);
//exports.Socket = Socket;
//

/**
 * Event: 'message'
 * Event: 'listening'
 * Event: 'close'
 * Event: 'error'
 */
udp::udp(const udp_type& type)
: EventEmitter(), type_(type), udp_(), receiving_(false), bound_(false)
{
  registerEvent<native::event::udp::message>();
  registerEvent<native::event::listening>();
  registerEvent<native::event::close>();
  registerEvent<native::event::error>();

  udp_.on_message([this](detail::udp* self, const detail::Buffer& buf, std::shared_ptr<detail::net_addr> address){
    this->on_message(self, buf, address);
  });
}

//exports.createSocket = function(type, listener) {
//  return new Socket(type, listener);
//};


udp* udp::createSocket(const udp_type& type, udp::message_callback_t callback) {
  auto socket = new udp(type);
  socket->on<event::udp::message>(callback);
  return socket;
}

//Socket.prototype.bind = function(port, address, callback) {
//  var self = this;
//
//  self._healthCheck();
//
//  if (typeof callback === 'function')
//    self.once('listening', callback);
//
//  // resolve address first
//  self._handle.lookup(address, function(err, ip) {
//    if (!self._handle)
//      return; // handle has been closed in the mean time
//
//    if (err) {
//      self.emit('error', err);
//      return;
//    }
//
//    if (self._handle.bind(ip, port || 0, /*flags=*/ 0)) {
//      self.emit('error', errnoException(errno, 'bind'));
//      return;
//    }
//
//    self._handle.onmessage = onMessage;
//    self._handle.recvStart();
//    self._receiving = true;
//    self._bound = true;
//    self.fd = -42; // compatibility hack
//
//    self.emit('listening');
//  });
//};

void udp::bind(int port, const std::string& address) {
  // TODO: resolve address
  if (udp_.bind(address, port)) {
    // TODO: handle bind error
  }
  udp_.recv_start();
  receiving_ = true;
  bound_ = true;
  emit<event::listening>();
}

void udp::bind(){
  // TODO: pick random port
  bind(1234);
}

//// thin wrapper around `send`, here for compatibility with dgram_legacy.js
//Socket.prototype.sendto = function(buffer,
//                                   offset,
//                                   length,
//                                   port,
//                                   address,
//                                   callback) {
//  if (typeof offset !== 'number' || typeof length !== 'number')
//    throw new Error('send takes offset and length as args 2 and 3');
//
//  if (typeof address !== 'string')
//    throw new Error(this.type + ' sockets must send to port, address');
//
//  this.send(buffer, offset, length, port, address, callback);
//};
//
//
//Socket.prototype.send = function(buffer,
//                                 offset,
//                                 length,
//                                 port,
//                                 address,
//                                 callback) {
//  var self = this;
//
//  if (offset >= buffer.length)
//    throw new Error('Offset into buffer too large');
//
//  if (offset + length > buffer.length)
//    throw new Error('Offset + length beyond buffer length');
//
//  callback = callback || noop;
//
//  self._healthCheck();
//
//  if (!self._bound) {
//    self.bind(0, null);
//    self.once('listening', function() {
//      self.send(buffer, offset, length, port, address, callback);
//    });
//    return;
//  }
//
//  self._handle.lookup(address, function(err, ip) {
//    if (err) {
//      if (callback) callback(err);
//      self.emit('error', err);
//    }
//    else if (self._handle) {
//      var req = self._handle.send(buffer, offset, length, port, ip);
//      if (req) {
//        req.oncomplete = afterSend;
//        req.cb = callback;
//      }
//      else {
//        // don't emit as error, dgram_legacy.js compatibility
//        var err = errnoException(errno, 'send');
//        process.nextTick(function() {
//          callback(err);
//        });
//      }
//    }
//  });
//};
//
//
//function afterSend(status, handle, req, buffer) {
//  var self = handle.owner;
//
//  if (req.cb)
//    req.cb(null, buffer.length); // compatibility with dgram_legacy.js
//}

detail::resval udp::send(const detail::Buffer& buf, size_t offset, size_t length
    , unsigned short int port, const std::string& address
    , detail::udp::on_complete_t callback) {
  if (offset >= buf.size()) {
    throw new Exception("Offset into buffer too large");
  }
  if (offset+length > buf.size()) {
    throw new Exception("Offset+length into buffer too large");
  }
  // TODO: handle address lookup
  detail::resval r = udp_.send(buf, offset, length, port, address, callback);
  if (!r) {
    emit<event::error>(Exception(r));
  }
  return r;
}

//Socket.prototype.close = function() {
//  this._healthCheck();
//  this._stopReceiving();
//  this._handle.close();
//  this._handle = null;
//  this.emit('close');
//};

void udp::close() {
  stopReceiving_();
  udp_.close();
  emit<event::close>();
}

//Socket.prototype.address = function() {
//  this._healthCheck();
//
//  var address = this._handle.getsockname();
//  if (!address)
//    throw errnoException(errno, 'getsockname');
//
//  return address;
//};

std::shared_ptr<detail::net_addr> udp::address() {
  return udp_.get_sock_name();
}

//Socket.prototype.setTTL = function(arg) {
//  if (typeof arg !== 'number') {
//    throw new TypeError('Argument must be a number');
//  }
//
//  if (this._handle.setTTL(arg)) {
//    throw errnoException(errno, 'setTTL');
//  }
//
//  return arg;
//};
//
//Socket.prototype.setBroadcast = function(arg) {
//  if (this._handle.setBroadcast((arg) ? 1 : 0)) {
//    throw errnoException(errno, 'setBroadcast');
//  }
//};
//
//Socket.prototype.setMulticastTTL = function(arg) {
//  if (typeof arg !== 'number') {
//    throw new TypeError('Argument must be a number');
//  }
//
//  if (this._handle.setMulticastTTL(arg)) {
//    throw errnoException(errno, 'setMulticastTTL');
//  }
//
//  return arg;
//};
//
//
//Socket.prototype.setMulticastLoopback = function(arg) {
//  arg = arg ? 1 : 0;
//
//  if (this._handle.setMulticastLoopback(arg)) {
//    throw errnoException(errno, 'setMulticastLoopback');
//  }
//
//  return arg; // 0.4 compatibility
//};

#define X(name)                                                                \
  void udp::name(int flag) {                                                   \
    if (detail::resval r = udp_.name(flag)) {                                  \
      throw Exception(r);                                                      \
    }                                                                          \
  }

X(set_ttl)
X(set_broadcast)
X(set_multicast_ttl)
X(set_multicast_loopback)

#undef X


//Socket.prototype.addMembership = function(multicastAddress,
//                                          interfaceAddress) {
//  this._healthCheck();
//
//  if (!multicastAddress) {
//    throw new Error('multicast address must be specified');
//  }
//
//  if (this._handle.addMembership(multicastAddress, interfaceAddress)) {
//    throw new errnoException(errno, 'addMembership');
//  }
//};
//
//
//Socket.prototype.dropMembership = function(multicastAddress,
//                                           interfaceAddress) {
//  this._healthCheck();
//
//  if (!multicastAddress) {
//    throw new Error('multicast address must be specified');
//  }
//
//  if (this._handle.dropMembership(multicastAddress, interfaceAddress)) {
//    throw new errnoException(errno, 'dropMembership');
//  }
//};
//
//
//Socket.prototype._healthCheck = function() {
//  if (!this._handle)
//    throw new Error('Not running'); // error message from dgram_legacy.js
//};
//
//
//Socket.prototype._stopReceiving = function() {
//  if (!this._receiving)
//    return;
//
//  this._handle.recvStop();
//  this._receiving = false;
//  this.fd = null; // compatibility hack
//};

void udp::stopReceiving_() {
 if (!receiving_) return;
 udp_.recv_stop();
 receiving_ = false;
}

//
//
//function onMessage(handle, slab, start, len, rinfo) {
//  var self = handle.owner;
//  if (!slab) return self.emit('error', errnoException(errno, 'recvmsg'));
//  rinfo.size = len; // compatibility
//  self.emit('message', slab.slice(start, start + len), rinfo);
//}

void udp::on_message(detail::udp* self, const detail::Buffer& buf, std::shared_ptr<detail::net_addr> address) {
  CRUMB();
  if (buf.size() > 0) {
    emit<event::udp::message>(buf, address);
  }
}

//Socket.prototype.ref = function() {
//  if (this._handle)
//    this._handle.ref();
//};
//
//
//Socket.prototype.unref = function() {
//  if (this._handle)
//    this._handle.unref();
//};
//
//// TODO share with net_uv and others
//function errnoException(errorno, syscall) {
//  var e = new Error(syscall + ' ' + errorno);
//  e.errno = e.code = errorno;
//  e.syscall = syscall;
//  return e;
//}

}  // namespace native
