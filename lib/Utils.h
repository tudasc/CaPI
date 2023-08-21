//
// Created by sebastian on 14.03.22.
//

#ifndef CAPI_UTILS_H
#define CAPI_UTILS_H

#include <cstddef>
#include <iostream>
#include <chrono>

namespace capi {

// https://jonasdevlieghere.com/containers-of-unique-pointers/
template<class BaseIterator>
class DereferenceIterator : public BaseIterator
{
public:
  using value_type = typename BaseIterator::value_type::element_type;
  using pointer = value_type *;
  using reference = value_type &;

  DereferenceIterator(const BaseIterator &other) : BaseIterator(other)
  {}

  reference operator*() const
  { return *(this->BaseIterator::operator*()); }

  pointer operator->() const
  { return this->BaseIterator::operator*().get(); }

  reference operator[](size_t n) const
  {
    return *(this->BaseIterator::operator[](n));
  }
};

template<typename Iterator>
DereferenceIterator<Iterator> dereference_iterator(Iterator t)
{
  return DereferenceIterator<Iterator>(t);
}

enum LogLevel { LOG_NONE, LOG_CRITICAL, LOG_STATUS, LOG_EXTRA };

inline std::ostream &logInfo() {
  return std::cout << "[Info] ";
}

inline std::ostream &logError() {
  return std::cerr << "[Error] ";
}

// The following macros required 'verbosity' to be defined somewhere.
extern LogLevel verbosity;

#define CAPI_DEFINE_VERBOSITY namespace capi {LogLevel verbosity{LOG_STATUS};}

inline bool checkVerbosity(LogLevel lvl) {
  return capi::verbosity >= lvl;
}

#define LOG_CRITICAL(x) if (checkVerbosity(capi::LOG_CRITICAL)) { logInfo() << x;}
#define LOG_STATUS(x) if (checkVerbosity(capi::LOG_STATUS)) { logInfo() << x;}
#define LOG_EXTRA(x) if (checkVerbosity(capi::LOG_EXTRA)) { logInfo() << x;}

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

#endif // CAPI_UTILS_H
