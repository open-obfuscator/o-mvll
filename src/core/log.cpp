//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <unistd.h>

#include "spdlog/sinks/android_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/null_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "omvll/log.hpp"
#include "omvll/omvll_config.hpp"

namespace omvll {

static std::mutex BindLog;
thread_local std::shared_ptr<spdlog::logger> Logger::Current;

static constexpr auto LogsRootDir = "omvll-logs";
static constexpr auto ModuleLogsDir = "omvll-module-logs";
static constexpr auto InitLogFileName = "omvll-init.log";
static constexpr auto DefaultLoggerKey = "omvll-default-shared";

static inline unsigned getPid() { return ::getpid(); }

static inline bool shouldTruncate() {
  static constexpr auto LogEnvVar = "OMVLL_TRUNCATE_LOG";
  return std::getenv(LogEnvVar);
}

static inline std::filesystem::path getDefaultLogsRootDir() {
  return std::filesystem::path(LogsRootDir);
}

static inline std::filesystem::path getLogsRootDir() {
  return Config.OutputFolder.empty()
             ? getDefaultLogsRootDir()
             : std::filesystem::path(Config.OutputFolder) / LogsRootDir;
}

static inline std::string initLogPath() {
  auto Dir = getLogsRootDir();
  std::filesystem::create_directories(Dir);
  return (Dir / InitLogFileName).string();
}

static bool tryOpenDefaultLog(const std::string &Path) {
  int Fd = open(Path.c_str(), O_CREAT | O_WRONLY | O_EXCL, 0600);
  if (Fd < 0)
    return false;
  close(Fd);
  return true;
}

static std::string basename(const std::string &PathStr) {
  std::filesystem::path P(PathStr);
  assert(P.has_filename());
  return P.filename().string();
}

static std::string makeKey(const std::string &Module) {
  return basename(Module) + "-" + std::to_string(getPid());
}

static std::string makeLogPath(const std::string &Module,
                               const std::string &Arch) {
  const auto &File =
      basename(Module) + "-omvll-" + std::to_string(getPid()) + ".log";
  std::filesystem::path FinalPath =
      getLogsRootDir() / std::filesystem::path(ModuleLogsDir) / Arch / File;
  std::filesystem::create_directories(FinalPath.parent_path());
  return FinalPath.string();
}

static void moveInitLogToUserProvidedFolder() {
  const auto DefaultDir = getDefaultLogsRootDir();
  const auto DefaultPath = DefaultDir / InitLogFileName;
  if (!std::filesystem::exists(DefaultPath))
    return;

  const auto NewDir = getLogsRootDir();
  std::error_code ErrorCode;
  std::filesystem::create_directories(NewDir, ErrorCode);

  auto NewPath = NewDir / InitLogFileName;
  // If NewPath already exists, the init log was already migrated,
  // thus remove the old DefaultPath.
  if (std::filesystem::exists(NewPath)) {
    std::filesystem::remove(DefaultPath, ErrorCode);
    return;
  } else {
    std::error_code MoveErrorCode;
    std::filesystem::rename(DefaultPath, NewPath, MoveErrorCode);
  }

  std::error_code RemoveErrorCode;
  if (std::filesystem::is_directory(DefaultDir) &&
      std::filesystem::is_empty(DefaultDir, RemoveErrorCode))
    std::filesystem::remove(DefaultDir, RemoveErrorCode);
}

Logger &Logger::Instance() {
  static Logger I;
  return I;
}

Logger::Logger() {
  std::shared_ptr<spdlog::logger> Logger;
  const auto &InitLogPath = initLogPath();

  if (tryOpenDefaultLog(InitLogPath)) {
    Logger = spdlog::basic_logger_mt(DefaultLoggerKey, InitLogPath,
                                     shouldTruncate());
  } else {
    // Discard writes in this process, if default log already exists.
    auto NullSink = std::make_shared<spdlog::sinks::null_sink_mt>();
    Logger = std::make_shared<spdlog::logger>(DefaultLoggerKey, NullSink);
    spdlog::register_logger(Logger);
  }

  Logger->set_pattern("%v");
  Logger->set_level(Level);
  Logger->flush_on(Level);
  Default = std::move(Logger);
}

void Logger::BindModule(const std::string &Module, const std::string &Arch) {
  // TODO: Shouldn't be necessary even in whole-module compilation mode.
  std::lock_guard<std::mutex> Lock(BindLog);

  // If OutputFolder is now set, migrate init log (once)
  if (!Config.OutputFolder.empty())
    moveInitLogToUserProvidedFolder();

  auto Key = makeKey(Module);
  auto Logger = spdlog::get(Key);
  if (!Logger)
    Logger = spdlog::basic_logger_mt(Key, makeLogPath(Module, Arch),
                                     shouldTruncate());
  assert(Logger);

  const auto &LI = Instance();
  Logger->set_pattern("%v");
  Logger->set_level(LI.Level);
  Logger->flush_on(LI.Level);
  Current = std::move(Logger);
}

std::shared_ptr<spdlog::logger> Logger::CurrentOrDefault() {
  if (Current)
    return Current;
  return Instance().Default;
}

void Logger::set_level(LogLevel L) {
  switch (L) {
  case LogLevel::Debug:
    SetLevel(spdlog::level::debug);
    return;
  case LogLevel::Trace:
    SetLevel(spdlog::level::trace);
    return;
  case LogLevel::Info:
    SetLevel(spdlog::level::info);
    return;
  case LogLevel::Warn:
    SetLevel(spdlog::level::warn);
    return;
  case LogLevel::Err:
    SetLevel(spdlog::level::err);
    return;
  }
}

void Logger::SetLevel(spdlog::level::level_enum L) {
  auto &LI = Instance();
  LI.Level = L;
  LI.Default->set_level(L);
  LI.Default->flush_on(L);

  spdlog::apply_all([L](const auto &Logger) {
    Logger->set_level(L);
    Logger->flush_on(L);
  });
}

} // end namespace omvll
