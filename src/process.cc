#include "native/process.h"
#include "native/tty.h"
#include "native/shell.h"

namespace native {

// http://stackoverflow.com/a/2212940
class process_ostream : public std::ostream {
private:
  class process_stringbuf : public std::stringbuf {
  private:
    bool err_;
  public:
    process_stringbuf(bool err) : err_(err) {}

    virtual int sync() {
      process& instance = process::instance();
      if (instance.shell_enabled()) {
        if (err_) {
          instance.shell()->cout << str() << std::flush;
        } else {
          instance.shell()->cerr << str() << std::flush;
        }
      } else {
        if (err_) {
          instance.stderr()->write(Buffer(str()), [](){});
        } else {
          instance.stdout()->write(Buffer(str()), [](){});
        }
      }
      str("");
      return 0;
    }
  };
  process_stringbuf sb_;
public:
  process_ostream(bool err) :
    std::ostream(&sb_), sb_(err) {}
};

  process::process()
  : start_time_()
  , check_()
  , prepare_()
  , idle_()
  , need_tick_(false)
  , tick_callbacks_()
  , stdin_()
  , stdout_()
  , stderr_()
  , cout_(new process_ostream(false))
  , cerr_(new process_ostream(true))
  , shell_()
  {
  }

  void process::nextTick(std::function<void()> callback)
  {
      process::instance().add_tick_callback(callback);
  }
  int process::run(std::function<void()> callback)
  {
      return process::instance().start(callback);
  }

  process& process::instance()
  {
      // TODO: (C++11) Is it really true that this is thread-safe and single instance is guaranteed across the modules?
      static process instance_;
      return instance_;
  }

  int process::start(std::function<void()> logic)
  {
      assert(logic);

      // initialize
      init();

      // run main logic
      logic();

      // start event loop ;D
      return uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  }

  void process::add_tick_callback(std::function<void()> callback)
  {
      tick_callbacks_.push_back(callback);
      enable_tick();
  }

  void process::init()
  {
      // Set start time
      uv_uptime(&start_time_);

      // Prepare tick callbacks
      uv_prepare_init(uv_default_loop(), &prepare_);
      prepare_.data = this;
      uv_prepare_start(&prepare_, [](uv_prepare_t* handle, int status) {
          auto self = reinterpret_cast<process*>(handle->data);
          assert(self && &self->prepare_ == handle);
          assert(status == 0);

          self->tick();
      });
      uv_unref(reinterpret_cast<uv_handle_t*>(&prepare_));

      uv_check_init(uv_default_loop(), &check_);
      check_.data = this;
      uv_check_start(&check_, [](uv_check_t* handle, int status) {
          auto self = reinterpret_cast<process*>(handle->data);
          assert(self && &self->check_ == handle);
          assert(status == 0);

          self->tick();
      });
      uv_unref(reinterpret_cast<uv_handle_t*>(&check_));

      uv_idle_init(uv_default_loop(), &idle_);
      idle_.data = this;
      uv_unref(reinterpret_cast<uv_handle_t*>(&idle_));

      // Prepare stdio streams
      prepareStdio();
      detail::dns::initialize();
  }

#define ENABLE_TTY
#ifdef ENABLE_TTY
  Stream* createWritableStdioStream(uv_file file) {
    Stream* stream = nullptr;

    switch (tty::guessHandleType(file)) {
    case UV_TTY: {
      stream = new tty::WriteStream(file);
      stream->detail()->unref();
    } break;

    case UV_FILE: {
      // TODO: support file streams
    } break;

    case UV_NAMED_PIPE: {
      // TODO: support pipe streams
    } break;

    case UV_UNKNOWN_HANDLE: {

    } break;

    default:
      assert(false);
      break;
    }

    return stream;
  }
#endif
  void process::prepareStdio() {
#ifdef ENABLE_TTY
    stdout_ = createWritableStdioStream(1);
    if (stdout_ && tty::isatty(stdout_)) {
      on<signal_event<SIGWINCH>>([=](int signo){
        reinterpret_cast<tty::WriteStream*>(stdout_)->refreshSize();
      });
    }
    stderr_ = createWritableStdioStream(2);

    stdin_ = nullptr;
    switch (tty::guessHandleType(0)) {
    case UV_TTY: {
      stdin_ = new tty::ReadStream(0);
      stdin_->detail()->unref();
    } break;

    case UV_FILE: {
      // TODO: support file streams
    } break;

    case UV_NAMED_PIPE: {
      // TODO: support pipe streams
    } break;

    case UV_UNKNOWN_HANDLE: {

    } break;

    default:
      assert(false);
      break;
    }
#endif
  }

  void process::tick()
  {
      if(!need_tick_) return;
      need_tick_ = false;

      if(uv_is_active(reinterpret_cast<uv_handle_t*>(&idle_)))
      {
          uv_idle_stop(&idle_);
          uv_unref(reinterpret_cast<uv_handle_t*>(&idle_));
      }

      auto fail_it = tick_callbacks_.end();
      for(auto it=tick_callbacks_.begin(); it!=tick_callbacks_.end(); ++it)
      {
          try { (*it)(); }
          catch(...)
          {
              // TODO: handle exception in a tick callback
              // ...

              // stop here and the remaining callbacks will be executed in the next tick.
              fail_it = ++it;
              break;
          }
      }
      tick_callbacks_.erase(tick_callbacks_.begin(), fail_it);
  }

  void process::enable_tick()
  {
      need_tick_ = true;
      if(!uv_is_active(reinterpret_cast<uv_handle_t*>(&idle_)))
      {
          uv_idle_start(&idle_, [](uv_idle_t* handle, int status) {
              auto self = reinterpret_cast<process*>(handle->data);
              assert(self);
              assert(self && &self->idle_ == handle);
              assert(status == 0);

              self->tick();
          });
          uv_ref(reinterpret_cast<uv_handle_t*>(&idle_));
      }
  }

  native::shell* process::shell() {
    process& instance_ = instance();
    if (instance_.shell_) return instance_.shell_;
    return instance_.shell_ = native::shell::create();
  };

  bool process::shell_enabled() {
    return instance().shell_ != nullptr && instance().shell_->running();
  }

}  // namespace native
