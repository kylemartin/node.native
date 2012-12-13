#include "base.h"
#include "handle.h"

#ifndef __NATIVE_DETAIL_SIGNAL__
#define __NATIVE_DETAIL_SIGNAL__

namespace native {
namespace detail {

class signal_watcher : public handle {
public:

  typedef std::function<void(int)> on_signal_callback_type;

  signal_watcher();

  ~signal_watcher();

  signal_watcher(const signal_watcher& s) = delete;

  resval start(int signum);

  resval stop();

  void callback(on_signal_callback_type cb);

private:
  static void on_signal(uv_signal_t *handle, int signum);

  uv_signal_t signal_;
  on_signal_callback_type callback_;
};

const char *signo_string(int signo);

}  // namespace detail
}  // namespace native

#endif // __NATIVE_DETAIL_SIGNAL__
