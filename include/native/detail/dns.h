
#ifndef __NATIVE__DETAIL__DNS_H__
#define __NATIVE__DETAIL__DNS_H__

namespace native {
namespace detail {

class dns {
public:
  typedef std::function<void(const std::vector<std::string>&)> on_complete_t;
  static int get_addr_info(const std::string& hostname, int
      , on_complete_t callback = nullptr);
};

}  // namespace detail
}  // namespace native

#endif // __NATIVE__DETAIL__DNS_H__
