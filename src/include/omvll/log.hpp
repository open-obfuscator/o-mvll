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

#include <filesystem>

#ifdef OMVLL_DEBUG
#define SDEBUG(...) Logger::debug(__VA_ARGS__)
#else
#define SDEBUG(...)
#endif

#define STRACE(...) Logger::trace(__VA_ARGS__)
#define SINFO(...)  Logger::info(__VA_ARGS__)
#define SWARN(...)  Logger::warn(__VA_ARGS__)
#define SERR(...)   Logger::err(__VA_ARGS__)

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

  static Logger &instance();

  //! @brief Disable the logging module
  static void disable();

  //! @brief Enable the logging module
  static void enable();

  static void set_level(spdlog::level::level_enum Level);
  static void set_level(LogLevel Level);

  template <typename... Ts>
  static void trace(const char *Fmt, const Ts &...Args) {
    Logger::instance().Sink->trace(Fmt, Args...);
  }

  template <typename... Ts>
  static void debug(const char *Fmt, const Ts &...Args) {
#ifdef OMVLL_DEBUG
    Logger::instance().Sink->debug(Fmt, Args...);
#endif
  }

  template <typename... Ts>
  static void info(const char *Fmt, const Ts &...Args) {
    Logger::instance().Sink->info(Fmt, Args...);
  }

  template <typename... Ts>
  static void err(const char *Fmt, const Ts &...Args) {
    Logger::instance().Sink->error(Fmt, Args...);
  }

  template <typename... Ts>
  static void warn(const char *Fmt, const Ts &...Args) {
    Logger::instance().Sink->warn(Fmt, Args...);
  }

  static void bindModule(std::string Name);

  ~Logger();

private:
  Logger(void);
  Logger(Logger &&);
  Logger &operator=(Logger &&);

  static void destroy();
  static thread_local inline Logger *Instance = nullptr;
  std::shared_ptr<spdlog::logger> Sink;
  unsigned LoggerId;
  std::filesystem::path ModName;
};
