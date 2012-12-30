
#ifndef __NATIVE__DETAIL__DNS_H__
#define __NATIVE__DETAIL__DNS_H__

namespace native {
namespace detail {

class dns {
public:
  typedef std::function<void(int, const std::vector<std::string>&, int)> on_complete_t;
  // TODO: move is_ip to a common namespace
  static int is_ip(const std::string& ip);
  static int get_addr_info(const std::string& hostname, int
      , on_complete_t callback = nullptr);
  static resval QueryGetHostByAddr(const std::string& name, on_complete_t oncomplete);
  static resval QueryGetHostByName(const std::string& name, on_complete_t oncomplete);
  static resval QueryGetHostByName(const std::string& name, int family, on_complete_t oncomplete);

  // Called on process start to setup ares
  static void initialize();
};

}  // namespace detail
}  // namespace native

#endif // __NATIVE__DETAIL__DNS_H__
