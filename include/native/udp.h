
#ifndef __NATIVE__UDP__
#define __NATIVE__UDP__

#include "native/detail/udp.h"
#include "native/detail/utility.h"
#include "native/events.h"

namespace native {
  namespace event { namespace udp {
    struct message : public util::callback_def<const detail::Buffer&, std::shared_ptr<detail::net_addr>> {};
  }}

  class udp : public EventEmitter {
  public:

    enum udp_type {
      UDP4,
      UDP6
    };

    typedef std::function<void(const detail::Buffer&, std::shared_ptr<detail::net_addr>)> message_callback_t;
    typedef std::function<void()> send_callback_t;

    virtual ~udp() {}

    static udp* createSocket(const udp_type& type, message_callback_t callback = nullptr);

    detail::resval send(const detail::Buffer& buf, size_t offset, size_t length
        , unsigned short int port, const std::string& address
        , detail::udp::on_complete_t callback = nullptr);

    bool bind(int port = 0, const std::string& address = "0.0.0.0");

    void close();

//    std::shared_ptr<detail::net_addr> address();

    std::string address();
    int port();

    void add_membership(const std::string& address, const std::string& iface);
    void drop_membership(const std::string& address, const std::string& iface);

#define X(name)                                                           \
  void name(int flag);

X(set_ttl)
X(set_broadcast)
X(set_multicast_ttl)
X(set_multicast_loopback)

#undef X

    void on_message(detail::udp* self, const detail::Buffer& buf, std::shared_ptr<detail::net_addr> address);

   private:

    void stopReceiving_();

    udp(const udp_type& type);
    udp_type type_;
    detail::udp udp_;
    bool receiving_;
    bool bound_;
  };

} // namespace native

#endif // __NATIVE__UDP__
