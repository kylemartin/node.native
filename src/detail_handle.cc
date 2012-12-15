#include "native/base.h"
#include "native/detail/handle.h"

namespace native {
namespace detail {

handle::handle(uv_handle_t* handle) :
    handle_(handle), unref_(false) {
  assert(handle_);
}

handle::~handle() {
}

void handle::ref() {
  if (!unref_)
    return;
  unref_ = false;
  uv_ref(handle_);
}

void handle::unref() {
  if (unref_)
    return;
  unref_ = true;
  uv_unref(handle_);
}

void handle::set_handle(uv_handle_t* h) {
  handle_ = h;
  handle_->data = this;
}

void handle::close() {
  CRUMB();
  if (!handle_)
    return;

  uv_close(handle_, [](uv_handle_t* h) {
    auto self = reinterpret_cast<handle*>(h->data);
    assert(self && self->handle_ == nullptr);
    delete self;
  });

  handle_ = nullptr;
  handle::ref();

  state_change();
}

void handle::state_change() {
}

uv_handle_t* handle::uv_handle() {
  return handle_;
}
const uv_handle_t* handle::uv_handle() const {
  return handle_;
}

uv_handle_type handle::type() const {
  assert(handle_);
  return handle_->type;
}
}
}
