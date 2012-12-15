#ifndef __DETAIL_HANDLE_H__
#define __DETAIL_HANDLE_H__

#include "base.h"

namespace native {
namespace detail {

class handle {
protected:
  handle(uv_handle_t* handle);

  virtual ~handle();

public:
  virtual void ref();

  virtual void unref();

  virtual void set_handle(uv_handle_t* h);

  // Note that the handle object itself is deleted in a deferred callback of uv_close() invoked in this function.
  virtual void close();

  virtual void state_change();

  uv_handle_t* uv_handle();
  const uv_handle_t* uv_handle() const;

  uv_handle_type type() const;
private:
  uv_handle_t* handle_;
  bool unref_;
};
}
}

#endif
