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

void init_yamlconfig(SmallVector<char>& YConfig) {
  SINFO("Loading omvll.yml from {}", std::string{YConfig.begin(), YConfig.end()});
  if (auto Buffer = MemoryBuffer::getFile(YConfig)) {
    yaml::Input yin(**Buffer);
    yin >> omvll::PyConfig::yconfig;
  } else {
    std::string path(YConfig.begin(), YConfig.end());
    SERR("Can't read '{}': {}", path, Buffer.getError().message());
  }
}

void omvll::init_yamlconfig() {
  SmallVector<char> cwd;
  if (auto err = sys::fs::current_path(cwd)) {
    SERR("Can't determine the current path: '{}''", err.message());
    return;
  }

  SINFO("Looking for omvll.yml in {}", std::string{cwd.begin(), cwd.end()});
  SmallVector<char> YConfig = cwd;
  sys::path::append(YConfig, omvll::PyConfig::YAML_FILE);
  if (sys::fs::exists(YConfig)) {
    return ::init_yamlconfig(YConfig);
  }

  std::string parent(cwd.begin(), cwd.end());
  while (sys::path::has_parent_path(parent)) {
    parent = sys::path::parent_path(parent);
    SINFO("Looking for omvll.yml in {}", std::string{parent.begin(), parent.end()});
    SmallVector<char> YConfig(parent.begin(), parent.end());
    sys::path::append(YConfig, omvll::PyConfig::YAML_FILE);
    if (sys::fs::exists(YConfig)) {
      return ::init_yamlconfig(YConfig);
    }
  }
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


