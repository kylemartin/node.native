
#include "native/detail/tcp.h"

namespace native
{
namespace detail
{

  tcp::tcp()
      : stream(reinterpret_cast<uv_stream_t*>(&tcp_))
      , tcp_()
  {
      int r = uv_tcp_init(uv_default_loop(), &tcp_);
      assert(r == 0);

      tcp_.data = this;
  }

  std::shared_ptr<net_addr> tcp::get_sock_name()
  {
      struct sockaddr_storage addr;
      int addrlen = static_cast<int>(sizeof(addr));

      if(uv_tcp_getsockname(&tcp_, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) == 0)
      {
          assert(addr.ss_family == AF_INET || addr.ss_family == AF_INET6);

          auto na = std::shared_ptr<net_addr>(new net_addr);
          na->is_ipv4 = (addr.ss_family == AF_INET);
          if(na->is_ipv4)
          {
              if(from_ip4_addr(reinterpret_cast<struct sockaddr_in*>(&addr), na->ip, na->port)) return na;
          }
          else
          {
              if(from_ip6_addr(reinterpret_cast<struct sockaddr_in6*>(&addr), na->ip, na->port)) return na;
          }
      }

      return nullptr;
  }

  std::shared_ptr<net_addr> tcp::get_peer_name()
  {
      struct sockaddr_storage addr;
      int addrlen = static_cast<int>(sizeof(addr));

      if(uv_tcp_getsockname(&tcp_, reinterpret_cast<struct sockaddr*>(&addr), &addrlen) == 0)
      {
          assert(addr.ss_family == AF_INET || addr.ss_family == AF_INET6);

          auto na = std::shared_ptr<net_addr>(new net_addr);
          na->is_ipv4 = (addr.ss_family == AF_INET);
          if(na->is_ipv4)
          {
              if(from_ip4_addr(reinterpret_cast<struct sockaddr_in*>(&addr), na->ip, na->port)) return na;
          }
          else
          {
              if(from_ip6_addr(reinterpret_cast<struct sockaddr_in6*>(&addr), na->ip, na->port)) return na;
          }
      }

      return nullptr;
  }

  resval tcp::set_no_delay(bool enable)
  {
      return run_(uv_tcp_nodelay, &tcp_, enable?1:0);
  }

  resval tcp::set_keepalive(bool enable, unsigned int delay)
  {
      return run_(uv_tcp_keepalive, &tcp_, enable?1:0, delay);
  }

  resval tcp::bind(const std::string& ip, int port)
  {
      return run_(uv_tcp_bind, &tcp_, to_ip4_addr(ip, port));
  }

  resval tcp::bind6(const std::string& ip, int port)
  {
      return run_(uv_tcp_bind6, &tcp_, to_ip6_addr(ip, port));
  }

  resval tcp::connect(const std::string& ip, int port)
  {
      auto ver = get_ip_version(ip);
      if(ver == 4) return connect4(ip, port);
      else if(ver == 6) return connect6(ip, port);
      else
      {
          // TODO: uh-oh...
          assert(false);
          return resval();
      }
  }

  resval tcp::connect4(const std::string& ip, int port)
  {
      struct sockaddr_in addr = to_ip4_addr(ip, port);

      auto req = new uv_connect_t;
      assert(req);

      if(uv_tcp_connect(req, &tcp_, addr, [](uv_connect_t* req, int status){
          auto self = reinterpret_cast<tcp*>(req->handle->data);
          assert(self);
          if(self->on_complete_) self->on_complete_(status?get_last_error():resval());
          delete req;
      }))
      {
          delete req;
          return get_last_error();
      }
      return resval();
  }

  resval tcp::connect6(const std::string& ip, int port)
  {
      struct sockaddr_in6 addr = to_ip6_addr(ip, port);

      auto req = new uv_connect_t;
      assert(req);

      if(uv_tcp_connect6(req, &tcp_, addr, [](uv_connect_t* req, int status){
          auto self = reinterpret_cast<tcp*>(req->handle->data);
          assert(self);
          if(self->on_complete_) self->on_complete_(status?get_last_error():resval());
          delete req;
      }))
      {
          delete req;
          return get_last_error();
      }
      return resval();
  }

  stream* tcp::accept_new_()
  {
      auto x = new tcp;
      assert(x);

      int r = uv_accept(reinterpret_cast<uv_stream_t*>(&tcp_), reinterpret_cast<uv_stream_t*>(&x->tcp_));
      assert(r == 0);

      return x;
  }

}
}
