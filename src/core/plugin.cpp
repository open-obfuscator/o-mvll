#include "omvll/passes.hpp"
#include "omvll/PyConfig.hpp"
#include "omvll/log.hpp"
#include "omvll/utils.hpp"
#include "omvll/Jitter.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Driver/Driver.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInstance.h>

#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/YAMLParser.h>
#include <llvm/Support/YAMLTraits.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

using namespace llvm;

#define REGISTER_PASS(X)     \
  do {                       \
    if (pass == X::name()) { \
      SINFO("[+] {}", pass); \
      MPM.addPass(X());      \
      continue;              \
    }                        \
  } while(0)

template <>
struct yaml::MappingTraits<omvll::yaml_config_t> {
    static void mapping(IO& io, omvll::yaml_config_t& config) {
      io.mapOptional("OMVLL_PYTHONPATH", config.PYTHONPATH, "");
      io.mapOptional("OMVLL_CONFIG",     config.OMVLL_CONFIG, "");
    }
};

bool load_yamlconfig(StringRef dir, StringRef filename) {
  SmallString<256> YConfig = dir;
  sys::path::append(YConfig, filename);
  if (!sys::fs::exists(YConfig))
    return false;

  SINFO("Loading omvll.yml from {}", YConfig.str());
  auto Buffer = MemoryBuffer::getFile(YConfig);
  if (!Buffer) {
    SERR("Can't read '{}': {}", YConfig.str(), Buffer.getError().message());
    return false;
  }

  yaml::Input yin(**Buffer);
  omvll::yaml_config_t yconfig;
  yin >> omvll::PyConfig::yconfig;
  return true;
}

bool find_yamlconfig(std::string dir) {
  while (true) {
    SINFO("Looking for omvll.yml in {}", dir);
    if (load_yamlconfig(dir, omvll::PyConfig::YAML_FILE))
      return true;
    if (sys::path::has_parent_path(dir)) {
      dir = sys::path::parent_path(dir);
    } else {
      return false;
    }
  }
}

void omvll::init_yamlconfig() {
  SmallString<256> cwd;
  if (auto err = sys::fs::current_path(cwd)) {
    SERR("Can't determine the current path: '{}''", err.message());
  } else {
    if (find_yamlconfig(cwd.str().str()))
      return;
  }

  SINFO("Didn't find omvll.yml");
}


PassPluginLibraryInfo getOMVLLPluginInfo() {
  static std::atomic<bool> ONCE_FLAG = false;
  Logger::set_level(spdlog::level::level_enum::debug);

  omvll::init_yamlconfig();
  omvll::init_pythonpath();
  return {LLVM_PLUGIN_API_VERSION, "OMVLL", "0.0.1",
          [](PassBuilder &PB) {

            try {
              auto& instance = omvll::PyConfig::instance();
              SDEBUG("OMVLL Path: {}", instance.config_path());

              PB.registerPipelineEarlySimplificationEPCallback(
                [&] (ModulePassManager &MPM, OptimizationLevel opt) {
                  if (ONCE_FLAG) {
                    return true;
                  }
                  for (const std::string& pass : instance.get_passes()) {
                    REGISTER_PASS(omvll::AntiHook);
                    REGISTER_PASS(omvll::StringEncoding);

                    REGISTER_PASS(omvll::OpaqueFieldAccess);
                    REGISTER_PASS(omvll::ControlFlowFlattening);
                    REGISTER_PASS(omvll::BreakControlFlow);

                    REGISTER_PASS(omvll::OpaqueConstants);
                    REGISTER_PASS(omvll::Arithmetic);

                    /* ObjCleaner must be the last pass as function's name could be
                     * changed, which can be confusing of the user
                     */
                    REGISTER_PASS(omvll::ObjCleaner);
                    REGISTER_PASS(omvll::Cleaning);
                  }
                  ONCE_FLAG = true;
                  return true;
                }
              );
            } catch (const std::exception& e) {
              omvll::fatalError(e.what());
            }
          }};
}

__attribute__((visibility("default")))
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return getOMVLLPluginInfo();
}


