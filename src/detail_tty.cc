
#include "native/detail/base.h"
#include "native/detail/stream.h"
#include "native/detail/tty.h"

namespace native {

namespace detail {

  tty::tty(uv_file file, bool readable)
        : stream(reinterpret_cast<uv_stream_t*>(&tty_))
        ,  tty_(), orig_termios_fd_(), orig_termios_()
    {
        int r = uv_tty_init(uv_default_loop(), &tty_, file, readable);
        assert(r == 0);

        tty_.data = this;
    }


  int tty::enable_raw() {
    // TODO: handle error
//              if (uv_tty_set_mode(ttyi->get<uv_tty_t>(), enabled ? 1 : 0) != 0) return -1;
//              if (uv_tty_set_mode(ttyo->get<uv_tty_t>(), enabled ? 1 : 0) != 0) return -1;
//              return 0;
    uv_tty_t* tty = &tty_;
    return uv_tty_set_mode(tty, 1);
  }

  int tty::disable_raw() {
    uv_tty_t* tty = &tty_;
    return uv_tty_set_mode(tty, 0);
  }

  int tty::get_window_size(int* width, int* height) {
    // TODO: handle error
    return uv_tty_get_winsize(&tty_, width, height);
  }

}  // namespace detail
}  // namespace native
