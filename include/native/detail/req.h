// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef REQ_WRAP_H_
#define REQ_WRAP_H_

#include <assert.h>

#include "ngx-queue.h"

namespace native {
namespace detail {

extern ngx_queue_t req_wrap_queue;

/**
 * A handle for an entry in a global linked list used to hold dispatched tasks
 * waiting for completion, such as dns queries and outgoing udp messages.
 * Each task contains one of these objects which get appended to the tail of
 * the list upon construction and removed from the list upon completion.
 *
 * The template argument is expected to have a void* data member which gets set
 * to the address of the req once dispatched, the expectation being that the
 * template argument will be passed to a sub-system for processing the task
 * which will invoke an asynchronous completion handler which will need to get
 * a handle to the req. The req also has a void* data member which can be set
 * to associtate a higher level class with the task awaiting completion.
 */
template <typename T>
class req {
 public:
  req() : data_(nullptr), req_() {
    ngx_queue_insert_tail(&req_wrap_queue, &req_wrap_queue_);
  }

  req(const T& r) : data_(nullptr), req_(r) {
    ngx_queue_insert_tail(&req_wrap_queue, &req_wrap_queue_);
  }


  ~req() {
    ngx_queue_remove(&req_wrap_queue_);
    // Assert that someone has called Dispatched()
    assert(req_.data == this);
  }

  // Call this after the req has been dispatched.
  void dispatched() {
    req_.data = this;
  }

  ngx_queue_t req_wrap_queue_;
  void* data_;
  T req_; // *must* be last, GetActiveRequests() in node.cc depends on it
};

}  // namespace detail
}  // namespace native


#endif  // REQ_WRAP_H_
