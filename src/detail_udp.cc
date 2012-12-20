// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "native/detail/req.h"
#include "native/detail/udp.h"

#include <stdlib.h>


namespace native {
namespace detail {

class send_req : public req<uv_udp_send_t>
{
public:
  typedef std::function<void(int, udp*, send_req*, const Buffer&)> on_complete_t;

  on_complete_t on_complete_;
  Buffer buffer_;
};

udp::udp()
: handle(reinterpret_cast<uv_handle_t*>(&udp_)), udp_() {
  int r = uv_udp_init(uv_default_loop(), &udp_);
  assert(r == 0); // can't fail anyway
  udp_.data = reinterpret_cast<void*>(this);
}

udp::~udp() {}

resval udp::bind(const std::string& address, int port, int flags, unsigned int family) {
  resval r;

  switch (family) {
  case AF_INET:
    r = uv_udp_bind(&udp_, uv_ip4_addr(address.c_str(), port), flags);
    break;
  case AF_INET6:
    r = uv_udp_bind6(&udp_, uv_ip6_addr(address.c_str(), port), flags);
    break;
  default:
    assert(0 && "unexpected address family");
    abort();
  }

  return r;
}


resval udp::bind(const std::string& address, int port, int flags) {
  return bind(address, port, flags, AF_INET);
}

resval udp::bind6(const std::string& address, int port, int flags) {
  return bind(address, port, flags, AF_INET6);
}

#define X(name, fn)                                                           \
  resval udp::name(int flag) {                                                \
    int r = fn(&udp_, flag);                                                  \
    if (r) return get_last_error();                                           \
    return resval();                                                          \
  }

X(set_ttl, uv_udp_set_ttl)
X(set_broadcast, uv_udp_set_broadcast)
X(set_multicast_ttl, uv_udp_set_multicast_ttl)
X(set_multicast_loopback, uv_udp_set_multicast_loop)

#undef X

resval udp::set_membership(const std::string& address, const std::string& iface,
                                     uv_membership membership) {
  const char* iface_cstr = iface.c_str();
  if (iface.empty()) {
      iface_cstr = NULL;
  }

  int r = uv_udp_set_membership(&udp_, address.c_str(), iface_cstr,
                                membership);

  if (r) return get_last_error();
  return resval();
}


resval udp::add_membership(const std::string& address, const std::string& iface) {
  return set_membership(address, iface, UV_JOIN_GROUP);
}


resval udp::drop_membership(const std::string& address, const std::string& iface) {
  return set_membership(address, iface, UV_LEAVE_GROUP);
}



resval udp::send(Buffer& buffer, size_t offset, size_t length
    , const unsigned short port, const std::string& address, int family) {
  int r;

  assert(offset < buffer.size());
  assert(length <= buffer.size() - offset);

  send_req* request = new send_req();
  request->data_ = this;
  request->buffer_ = buffer;

  uv_buf_t buf = uv_buf_init(buffer.base() + offset, length);

  switch (family) {
  case AF_INET:
    r = uv_udp_send(&request->req_, &udp_, &buf, 1,
                    uv_ip4_addr(address.c_str(), port), on_send);
    break;
  case AF_INET6:
    r = uv_udp_send6(&request->req_, &udp_, &buf, 1,
                     uv_ip6_addr(address.c_str(), port), on_send);
    break;
  default:
    assert(0 && "unexpected address family");
    abort();
  }

  request->dispatched();

  if (r) {
    delete request;
    return get_last_error();
  }
  return resval();
}


resval udp::send(Buffer& buffer, size_t offset, size_t length
    , const unsigned short port, const std::string& address) {
  return send(buffer, offset, length, port, address, AF_INET);
}


resval udp::send6(Buffer& buffer, size_t offset, size_t length
    , const unsigned short port, const std::string& address) {
  return send(buffer, offset, length, port, address, AF_INET6);
}


resval udp::recv_start() {

  // UV_EALREADY means that the socket is already bound but that's okay
  int r = uv_udp_recv_start(&udp_, on_alloc, on_recv);
  if (r && get_last_error().code() != UV_EALREADY) {
    return get_last_error();
  }

  return resval();
}

resval udp::recv_stop() {
  return uv_udp_recv_stop(&udp_);
}

std::shared_ptr<net_addr> udp::get_sock_name() {
  struct sockaddr_storage addr;
  int addrlen = static_cast<int>(sizeof(addr));

  if(uv_udp_getsockname(&udp_, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) == 0)
  {
      return get_net_addr(reinterpret_cast<const sockaddr*>(&addr));
  }

  return nullptr;
}


// TODO share with StreamWrap::AfterWrite() in stream_wrap.cc
void udp::on_send(uv_udp_send_t* req, int status) {
  assert(req != NULL);

  send_req* request = reinterpret_cast<send_req*>(req->data);
  udp* socket = reinterpret_cast<udp*>(request->data_);

  request->on_complete_(status, socket, request, request->buffer_);
  delete request;
}


uv_buf_t udp::on_alloc(uv_handle_t* handle, size_t suggested_size) {
  udp* socket = static_cast<udp*>(handle->data);
  // TODO: use slab allocator to allocate space for udp messages
  char* buf = new char[suggested_size];
  return uv_buf_init(buf, suggested_size);
}

void udp::on_recv(uv_udp_t* handle,
                     ssize_t nread,
                     uv_buf_t buf,
                     struct sockaddr* addr,
                     unsigned flags) {

  udp* socket = reinterpret_cast<udp*>(handle->data);

  // TODO: return allocated space to slab allocator

  if (nread == 0) return;

  if (nread < 0) {
    socket->on_message_(socket, Buffer(), 0, 0, std::shared_ptr<net_addr>());
    return;
  }

  socket->on_message_(socket, Buffer(buf.base, buf.len), 0, nread, get_net_addr(addr));
}

}  // namespace detail
} // namespace node
