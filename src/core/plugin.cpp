//
// This file is distributed under the Apache License v2.0. See LICENSE for
// details.
//

#include <dlfcn.h>

#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Threading.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"

#include "omvll/PyConfig.hpp"
#include "omvll/jitter.hpp"
#include "omvll/log.hpp"
#include "omvll/omvll_config.hpp"
#include "omvll/passes.hpp"
#include "omvll/utils.hpp"

using namespace llvm;

static llvm::once_flag InitializePluginFlag;

using PassFactory = std::function<void(ModulePassManager &)>;

// Ordered list of all configurable passes. The position within this list
// determines the relative execution order within each phase.
static const std::vector<std::pair<std::string, PassFactory>> &getPassRegistry() {
  static const std::vector<std::pair<std::string, PassFactory>> Registry = {
      {omvll::AntiHook::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::AntiHook()); }},
      {omvll::FunctionOutline::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::FunctionOutline()); }},
      {omvll::StringEncoding::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::StringEncoding()); }},
      {omvll::OpaqueFieldAccess::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::OpaqueFieldAccess()); }},
      {omvll::BasicBlockDuplicate::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::BasicBlockDuplicate()); }},
      {omvll::ControlFlowFlattening::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::ControlFlowFlattening()); }},
      {omvll::BreakControlFlow::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::BreakControlFlow()); }},
      {omvll::OpaqueConstants::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::OpaqueConstants()); }},
      {omvll::Arithmetic::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::Arithmetic()); }},
#ifdef OMVLL_EXPERIMENTAL
      // ObjCleaner must be the last pass as function's name
      // could be changed, which can be confusing for the user.
      {omvll::ObjCleaner::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::ObjCleaner()); }},
#endif
      {omvll::IndirectCall::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::IndirectCall()); }},
      {omvll::IndirectBranch::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::IndirectBranch()); }},
      {omvll::Cleaning::name().str(),
       [](ModulePassManager &M) { M.addPass(omvll::Cleaning()); }},
  };
  return Registry;
}

static void addPassesForPhase(ModulePassManager &MPM, omvll::Phase P) {
  for (const auto &[Name, Factory] : getPassRegistry()) {
    if (omvll::hasPhase(Name, P))
      Factory(MPM);
  }
}

template <> struct yaml::MappingTraits<omvll::YamlConfig> {
  static void mapping(IO &IO, omvll::YamlConfig &Config) {
    IO.mapOptional("OMVLL_PYTHONPATH", Config.PythonPath, "");
    IO.mapOptional("OMVLL_CONFIG", Config.OMVLLConfig, "");
  }
};

static std::string expandAbsPath(StringRef Path, StringRef Base) {
  if (sys::path::is_absolute(Path)) {
    if (!sys::fs::exists(Path))
      SWARN("Absolute path from does not exist");
    return Path.str();
  }

  SmallString<256> YConfigAbs = Base;
  sys::path::append(YConfigAbs, Path);
  if (!sys::fs::exists(YConfigAbs))
    SWARN("Relative path does not exist");

  return YConfigAbs.str().str();
}

static bool loadYamlConfig(StringRef Dir, StringRef FileName) {
  SmallString<256> YConfig = Dir;
  sys::path::append(YConfig, FileName);
  if (!sys::fs::exists(YConfig))
    return false;

  SINFO("Loading omvll.yml from {}", YConfig.str());
  auto Buffer = MemoryBuffer::getFile(YConfig);
  if (!Buffer) {
    SERR("Cannot read '{}': {}", YConfig.str(), Buffer.getError().message());
    return false;
  }

  yaml::Input Input(**Buffer);
  omvll::YamlConfig Config;
  Input >> Config;

  SINFO("OMVLL_PYTHONPATH = {}", Config.PythonPath);
  Config.PythonPath = expandAbsPath(Config.PythonPath, Dir);

  SINFO("OMVLL_CONFIG = {}", Config.OMVLLConfig);
  Config.OMVLLConfig = expandAbsPath(Config.OMVLLConfig, Dir);

  omvll::PyConfig::YConfig = Config;
  return true;
}

static bool findYamlConfig(std::string Dir) {
  while (true) {
    SINFO("Looking for omvll.yml in {}", Dir);
    if (loadYamlConfig(Dir, omvll::PyConfig::YamlFile))
      return true;
    if (sys::path::has_parent_path(Dir)) {
      Dir = sys::path::parent_path(Dir);
    } else {
      return false;
    }
  }
}

void omvll::initYamlConfig() {
  SmallString<256> CurrentPath;
  if (auto Err = sys::fs::current_path(CurrentPath)) {
    SERR("Cannot determine the current path: '{}'", Err.message());
  } else {
    if (findYamlConfig(CurrentPath.str().str()))
      return;
  }

  Dl_info Info;
  if (!dladdr((void *)findYamlConfig, &Info)) {
    SERR("Cannot determine plugin file path");
  } else {
    SmallString<256> PluginDir = StringRef(Info.dli_fname);
    sys::path::remove_filename(PluginDir);
    if (findYamlConfig(PluginDir.str().str()))
      return;
  }

  SINFO("Could not find omvll.yml");
}

static void initializePluginOnce() {
  omvll::initYamlConfig();
  omvll::initPythonpath();

  auto &Instance = omvll::PyConfig::instance();
  SINFO("Found OMVLL at: {}", Instance.configPath());
}

PassPluginLibraryInfo getOMVLLPluginInfo() {
  omvll::Logger::set_level(spdlog::level::level_enum::debug);
  llvm::call_once(InitializePluginFlag, initializePluginOnce);

  return {LLVM_PLUGIN_API_VERSION, "OMVLL", "1.4.1", [](PassBuilder &PB) {
            try {
              PB.registerPipelineEarlySimplificationEPCallback(
                  [](ModulePassManager &MPM, OptimizationLevel Opt) {
                    MPM.addPass(omvll::LoggerBind());
                    addPassesForPhase(MPM, omvll::Phase::Early);
                    return true;
                  });
              PB.registerOptimizerLastEPCallback(
                  [](ModulePassManager &MPM, OptimizationLevel Opt) {
                    addPassesForPhase(MPM, omvll::Phase::Last);
                    return true;
                  });
            } catch (const std::exception &Exc) {
              omvll::fatalError(Exc.what());
            }
          }};
}

__attribute__((visibility("default")))
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOMVLLPluginInfo();
}
