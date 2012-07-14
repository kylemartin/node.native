
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

#include <chrono>

#include "base.h"
#include "detail.h"

// TODO: is this the best way to get current time?
static int64_t get_time_milliseconds() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
}

namespace native
{

	class timer;
	class timers;

	// Timeout values > TIMEOUT_MAX are set to 1.
	static int TIMEOUT_MAX = 2147483647; // 2^31-1

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
		TimeoutHandler() : idleTimeout_(-1) {}
		virtual void on_timeout() = 0;
		virtual ~TimeoutHandler() { std::cerr << "~TimeoutHandler()" << std::endl; }
	private:
		int64_t idleTimeout_;
		int64_t idleStart_;
	};

	/**
	 * A TimeoutHandler implementation for EventEmitter classes
	 */
	class TimeoutEmitter : public TimeoutHandler {
	public:
		TimeoutEmitter(EventEmitter* emitter) : emitter_(emitter) {}

		void on_timeout() {
			emitter_->emit<event::timeout>();
		}
	private:
		EventEmitter* emitter_;
	};

	/**
	 * A TimeoutHandler implementation wrapping a callback function
	 */
	class TimeoutCaller : public TimeoutHandler {
	public:
		TimeoutCaller(timeout_callback_type callback) : callback_(callback) {}

		void on_timeout() {
			callback_();
		}
	private:
		timeout_callback_type callback_;
	};

	class timer : public detail::handle {
		friend class timers;
	public:
		typedef std::shared_ptr<timer> ptr;
		/** A list of TimeoutHandler pointers associated with this timer */
		typedef std::list<std::shared_ptr<TimeoutHandler>> list_type;

		// TODO: make this noncopyable
		timer() : handle(reinterpret_cast<uv_handle_t*>(&timer_)) {
			int r = uv_timer_init(uv_default_loop(), &timer_);
			assert(r == 0);
			timer_.data = this;
		}
		virtual ~timer() { std::cerr << "~timer()" << std::endl; };

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

    class timers
    {
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

	void timer::handle_timeout_(uv_timer_t* handle, int status) {
		std::cerr << "timer::handle_timeout_" << std::endl;
		timer* self = static_cast<timer*>(handle->data);
		assert(self != nullptr);
		if (self->list_.empty()) { // No enrolled TimeoutHandler
			if (self->on_timeout_) {
				self->on_timeout_(); // Invoke callback
			}
		} else { // Invoke all enrolled TimeoutHandler

			// Get current time
			int64_t now = get_time_milliseconds();

			std::cerr << "number of timeouts: " << self->list_.size() << std::endl;
			while (!self->list_.empty()) {
				std::shared_ptr<TimeoutHandler> first = self->list_.front();
				unsigned int diff = now - first->idleStart_;
				unsigned int msecs = first->idleTimeout_; // XXX: maybe store timeout in timer
				if (diff + 1 < msecs) {
					// called too early, start timer again
					self->start(msecs - diff, 0);
					return;
				}
				std::shared_ptr<TimeoutHandler> handler = self->list_.front();
				self->list_.pop_front();
				handler->on_timeout();
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
		std::cerr << "timer::start(" << timeout << ", " << repeat << ")" << std::endl;
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

    timers::map_type timers::map_;

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
		std::cerr << "start: " << item->idleStart_ << std::endl;
		// Append TimeoutHandler to timer list
		t->list_.push_back(item);
		std::cerr << "timers::activate(" << item->idleTimeout_ << "," << item->idleStart_ << ")" << std::endl;
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

}

#endif
