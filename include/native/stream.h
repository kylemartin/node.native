#ifndef __STREAM_H__
#define __STREAM_H__

#include "base.h"
#include "detail.h"
#include "error.h"
#include "events.h"

namespace native
{
	using detail::Buffer;

    class Stream : public EventEmitter
    {
    public:
        Stream(detail::stream* stream, bool readable=true, bool writable=true);

        virtual ~Stream();

    public:
        bool readable() const;
        bool writable() const;

        // TODO: implement Stream::setEncoding() function.
        virtual void setEncoding(const std::string& encoding);

        virtual void pause();
        virtual void resume();

        virtual Stream* pipe(Stream* destination, const util::dict& options);

        virtual Stream* pipe(Stream* destination);

        virtual bool write(const Buffer& buffer, std::function<void()> callback=nullptr) = 0;
        virtual bool write(const std::string& str, const std::string& encoding, int fd) = 0;

        virtual bool end(const Buffer& buffer) = 0;
        virtual bool end(const std::string& str, const std::string& encoding, int fd) = 0;
        virtual bool end() = 0;

        virtual void destroy(Exception exception) = 0;
        virtual void destroy() = 0;
        virtual void destroySoon() = 0;

    protected:
        void writable(bool b);
        void readable(bool b);

    private:
        struct pipe_context
        {
            pipe_context(Stream* source, Stream* destination, const util::dict& options);
            void on_error(Exception exception);
            void on_close();
            void on_end();
            void cleanup();
            Stream* source_;
            Stream* destination_;

            void* source_on_data_;
            void* dest_on_drain_;
            void* source_on_end_;
            void* source_on_close_;
            void* source_on_error_;
            void* dest_on_error_;
            void* source_on_end_clenaup_;
            void* source_on_close_clenaup_;
            void* dest_on_end_clenaup_;
            void* dest_on_close_clenaup_;

            bool did_on_end_;
            std::size_t pipe_count_;
        };

    private:
        detail::stream* stream_;
        bool readable_;
        bool writable_;

        bool is_stdio_;
        std::shared_ptr<pipe_context> pipe_;
    };
}

#endif

