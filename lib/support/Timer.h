//
// Created by sebastian on 21.08.23.
//

#ifndef CAPI_TIMER_H
#define CAPI_TIMER_H

#include <iostream>
#include <chrono>

namespace capi {

class Timer {
  using clock = std::chrono::steady_clock;
  std::string msg;
  std::ostream& out;
  clock::time_point start;
  clock::duration totalDiff;
  bool running;
public:
  Timer(const std::string& msg, std::ostream& out, bool autoStart = true) : msg(msg), out(out), start(clock::now()), totalDiff(0), running(false) {
    if (autoStart)
      restart();
  }
  ~Timer() {
    pause();
    auto secs = std::chrono::duration_cast<std::chrono::milliseconds>(totalDiff).count() / 1000.0;
    out << msg << secs << " seconds\n";
  }

  void resume() {
    if (running)
      return;
    start = clock::now();
    running = true;
  }

  void pause() {
    if (!running)
      return;
    auto stop = clock::now();
    auto diff = stop - start;
    totalDiff += diff;
    running = false;
  }

  void restart() {
    running = false;
    totalDiff = clock::duration{0};
    resume();
  }
};

}

#endif // CAPI_TIMER_H
