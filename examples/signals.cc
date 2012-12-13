
#include "native.h"
using namespace native;

void handle_signal(int signo) {
  std::cout << "Received signal: " << detail::signo_string(signo) << std::endl;
}

int main(int argc, char** argv) {
    return process::run([=]() {

      process::instance().onSignal<SIGTSTP>(handle_signal);

      timer* interval1 = setInterval([=](){
        std::cout << "interval1!" << std::endl;
      }, 1000);
    });
}
