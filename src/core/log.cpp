#include "omvll/log.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/android_sink.h"

Logger::Logger(Logger&&) = default;
Logger& Logger::operator=(Logger&&) = default;
Logger::~Logger() = default;

Logger::Logger() {
  bool truncate = true;
  if (char* _ = getenv("OMVLL_DONT_TRUNCATE")) {
    truncate = true;
  }
  sink_ = spdlog::basic_logger_mt("omvll", "omvll.log", truncate);
  //sink_ = spdlog::stdout_color_mt("omvll");
  sink_->set_pattern("%v");
  sink_->set_level(spdlog::level::debug);
  sink_->flush_on(spdlog::level::debug);
}

Logger& Logger::instance() {
  if (instance_ == nullptr) {
    instance_ = new Logger{};
    std::atexit(destroy);
  }
  return *instance_;
}


void Logger::destroy() {
  delete instance_;
}

void Logger::disable() {
  Logger::instance().sink_->set_level(spdlog::level::off);
}

void Logger::enable() {
  Logger::instance().sink_->set_level(spdlog::level::warn);
}

void Logger::set_level(spdlog::level::level_enum lvl) {
  Logger& instance = Logger::instance();
  instance.sink_->set_level(lvl);
  instance.sink_->flush_on(lvl);
}

void Logger::set_level(LOG_LEVEL lvl) {
  switch (lvl) {
    case LOG_LEVEL::DEBUG: Logger::set_level(spdlog::level::debug); return;
    case LOG_LEVEL::TRACE: Logger::set_level(spdlog::level::trace); return;
    case LOG_LEVEL::INFO:  Logger::set_level(spdlog::level::info);  return;
    case LOG_LEVEL::WARN:  Logger::set_level(spdlog::level::warn);  return;
    case LOG_LEVEL::ERR:   Logger::set_level(spdlog::level::err);   return;
  }
}


