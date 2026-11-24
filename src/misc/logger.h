#ifndef MISC_LOGGER_H_
#define MISC_LOGGER_H_

#ifndef NDEBUG

#include <string_view>

namespace ec::log {
void info(std::string_view ns, std::string_view text);
}  // namespace ec::log

#define LOG_INFO(ns, text) ::ec::log::info((ns), (text))
#else  // NDEBUG
#define LOG_INFO(ns, text) ((void)0)
#endif  // NDEBUG

#endif
