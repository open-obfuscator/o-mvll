#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/fmt/chrono.h>

#include "omvll/config.hpp"

#ifdef OMVLL_DEBUG
#define SDEBUG(...) Logger::debug(__VA_ARGS__)
#else
#define SDEBUG(...)
#endif

#define STRACE(...) Logger::trace(__VA_ARGS__)
#define SINFO(...)  Logger::info(__VA_ARGS__)
#define SWARN(...)  Logger::warn(__VA_ARGS__)
#define SERR(...)   Logger::err(__VA_ARGS__)

enum class LOG_LEVEL {
  DEBUG,
  TRACE,
  INFO,
  WARN,
  ERR,
};

class Logger {
  public:
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  static Logger& instance();

  //! @brief Disable the logging module
  static void disable();

  //! @brief Enable the logging module
  static void enable();

  static void set_level(spdlog::level::level_enum lvl);
  static void set_level(LOG_LEVEL lvl);

  template <typename... Args>
  static void trace(const char *fmt, const Args &... args) {
    Logger::instance().sink_->trace(fmt, args...);
  }

  template <typename... Args>
  static void debug(const char *fmt, const Args &... args) {
    #ifdef OMVLL_DEBUG
      Logger::instance().sink_->debug(fmt, args...);
    #endif
  }

  template <typename... Args>
  static void info(const char *fmt, const Args &... args) {
    Logger::instance().sink_->info(fmt, args...);
  }

  template <typename... Args>
  static void err(const char *fmt, const Args &... args) {
    Logger::instance().sink_->error(fmt, args...);
  }

  template <typename... Args>
  static void warn(const char *fmt, const Args &... args) {
    Logger::instance().sink_->warn(fmt, args...);
  }

  ~Logger();
  private:
  Logger(void);
  Logger(Logger&&);
  Logger& operator=(Logger&&);

  static void destroy();
  inline static Logger* instance_ = nullptr;
  std::shared_ptr<spdlog::logger> sink_;
};

