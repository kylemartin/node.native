
#include "native/detail/signal.h"

namespace native {
namespace detail {

  signal_watcher::signal_watcher()
  : handle(reinterpret_cast<uv_handle_t*>(&signal_)),
    signal_(), callback_()
  {
    int r = uv_signal_init(uv_default_loop(), &signal_);
    assert(r == 0);
    signal_.data = this;
  }

  signal_watcher::~signal_watcher() {
//    uv_signal_stop(&signal_);
  }

  resval signal_watcher::start(int signum) {
    resval r = uv_signal_start(&signal_, on_signal, signum);
    if (r) { return resval(uv_last_error(uv_default_loop())); }
    return r;
  }

  resval signal_watcher::stop() {
    resval r = uv_signal_stop(&signal_);
    if (r) { return resval(uv_last_error(uv_default_loop())); }
    return r;
  }

  void signal_watcher::callback(on_signal_callback_type cb) {
    callback_ = cb;
  }

  void signal_watcher::on_signal(uv_signal_t *handle, int signum) {
    auto self = reinterpret_cast<signal_watcher*>(handle->data);
    assert(&self->signal_ == handle);
    self->callback_(signum);
  }

  const char *signo_string(int signo) {
  #define SIGNO_CASE(e)  case e: return #e;
    switch (signo) {

  #ifdef SIGHUP
    SIGNO_CASE(SIGHUP);
  #endif

  #ifdef SIGINT
    SIGNO_CASE(SIGINT);
  #endif

  #ifdef SIGQUIT
    SIGNO_CASE(SIGQUIT);
  #endif

  #ifdef SIGILL
    SIGNO_CASE(SIGILL);
  #endif

  #ifdef SIGTRAP
    SIGNO_CASE(SIGTRAP);
  #endif

  #ifdef SIGABRT
    SIGNO_CASE(SIGABRT);
  #endif

  #ifdef SIGIOT
  # if SIGABRT != SIGIOT
    SIGNO_CASE(SIGIOT);
  # endif
  #endif

  #ifdef SIGBUS
    SIGNO_CASE(SIGBUS);
  #endif

  #ifdef SIGFPE
    SIGNO_CASE(SIGFPE);
  #endif

  #ifdef SIGKILL
    SIGNO_CASE(SIGKILL);
  #endif

  #ifdef SIGUSR1
    SIGNO_CASE(SIGUSR1);
  #endif

  #ifdef SIGSEGV
    SIGNO_CASE(SIGSEGV);
  #endif

  #ifdef SIGUSR2
    SIGNO_CASE(SIGUSR2);
  #endif

  #ifdef SIGPIPE
    SIGNO_CASE(SIGPIPE);
  #endif

  #ifdef SIGALRM
    SIGNO_CASE(SIGALRM);
  #endif

    SIGNO_CASE(SIGTERM);

  #ifdef SIGCHLD
    SIGNO_CASE(SIGCHLD);
  #endif

  #ifdef SIGSTKFLT
    SIGNO_CASE(SIGSTKFLT);
  #endif


  #ifdef SIGCONT
    SIGNO_CASE(SIGCONT);
  #endif

  #ifdef SIGSTOP
    SIGNO_CASE(SIGSTOP);
  #endif

  #ifdef SIGTSTP
    SIGNO_CASE(SIGTSTP);
  #endif

  #ifdef SIGBREAK
    SIGNO_CASE(SIGBREAK);
  #endif

  #ifdef SIGTTIN
    SIGNO_CASE(SIGTTIN);
  #endif

  #ifdef SIGTTOU
    SIGNO_CASE(SIGTTOU);
  #endif

  #ifdef SIGURG
    SIGNO_CASE(SIGURG);
  #endif

  #ifdef SIGXCPU
    SIGNO_CASE(SIGXCPU);
  #endif

  #ifdef SIGXFSZ
    SIGNO_CASE(SIGXFSZ);
  #endif

  #ifdef SIGVTALRM
    SIGNO_CASE(SIGVTALRM);
  #endif

  #ifdef SIGPROF
    SIGNO_CASE(SIGPROF);
  #endif

  #ifdef SIGWINCH
    SIGNO_CASE(SIGWINCH);
  #endif

  #ifdef SIGIO
    SIGNO_CASE(SIGIO);
  #endif

  #ifdef SIGPOLL
  # if SIGPOLL != SIGIO
    SIGNO_CASE(SIGPOLL);
  # endif
  #endif

  #ifdef SIGLOST
    SIGNO_CASE(SIGLOST);
  #endif

  #ifdef SIGPWR
  # if SIGPWR != SIGLOST
    SIGNO_CASE(SIGPWR);
  # endif
  #endif

  #ifdef SIGSYS
    SIGNO_CASE(SIGSYS);
  #endif

    default: return (std::string("unknown signal: ") + std::to_string(signo)).c_str();
    }
  }


}  // namespace detail
}  // namespace native
