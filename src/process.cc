#include "native/process.h"
#include "native/tty.h"

namespace native {

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
      return uv_run(uv_default_loop());
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
    Stream* stdout_ = createWritableStdioStream(1);
    if (tty::isatty(stdout_)) {
      on<signal_event<SIGWINCH>>([=](int signo){
        reinterpret_cast<tty::WriteStream*>(stdout_)->refreshSize();
      });
    }
    Stream* stderr_ = createWritableStdioStream(2);

    Stream* stdin_ = nullptr;
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
          uv_unref(reinterpret_cast<uv_handle_t*>(&idle_));
      }
  }
}  // namespace native
