#include "logger.h"

#ifndef NDEBUG

#include <iostream>
#include <mutex>

namespace ec::log {

namespace {
std::mutex& log_mutex() {
  static std::mutex m;
  return m;
}
}  // namespace

void info(std::string_view ns, std::string_view text) {
  std::lock_guard<std::mutex> lock(log_mutex());
  std::cerr << '[' << ns << "] " << text << '\n';
}

}  // namespace ec::log

#endif  // NDEBUG
