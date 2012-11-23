#include "native.h"

namespace native
{
    namespace detail
    {
            void handle::close()
            {
              CRUMB();
                if(!handle_) return;

                uv_close(handle_, [](uv_handle_t* h) {
                    auto self = reinterpret_cast<handle*>(h->data);
                    assert(self && self->handle_ == nullptr);
                    delete self;
                });

                handle_ = nullptr;
                handle::ref();

                state_change();
            }

    }
}
