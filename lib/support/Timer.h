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
public:
  Timer(const std::string& msg, std::ostream& out) : msg(msg), out(out), start(clock::now()){
  }
  ~Timer() {
    auto stop = clock::now();
    auto diff = stop - start;
    auto secs = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() / 1000.0;
    out << msg << secs << " seconds\n";
  }
};

}

#endif // CAPI_TIMER_H
