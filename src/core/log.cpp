//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/android_sink.h"

#include "omvll/log.hpp"

#include <mutex>
#include <unistd.h>
#include <filesystem>

static std::mutex LoggerInstantiation;
static constexpr auto EnvLogName = "OMVLL_TRUNCATE_LOG";

static std::string int2hexstr(unsigned Val, unsigned Digits) {
  static constexpr char HexChars[] = "0123456789abcdef";
  std::string Str(Digits, '0');
  for (int i = Digits - 1; i >= 0; i -= 1) {
    Str[i] = HexChars[Val % 16];
    Val /= 16;
  }
  return Str;
}

Logger::Logger(Logger &&) = default;
Logger &Logger::operator=(Logger &&) = default;
Logger::~Logger() = default;

Logger::Logger() {
  bool Truncate = false;
  if (getenv(EnvLogName))
    Truncate = true;

  std::lock_guard<std::mutex> Lock(LoggerInstantiation);
  do {
    uint64_t Id = getpid();
    Id <<= 32;
    Id += rand();
    FileNameTmp = "omvll-" + int2hexstr(Id, 16) + ".log";
  } while (std::filesystem::exists(FileNameTmp));

  std::string Name = FileNameTmp.substr(0, FileNameTmp.size() - 4);
  Sink = spdlog::basic_logger_mt(Name, FileNameTmp, Truncate);
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

void Logger::bindModule(std::string Name) {
  assert(instance().FileNameFinal.empty() && "Bind module name once");
  std::filesystem::path Path = Name;
  instance().FileNameFinal = Path.filename().string() + ".omvll";
}

void Logger::destroy() {
  if (Instance->FileNameFinal.empty()) {
    delete Instance;
    return;
  }

  std::string TmpName = Instance->FileNameTmp;
  if (!std::filesystem::exists(Instance->FileNameTmp)) {
    SWARN("Temporary log file {} not found", Instance->FileNameTmp);
    return;
  }
  std::string FixName = Instance->FileNameFinal;
  if (std::filesystem::exists(FixName))
    std::filesystem::remove(FixName);

  delete Instance;
  std::filesystem::rename(TmpName, FixName);
}

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
