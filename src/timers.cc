#include <chrono>

#include "native/timers.h"

// TODO: is this the best way to get current time?
static int64_t get_time_milliseconds() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}

namespace native {

// Timeout values > TIMEOUT_MAX are set to 1.
static int TIMEOUT_MAX = 2147483647; // 2^31-1

timer::timer() : handle(reinterpret_cast<uv_handle_t*>(&timer_))
    , on_timeout_(), list_(), timeout_(-1) {
  int r = uv_timer_init(uv_default_loop(), &timer_);
  assert(r == 0);
  timer_.data = this;
}

timers::map_type timers::map_;

void timer::handle_timeout_(uv_timer_t* handle, int status) {
  timer* self = static_cast<timer*>(handle->data);
  assert(self != nullptr);
  if (self->list_.empty()) { // No enrolled TimeoutHandler
    if (self->on_timeout_) {
      self->on_timeout_(); // Invoke callback
    }
  } else { // Invoke all enrolled TimeoutHandler

    // Get current time
    int64_t now = get_time_milliseconds();

//    DBG("number of timeouts: " << self->list_.size());
    while (!self->list_.empty()) {
      std::shared_ptr<TimeoutHandler> first = self->list_.front();
      unsigned int diff = now - first->idleStart_;
      unsigned int msecs = first->idleTimeout_; // XXX: maybe store timeout in timer
      if (diff + 1 < msecs) {
        // called too early, start timer again
        self->start(msecs - diff, 0);
        return;
      }
      self->list_.pop_front();
      first->on_timeout();
    }

    assert(self->list_.empty()); // List must now be empty
    self->close(); // Close this timer
    timers::map_.erase(self->timeout_); // Erase entry in map
  }
}

void timer::on_timeout(timeout_callback_type callback) {
  on_timeout_ = callback;
}

detail::resval timer::start(int64_t timeout, int64_t repeat) {
//    DBG("timer::start(" << timeout << ", " << repeat << ")");
  timeout_ = timeout;
  return detail::run_(uv_timer_start, &timer_, handle_timeout_, timeout, repeat);
}

detail::resval timer::stop() {
  return detail::run_(uv_timer_stop, &timer_);
}

detail::resval timer::again() {
  return detail::run_(uv_timer_again, &timer_);
}

void timer::set_repeat(int64_t repeat) {
  uv_timer_set_repeat(&timer_, repeat);
}

int64_t timer::get_repeat() {
  int64_t repeat = uv_timer_get_repeat(&timer_);

  if (repeat < 0) {
    //TODO: handle error
  }

  return repeat;
}

void timers::active(std::shared_ptr<TimeoutHandler> item) {
  assert(item != nullptr);

  // If TimeoutHandler was not previously enrolled then skip
  if (item->idleTimeout_ < 0) return;

  // Get timeout from handler
  unsigned int msecs = item->idleTimeout_;

  timer* t;

  if (!map_.count(msecs)) {// no timer in map for msecs
    // insert new timer into map and start it
    t = new timer();
    map_.emplace(msecs, t).first->second->start(msecs, 0);
    // NOTE: the timer must be closed before removing from map, otherwise it will not be deleted
  } else {
    // Find timeout in map
    t = map_[msecs];
  }

  // Set start time for TimeoutHandler
  item->idleStart_ = get_time_milliseconds();
//  DBG("start: " << item->idleStart_);
  // Append TimeoutHandler to timer list
  t->list_.push_back(item);
//  DBG("timers::activate(" << item->idleTimeout_ << "," << item->idleStart_ << ")");
}

void timers::enroll(std::shared_ptr<TimeoutHandler> item, unsigned int msecs) {
  // we assume that item already enrolled if idleTimeout >= 0
  if (item->idleTimeout_ >= 0) {
    unenroll(item);
  }
  assert(item->idleTimeout_ < 0);
  item->idleTimeout_ = msecs;
}

void timers::unenroll(std::shared_ptr<TimeoutHandler> item) {
  if (item->idleTimeout_ < 0) return; // item must already be enrolled
  auto it = map_.find(item->idleTimeout_);
  if (it != map_.end()) { // found entry for timeout
    timer* t = it->second;
    t->list_.remove(item); // remove from timer list
    if (t->list_.empty()) { // timer list now empty
      t->close(); // close timer
      map_.erase(it); // remove it from map
    }
  }
  // ensure handle won't be insterted again if active called later
  item->idleTimeout_ = -1;
}

std::shared_ptr<TimeoutHandler> setTimeout(const timeout_callback_type callback, int delay) {
  if (!(delay >= 1 && delay <= TIMEOUT_MAX)) {
    delay = 1;
  }

  std::shared_ptr<TimeoutHandler> handler(new TimeoutCaller(callback));

  timers::enroll(handler, delay);
  timers::active(handler);

  return handler;
}

std::shared_ptr<TimeoutHandler> setTimeout(EventEmitter* emitter, int delay) {
  if (!(delay >= 1 && delay <= TIMEOUT_MAX)) {
    delay = 1;
  }

  std::shared_ptr<TimeoutHandler> handler(new TimeoutEmitter(emitter));

  timers::enroll(handler, delay);
  timers::active(handler);

  return handler;
}

bool clearTimeout(std::shared_ptr<TimeoutHandler> handler) {
  timers::unenroll(handler);
  return true;
}

timer* setInterval(timeout_callback_type callback, int repeat) {
  timer* interval = new timer;

  if (!(repeat >= 1 && repeat <= TIMEOUT_MAX)) {
    repeat = 1; // schedule on next tick, follows browser behaviour
  }

  interval->on_timeout(callback);

  interval->start(repeat, repeat);
  return interval;
}

bool clearInterval(timer* timer) {
  // TODO: check if timer is running
  timer->close();
  return true;
}

} // namespace native
