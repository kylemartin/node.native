#ifndef __DETAIL_PIPE_H__
#define __DETAIL_PIPE_H__

#include "base.h"
#include "stream.h"

namespace native
{
    namespace detail
    {

        class pipe : public stream
        {
        public:
            pipe(bool ipc=false);

        private:
            virtual ~pipe()
            {}

        public:
            virtual resval bind(const std::string& name);

            virtual void open(int fd);

            virtual void connect(const std::string& name);

        private:
            virtual stream* accept_new_();

        private:
            uv_pipe_t pipe_;
        };

    }
}

#endif
