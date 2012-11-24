/**
 In node.js, a single Timer wrapper object is used as the head of a linked list
 of objects which have been enrolled (_idleStart, _idleNext, _idlePrev,
 _idleTimout set on the object). When the timer at the head of the list fires,
 it iterates over the objects and invokes the _onTimeout function on each one.

 Since we don't want to restrict what types of objects may be enrolled we define
 a TimeoutHandler interface which can be implemented for different types of
 objects and held by each instance.

 The timer manager maintains a map from time values to timer instances.
 To enroll a TimeoutHandler for a specific time value a timer is created if not
 already in the map and the TimeoutHandler is appended to a list in the timer.
 When the timer fires it invokes each TimeoutHandler in the list.
 */

#ifndef __TIMERS_H__
#define __TIMERS_H__

#include "base.h"
#include "detail.h"
#include "events.h"

namespace native {

class timer;
class timers;

typedef std::function<void()> timeout_callback_type;

/**
 * TimeoutHandler hides the details of dispatching a timeout event without
 * imposing type requirements on the classes receiving the event. It also
 * stores state related to dispatching the timeout to the class, namely the
 * time of enrollment and requested timeout delay. Any type of object with
 * an associated implementation of TimeoutHandler can receive timeouts.
 * There may well be a better way of doing this but it will do for now.
 */
class TimeoutHandler {
  friend class timer;
  friend class timers;
public:
  TimeoutHandler() :
      idleTimeout_(-1), idleStart_(-1) {
  }
  virtual void on_timeout() = 0;
  virtual ~TimeoutHandler() {
    std::cerr << "~TimeoutHandler()" << std::endl;
  }
private:
  int64_t idleTimeout_;
  int64_t idleStart_;
};

/**
 * A TimeoutHandler implementation for EventEmitter classes
 */
class TimeoutEmitter: public TimeoutHandler {
public:
  TimeoutEmitter(EventEmitter* emitter) :
      emitter_(emitter) {
  }

  void on_timeout() {
    emitter_->emit<event::timeout>();
  }
private:
  EventEmitter* emitter_;
};

/**
 * A TimeoutHandler implementation wrapping a callback function
 */
class TimeoutCaller: public TimeoutHandler {
public:
  TimeoutCaller(timeout_callback_type callback) :
      callback_(callback) {
  }

  void on_timeout() {
    callback_();
  }
private:
  timeout_callback_type callback_;
};

class timer: public detail::handle {
  friend class timers;
public:
  typedef std::shared_ptr<timer> ptr;
  /** A list of TimeoutHandler pointers associated with this timer */
  typedef std::list<std::shared_ptr<TimeoutHandler>> list_type;

  // TODO: make this noncopyable
  timer();
  virtual ~timer();

private:
  static void handle_timeout_(uv_timer_t* handle, int status);

public:

  void on_timeout(timeout_callback_type callback);

  detail::resval start(int64_t timeout, int64_t repeat);

  detail::resval stop();

  detail::resval again();
  void set_repeat(int64_t repeat);

  int64_t get_repeat();

private:
  uv_timer_t timer_;
  timeout_callback_type on_timeout_;
  list_type list_;
  int64_t timeout_;
};

class timers {
  friend class timer;
public:
  /** Map from milliseconds to pairs of timer and TimeoutEmitter lists */
  typedef std::unordered_map<unsigned int, timer*> map_type;
private:
  static map_type map_;

public:
  /**
   * Append previously enrolled TimeoutHandler to timer in map
   */
  static void active(std::shared_ptr<TimeoutHandler> item);

  /**
   * Set timeout on a TimoutHandler, unenroll if already enrolled
   */
  static void enroll(std::shared_ptr<TimeoutHandler> item, unsigned int msecs);

  /**
   * Remove a TimeoutHandler from associated timer, close timer and
   * remove from map if timer list becomes empty
   */
  static void unenroll(std::shared_ptr<TimeoutHandler> item);
};

std::shared_ptr<TimeoutHandler> setTimeout(const timeout_callback_type callback,
    int delay);
bool clearTimeout(std::shared_ptr<TimeoutHandler> handler);
timer* setInterval(timeout_callback_type callback, int repeat);
bool clearInterval(timer* timer);
}

#endif
