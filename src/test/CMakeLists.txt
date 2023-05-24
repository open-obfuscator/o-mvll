if(NOT LLVM_TOOLS_DIR)
  set(LLVM_TOOLS_DIR ${LLVM_BINARY_DIR})
endif()
if(NOT LLVM_EXTERNAL_LIT OR NOT EXISTS ${LLVM_EXTERNAL_LIT})
  if(EXISTS ${LLVM_TOOLS_DIR}/bin/llvm-lit)
    set(LLVM_EXTERNAL_LIT ${LLVM_TOOLS_DIR}/bin/llvm-lit)
    message(STATUS "Found llvm-lit: ${LLVM_EXTERNAL_LIT}")
  endif()
endif()

list(APPEND CMAKE_MODULE_PATH ${LLVM_DIR})
include(AddLLVM)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in"
               "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py" @ONLY)

set(LLVM_LIT_ARGS "-vv --no-progress-bar --show-xfail --show-unsupported")

add_lit_testsuite(check "Running O-MVLL regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS OMVLL ${LLVM_TEST_DEPENDS}
)
