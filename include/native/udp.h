
#ifndef __NATIVE__UDP__
#define __NATIVE__UDP__

#include "native/detail/udp.h"
#include "native/detail/utility.h"
#include "native/events.h"

namespace native {
  namespace event { namespace udp {
    struct message : public util::callback_def<const detail::Buffer&, std::shared_ptr<detail::net_addr>> {};
  }}

  enum udp_type {
    UDP4,
    UDP6
  };

  class udp : public EventEmitter {
  public:
    typedef std::function<void(const detail::Buffer&, std::shared_ptr<detail::net_addr>)> message_callback_t;
    typedef std::function<void()> send_callback_t;
//  dgram.createSocket(type, [callback])
    static udp* createSocket(const udp_type& type, message_callback_t callback);
//    dgram.send(buf, offset, length, port, address, [callback])
    detail::resval send(const detail::Buffer& buf, size_t offset, size_t length
        , unsigned short int port, const std::string& address
        , detail::udp::on_complete_t callback = nullptr);

//    dgram.bind(port, [address])
    void bind();
    void bind(int port, const std::string& address = "127.0.0.1");
//    dgram.close()
    void close();

    //    dgram.address()
    std::shared_ptr<detail::net_addr> address();
//    dgram.addMembership(multicastAddress, [multicastInterface])
//    dgram.dropMembership(multicastAddress, [multicastInterface])

//    dgram.setBroadcast(flag)
//    dgram.setTTL(ttl)
//    dgram.setMulticastTTL(ttl)
//    dgram.setMulticastLoopback(flag)
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
    virtual ~udp() {}
    udp_type type_;
    detail::udp udp_;
    bool receiving_;
    bool bound_;
  };

} // namespace native

#endif // __NATIVE__UDP__
