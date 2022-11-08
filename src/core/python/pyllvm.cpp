#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Demangle/Demangle.h>

#include "omvll/utils.hpp"

#include "init.hpp"

namespace omvll {

py::module_& py_init_llvm_bindings(py::module_& m) {
  py::class_<llvm::StructType>(m, "Struct", "This class mirrors ``llvm::StructType``")
    .def_property_readonly("name",
        [] (const llvm::StructType& S) { return S.getName().str(); },
        R"delim(
        The name of the structure or the class.

        For instance:

        - ``struct.std::__ndk1::basic_string<char>``
        - ``struct._JNIEnv``
        - ``struct.JNINativeInterface``
        - ``class.SecretString``
        )delim");

  py::class_<llvm::Module>(m, "Module", "This class mirrors ``llvm::Module``")
    .def_property_readonly("identifier", &llvm::Module::getModuleIdentifier)

    .def_property_readonly("instruction_count",
                           &llvm::Module::getInstructionCount,
                           R"delim(
                           The number of non-debug IR instructions in the module.
                           )delim")

    .def_property_readonly("source_filename",
                           &llvm::Module::getSourceFileName,
                           R"delim(
                           The source filename of the module
                           )delim")

    .def_property_readonly("name",
                           [] (const llvm::Module& module) { return module.getName().str(); },
                           R"delim(
                           The short "name" of this module
                           )delim")

    .def_property_readonly("data_layout",
                           &llvm::Module::getDataLayoutStr,
                           R"delim(
                           Get the data layout string for the module's target platform.
                           )delim")
    .def("dump", [] (llvm::Module& self, const std::string& path) {
                   dump(self, path);
                 },
                 R"delim(
                 This function dumps the IR instructions of the current module in the
                 file provided in the second parameter.
                 )delim", "file"_a);


  py::class_<llvm::Function>(m, "Function", "This class mirrors ``llvm::Function``")
    .def_property_readonly("nb_instructions",
        &llvm::Function::getInstructionCount,
        R"delim(
        Return the number of IR instructions.
        )delim")
    .def_property_readonly("name",
        [] (const llvm::Function& func) {
          return func.getName().str();
        }, R"delim(
        The (mangled) name of the function.
        For instance:

        - ``_ZN7_JNIEnv12NewStringUTFEPKc``
        - ``main``
        )delim")
    .def_property_readonly("demangled_name",
        [] (const llvm::Function& func) {
          return llvm::demangle(func.getName().str());
        }, R"delim(
        The demangled name of the function.
        For instance:

        - ``_JNIEnv::NewStringUTF(char const*)``
        - ``main``
        )delim");

  py::class_<llvm::GlobalVariable>(m, "GlobalVariable", "This class mirrors ``llvm::GlobalVariable``")
    .def_property_readonly("name",
        [] (const llvm::GlobalVariable& GV) {
          return GV.getName().str();
        }, R"delim(
        The mangled name of the global variable.
        )delim")
    .def_property_readonly("demangled_name",
        [] (const llvm::GlobalVariable& GV) {
          return llvm::demangle(GV.getName().str());
        }, R"delim(
        The demangled name of the global variable.
        )delim");

  return m;
}

}
