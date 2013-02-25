/*
 * detail_base.cc
 *
 *  Created on: Feb 25, 2013
 *      Author: kmartin
 */

#include "native/detail/base.h"

namespace native {
namespace detail {

resval from_ip4_addr(sockaddr_in* src, std::string& ip, int& port)
{
  char dest[INET_ADDRSTRLEN];
  if(uv_ip4_name(src, dest, INET_ADDRSTRLEN)) return get_last_error();

  ip = dest;
  port = static_cast<int>(ntohs(src->sin_port));

  return resval();
}


resval from_ip6_addr(sockaddr_in6* src, std::string& ip, int& port)
{
  char dest[INET6_ADDRSTRLEN];
  if(uv_ip6_name(src, dest, 46)) return get_last_error();

  ip = dest;
  port = static_cast<int>(ntohs(src->sin6_port));
  return resval();
}

std::shared_ptr<net_addr> get_net_addr(sockaddr* addr) {

  assert(addr->sa_family == AF_INET || addr->sa_family == AF_INET6);

  auto na = std::shared_ptr<net_addr>(new net_addr);
  na->is_ipv4 = (addr->sa_family == AF_INET);
  if(na->is_ipv4)
  {
    if (from_ip4_addr(reinterpret_cast<sockaddr_in*>(addr), na->ip, na->port)) {
      return na;
    }
  }
  else
  {
    if (from_ip6_addr(reinterpret_cast<sockaddr_in6*>(addr), na->ip, na->port)) {
      return na;
    }
  }
  return nullptr;
}

}  // namespace detail
}  // namespace native

