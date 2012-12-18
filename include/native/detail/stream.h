#ifndef __DETAIL_STREAM_H__
#define __DETAIL_STREAM_H__

#include "base.h"
#include "handle.h"

namespace native
{
    namespace detail
    {
        class tcp;

        class stream : public handle
        {
            typedef std::function<void(const char*, std::size_t, std::size_t, stream*, resval)> on_read_callback_type;
            typedef std::function<void(resval)> on_complete_callback_type;
            typedef std::function<void(stream*, resval)> on_connection_callback_type;

        protected:
            stream(uv_stream_t* stream);

            virtual ~stream() {}

        public:
            void on_read(on_read_callback_type callback);

            void on_complete(on_complete_callback_type callback);

            void on_connection(on_connection_callback_type callback);

            virtual void set_handle(uv_handle_t* h);

            std::size_t write_queue_size() const;

            virtual resval read_start();

            virtual resval read_stop();

            virtual resval write(const char* data, int offset, int length, stream* send_stream=nullptr);

            virtual resval shutdown();

            virtual resval listen(int backlog);

            bool is_readable() const;
            bool is_writable() const;

            uv_stream_t* uv_stream();
            const uv_stream_t* uv_stream() const;

        private:
            virtual stream* accept_new_();

            static uv_buf_t on_alloc(uv_handle_t* h, size_t suggested_size);

            void after_read_(uv_stream_t* handle, ssize_t nread, uv_buf_t buf, uv_handle_type pending);

        private:
            uv_stream_t* stream_;
        protected:
            on_connection_callback_type on_connection_;
            on_read_callback_type on_read_;
            on_complete_callback_type on_complete_;

        };

    }
}

#endif
