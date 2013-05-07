/**
@class udp

# Events #
## Emitted ##
native::event::udp::message
native::event::listening
native::event::close
native::event::error
## Registered ##
 */

#include "native/process.h"
#include "native/udp.h"
#include "native/detail/buffers.h"
#include "native/error.h"

namespace native {

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

udp* udp::createSocket(const udp_type& type, udp::message_callback_t callback) {
  auto socket = new udp(type);
  if (callback)
    socket->on<event::udp::message>(callback);
  return socket;
}

detail::resval udp::bind(int port, const std::string& address) {
  // TODO: resolve address
  detail::resval r = udp_.bind(address, port);
  if (!r) {
    return r;
  }
  udp_.recv_start();
  receiving_ = true;
  bound_ = true;
  process::nextTick([=](){
    emit<event::listening>();
  });
  return r;
}

detail::resval udp::send(const detail::Buffer& buf, size_t offset, size_t length
    , unsigned short int port, const std::string& address
    , detail::udp::on_complete_t callback) {
  if (buf.size() == 0) {
    return detail::resval("ignoring empty buffer");
  }
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

void udp::close() {
  stopReceiving_();
  udp_.close();
  emit<event::close>();
}

std::string udp::address() {
  if(type_ == UDP4 || type_ == UDP6)
  {
    auto addr = udp_.get_sock_name();
    if(addr) return addr->ip;
  }

  return std::string();
}

int udp::port() {
  if(type_ == UDP4 || type_ == UDP6)
  {
    auto addr = udp_.get_sock_name();
    if(addr) return addr->port;
  }

  return 0;
}

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

void udp::add_membership(const std::string& address, const std::string& iface) {
  if (udp_.add_membership(address, iface)) {
    throw new Exception(detail::get_last_error());
  }
}

void udp::drop_membership(const std::string& address, const std::string& iface) {
  if (udp_.drop_membership(address, iface)) {
    throw new Exception(detail::get_last_error());
  }
}

void udp::stopReceiving_() {
 if (!receiving_) return;
 udp_.recv_stop();
 receiving_ = false;
}

void udp::on_message(detail::udp* self, const detail::Buffer& buf, std::shared_ptr<detail::net_addr> address) {
  CRUMB();
  if (buf.size() > 0) {
    emit<event::udp::message>(buf, address);
  }
}

}  // namespace native
