#include "App.hpp"
#include "framework/internal/Framework.hpp"
#include <csignal>

namespace app {

void App::run(int argc, char **argv) {
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);
  sigaddset(&mask, SIGTERM);
  sigprocmask(SIG_BLOCK, &mask, nullptr);

  framework::Framework::instance().init(PROJECT_DIR_PATH);

  sigwaitinfo(&mask, nullptr);

  framework::Framework::instance().stop();
}
} // namespace app
