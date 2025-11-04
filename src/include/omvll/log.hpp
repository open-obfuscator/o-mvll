#pragma once

//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>

#include "omvll/config.hpp"

namespace omvll {

#ifdef OMVLL_DEBUG
#define SDEBUG(...) omvll::Logger::debug(__VA_ARGS__)
#else
#define SDEBUG(...)
#endif
#define STRACE(...) omvll::Logger::trace(__VA_ARGS__)
#define SINFO(...) omvll::Logger::info(__VA_ARGS__)
#define SWARN(...) omvll::Logger::warn(__VA_ARGS__)
#define SERR(...) omvll::Logger::err(__VA_ARGS__)

enum class LogLevel {
  Debug,
  Trace,
  Info,
  Warn,
  Err,
};

class Logger {
public:
  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  template <typename... Ts>
  static void trace(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->trace(Fmt, Args...);
  }

  template <typename... Ts>
  static void debug(const char *Fmt, const Ts &...Args) {
#ifdef OMVLL_DEBUG
    CurrentOrDefault()->debug(Fmt, Args...);
#endif
  }

  template <typename... Ts>
  static void info(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->info(Fmt, Args...);
  }

  template <typename... Ts>
  static void warn(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->warn(Fmt, Args...);
  }

  template <typename... Ts>
  static void err(const char *Fmt, const Ts &...Args) {
    CurrentOrDefault()->error(Fmt, Args...);
  }

  static void SetLevel(spdlog::level::level_enum L);
  static void set_level(spdlog::level::level_enum L) { SetLevel(L); }
  static void set_level(LogLevel L);

  static void BindModule(const std::string &Module, const std::string &Arch);

private:
  static std::shared_ptr<spdlog::logger> CurrentOrDefault();

private:
  Logger();
  static Logger &Instance();
  spdlog::level::level_enum Level = spdlog::level::debug;

  // Default sink used before any thread binds a module.
  std::shared_ptr<spdlog::logger> Default;

  // Per-thread bound module sink.
  static thread_local std::shared_ptr<spdlog::logger> Current;
};

} // end namespace omvll
