#include "native.h"

namespace native
{
    int run(std::function<void()> callback)
    {
        return detail::node::instance().start(callback);
    }
}
