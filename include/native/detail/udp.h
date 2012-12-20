#ifndef __DETAIL_UDP_H__
#define __DETAIL_UDP_H__

#include "native/detail/buffers.h"
#include "native/detail/handle.h"

namespace native {

namespace detail {

class udp: public handle {
 public:
  uv_udp_t* uv_udp();

  resval bind(const std::string& address, int port, int flags);
  resval bind6(const std::string& address, int port, int flags);

  #define X(name)                                                           \
    resval name(int flag);

  X(set_ttl)
  X(set_broadcast)
  X(set_multicast_ttl)
  X(set_multicast_loopback)

  #undef X

  resval add_membership(const std::string& address, const std::string& iface);
  resval drop_membership(const std::string& address, const std::string& iface);

  resval send(Buffer& buffer, size_t offset, size_t length
      , const unsigned short port, const std::string& address);
  resval send6(Buffer& buffer, size_t offset, size_t length
      , const unsigned short port, const std::string& address);

  resval recv_start();
  resval recv_stop();
  std::shared_ptr<net_addr> get_sock_name();

 private:
  udp();
  virtual ~udp();


  resval bind(const std::string& address, int port, int flags, unsigned int family);
//  static Handle<Value> DoSend(const Arguments& args, int family);
  resval set_membership(const std::string& address, const std::string& iface,
                                       uv_membership membership);

  resval send(Buffer& buffer, size_t offset, size_t length
      , const unsigned short port, const std::string& address, int family);

  static uv_buf_t on_alloc(uv_handle_t* handle, size_t suggested_size);
  static void on_send(uv_udp_send_t* req, int status);
  static void on_recv(uv_udp_t* handle,
                     ssize_t nread,
                     uv_buf_t buf,
                     struct sockaddr* addr,
                     unsigned flags);

  uv_udp_t udp_;

  typedef std::function<void(udp*, const Buffer&, int, ssize_t, std::shared_ptr<net_addr>)> on_message_t;
  on_message_t on_message_;
};


}  // namespace detail

} // namespace node

#endif // UDP_WRAP_H_
