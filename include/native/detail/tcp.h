#ifndef __DETAIL_TCP_H__
#define __DETAIL_TCP_H__

#include "base.h"
#include "stream.h"

namespace native
{
    namespace detail
    {
        class tcp : public stream
        {
        public:
            tcp();

        private:
            virtual ~tcp()
            {}

        public:
            // tcp hides handle::ref() and handle::unref()
            virtual void ref() {}
            virtual void unref() {}

            virtual std::shared_ptr<net_addr> get_sock_name();

            virtual std::shared_ptr<net_addr> get_peer_name();

            virtual resval set_no_delay(bool enable);

            virtual resval set_keepalive(bool enable, unsigned int delay);

            virtual resval bind(const std::string& ip, int port);

            virtual resval bind6(const std::string& ip, int port);

            virtual resval connect(const std::string& ip, int port);

            virtual resval connect4(const std::string& ip, int port);

            virtual resval connect6(const std::string& ip, int port);

        private:
            virtual stream* accept_new_();

        private:
            uv_tcp_t tcp_;
        };

    }
}

#endif
