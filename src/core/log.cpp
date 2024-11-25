//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/android_sink.h"

#include "omvll/log.hpp"

static constexpr auto EnvLogName = "OMVLL_TRUNCATE_LOG";
static constexpr auto LogFileName = "omvll.log";
static constexpr auto LogName = "omvll";

Logger::Logger(Logger &&) = default;
Logger &Logger::operator=(Logger &&) = default;
Logger::~Logger() = default;

Logger::Logger() {
  bool Truncate = false;
  if (getenv(EnvLogName))
    Truncate = true;

  Sink = spdlog::basic_logger_mt(LogName, LogFileName, Truncate);
  Sink->set_pattern("%v");
  Sink->set_level(spdlog::level::debug);
  Sink->flush_on(spdlog::level::debug);
}

Logger &Logger::instance() {
  if (!Instance) {
    Instance = new Logger{};
    std::atexit(destroy);
  }
  return *Instance;
}

void Logger::destroy() { delete Instance; }

void Logger::disable() {
  Logger::instance().Sink->set_level(spdlog::level::off);
}

void Logger::enable() {
  Logger::instance().Sink->set_level(spdlog::level::warn);
}

void Logger::set_level(spdlog::level::level_enum Level) {
  Logger &Instance = Logger::instance();
  Instance.Sink->set_level(Level);
  Instance.Sink->flush_on(Level);
}

void Logger::set_level(LogLevel Level) {
  switch (Level) {
  case LogLevel::Debug:
    Logger::set_level(spdlog::level::debug);
    return;
  case LogLevel::Trace:
    Logger::set_level(spdlog::level::trace);
    return;
  case LogLevel::Info:
    Logger::set_level(spdlog::level::info);
    return;
  case LogLevel::Warn:
    Logger::set_level(spdlog::level::warn);
    return;
  case LogLevel::Err:
    Logger::set_level(spdlog::level::err);
    return;
  }
}
