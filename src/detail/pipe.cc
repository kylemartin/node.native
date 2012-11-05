
#include "native.h"

namespace native
{
    namespace detail
    {
            pipe::pipe(bool ipc)
                : stream(reinterpret_cast<uv_stream_t*>(&pipe_))
            {
                int r = uv_pipe_init(uv_default_loop(), &pipe_, ipc?1:0);
                assert(r == 0);

                pipe_.data = this;
            }

            resval pipe::bind(const std::string& name)
            {
                return run_(uv_pipe_bind, &pipe_, name.c_str());
            }

            void pipe::open(int fd)
            {
                uv_pipe_open(&pipe_, fd);
            }

            void pipe::connect(const std::string& name)
            {
                auto req = new uv_connect_t;
                assert(req);

                uv_pipe_connect(req, &pipe_, name.c_str(), [](uv_connect_t* req, int status){
                    auto self = reinterpret_cast<pipe*>(req->handle->data);
                    assert(self);
                    if(self->on_complete_) self->on_complete_(status?get_last_error():resval());
                    delete req;
                });
            }

            stream* pipe::accept_new_()
            {
                auto x = new pipe;
                assert(x);

                int r = uv_accept(reinterpret_cast<uv_stream_t*>(&pipe_), reinterpret_cast<uv_stream_t*>(&x->pipe_));
                assert(r == 0);

                return x;
            }

    }
}
