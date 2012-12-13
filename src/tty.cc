
#include "native/tty.h"
#include "native/net.h"

namespace native {

namespace tty {

  ReadStream::ReadStream(uv_file file)
  : tty_(file, true),
    native::net::Socket(&tty_,nullptr,false)
  {}

  bool ReadStream::isRaw() { return false; }
  void ReadStream::setRawMode(bool mode) {}

  WriteStream::WriteStream(uv_file file)
  : tty_(file, false), columns_(-1), rows_(-1),
    native::net::Socket(&tty_,nullptr,false)
  {
    registerEvent<event::resize>();
  }
  void WriteStream::refreshSize() {
    tty_.get_window_size(&columns_,&rows_);
    emit<event::resize>();
  }
  int WriteStream::columns() { return columns_; }
  int WriteStream::rows() { return rows_; }

  bool isatty(uv_file file) {
    return UV_TTY == uv_guess_handle(file);
  }

  uv_handle_type guessHandleType(uv_file file) {
    assert(file >= 0);
    return uv_guess_handle(file);
  }

}  // namespace tty
}  // namespace native
