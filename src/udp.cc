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
  socket->on<event::udp::message>(callback);
  return socket;
}

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

void udp::close() {
  stopReceiving_();
  udp_.close();
  emit<event::close>();
}

std::shared_ptr<detail::net_addr> udp::address() {
  return udp_.get_sock_name();
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
