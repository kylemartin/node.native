
#include "native/detail/base.h"
#include "native/detail/stream.h"
#include "native/detail/tty.h"

namespace native {

namespace detail {

  tty::tty(uv_file file, bool readable)
  : stream(reinterpret_cast<uv_stream_t*>(&tty_))
  , tty_(), orig_termios_fd_(), orig_termios_(), raw_mode_(false)
  {

      int r = uv_tty_init(uv_default_loop(), &tty_, file, readable);
      assert(r == 0);

      tty_.data = this;
  }

  bool tty::is_raw() {
    return raw_mode_;
  }

  int tty::set_raw_mode(bool mode) {
    raw_mode_ = mode;
    // TODO: check for errors setting raw mode
    return uv_tty_set_mode(&tty_, mode);
  }

  int tty::get_window_size(int* width, int* height) {
    // TODO: handle error
    return uv_tty_get_winsize(&tty_, width, height);
  }

}  // namespace detail
}  // namespace native
