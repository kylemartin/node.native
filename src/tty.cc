
#include "native/tty.h"
#include "native/net.h"

namespace native {

namespace tty {

  ReadStream::ReadStream(uv_file file)
  : native::net::Socket(nullptr,nullptr,false)
  , tty_(file, true)
  {
    init_socket(&tty_);
  }

  bool ReadStream::isRaw() { return tty_.is_raw(); }
  void ReadStream::setRawMode(bool mode) {
    tty_.set_raw_mode(mode);
  }
  void ReadStream::destroy() { setRawMode(0); }

  WriteStream::WriteStream(uv_file file)
  : native::net::Socket(nullptr,nullptr,false)
  , tty_(file, false)
  , columns_(-1)
  , rows_(-1)
  {
    init_socket(&tty_);
    registerEvent<event::resize>();
  }
  void WriteStream::refreshSize() {
    tty_.get_window_size(&columns_,&rows_);
    emit<event::resize>();
  }
  int WriteStream::columns() const { return columns_; }
  int WriteStream::rows() const { return rows_; }

  bool isatty(uv_file file) {
    return UV_TTY == uv_guess_handle(file);
  }

  bool isatty(Stream* stream) {
    return UV_TTY == stream->type();
  }

  uv_handle_type guessHandleType(uv_file file) {
    assert(file >= 0);
    return uv_guess_handle(file);
  }

}  // namespace tty
}  // namespace native
