cmake_minimum_required(VERSION 3.15)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

# O-MVLL Versioning from Git (if available)
if(EXISTS       "${CMAKE_CURRENT_SOURCE_DIR}/../.git" AND
   IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../.git")
  find_package(Git REQUIRED)
  include(GitInfo)
else()
  set(OMVLL_VERSION_MAJOR "1")
  set(OMVLL_VERSION_MINOR "0")
  set(OMVLL_VERSION_PATCH "1")
endif()

project(OMVLL
        VERSION ${OMVLL_VERSION_MAJOR}.${OMVLL_VERSION_MINOR}.${OMVLL_VERSION_PATCH})
message(STATUS "${PROJECT_NAME}: ${PROJECT_VERSION}")

option(OMVLL_FORCE_LOG_DEBUG "Force the log debug output" OFF)
option(OMVLL_PY_STANDALONE   "Build OMVLL as a standalone Python module" OFF)

if(APPLE)
  set(LLVM_REQUIRED_VERSION 19.1.4)
else()
  set(LLVM_REQUIRED_VERSION 17.0.2)
endif()

find_package(LLVM ${LLVM_REQUIRED_VERSION} REQUIRED CONFIG NO_DEFAULT_PATH)

set(PYTHON_VERSION "3" CACHE STRING "Python version")
find_package(Python3 COMPONENTS Development Interpreter)

find_package(spdlog REQUIRED CONFIG)
find_package(pybind11 REQUIRED CONFIG NO_DEFAULT_PATH)

message(STATUS "Python version: ${Python3_VERSION}")
message(STATUS "Python lib:     ${Python3_LIBRARIES}")
message(STATUS "Python include: ${Python3_INCLUDE_DIRS}")
message(STATUS "Python interpreter: ${Python3_EXECUTABLE}")

if(NOT "${LLVM_PACKAGE_VERSION}" VERSION_EQUAL "${LLVM_REQUIRED_VERSION}")
  message(FATAL_ERROR "Found LLVM ${LLVM_PACKAGE_VERSION}, but need LLVM ${LLVM_REQUIRED_VERSION}")
endif()

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

message("LLVM STATUS:
  Definitions ${LLVM_DEFINITIONS}
  Includes    ${LLVM_INCLUDE_DIRS}
  Libraries   ${LLVM_LIBRARY_DIRS}
  Targets     ${LLVM_TARGETS_TO_BUILD}"
)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

# Build type
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE
      STRING "Build type (default RelWithDebInfo):" FORCE)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fdiagnostics-color=always")

set(OMVLL_DEBUG 0)
if(CMAKE_BUILD_TYPE MATCHES Release)
  if(OMVLL_FORCE_LOG_DEBUG)
    set(OMVLL_DEBUG 1)
  endif()
else()
  set(OMVLL_DEBUG 1)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/include/omvll/config.hpp.in"
               "${CMAKE_CURRENT_BINARY_DIR}/include/omvll/config.hpp")

add_library(OMVLL SHARED)

set_target_properties(OMVLL
  PROPERTIES
  POSITION_INDEPENDENT_CODE ON
)
target_include_directories(OMVLL
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/include
  SYSTEM
    ${Python3_INCLUDE_DIRS}
)

target_compile_options(OMVLL PUBLIC
  -fdata-sections
  -ffunction-sections
  -fvisibility-inlines-hidden -fvisibility=hidden
)

target_compile_options(OMVLL PUBLIC
  -fno-rtti
)

target_compile_definitions(OMVLL
  PRIVATE
    SPDLOG_DISABLE_DEFAULT_LOGGER
    SPDLOG_FUNCTION=
    SPDLOG_NO_EXCEPTIONS
    SPDLOG_NO_THREAD_ID
)

set(OMVLL_LINK_OPT
  #-fuse-ld=gold
)

if(NOT APPLE)
  list(APPEND OMVLL_LINK_OPT
    -Wl,-as-needed
    -Wl,--gc-sections
    -Wl,--exclude-libs,ALL
    -fuse-ld=gold
  )
endif()

if(APPLE)
  list(APPEND OMVLL_LINK_OPT
    -Wl,-flat_namespace
    -Wl,-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/exports.txt
  )
endif()

target_link_options(OMVLL PUBLIC ${OMVLL_LINK_OPT})

target_link_libraries(OMVLL
  PRIVATE
    spdlog::spdlog pybind11::headers
    ${Python3_LIBRARIES}
)

if(NOT APPLE)
  target_link_libraries(OMVLL
    PRIVATE
    -static-libgcc
  )
endif()

set(LLVM_LIBS_DEP
  demangle
  OrcJIT
  native
)

if(APPLE)
  list(APPEND
    LLVM_LIBS_DEP
    Option
    MCJIT
  )
endif()

if (OMVLL_PY_STANDALONE)
  find_package(Clang REQUIRED CONFIG NO_DEFAULT_PATH)
  include_directories(SYSTEM ${CLANG_INCLUDE_DIRS})
  set_target_properties(OMVLL PROPERTIES PREFIX "" OUTPUT_NAME "omvll" CLEAN_DIRECT_OUTPUT 1)
  list(APPEND LLVM_LIBS_DEP
    core
    support
    TransformUtils
    Passes
    Option
    MCJIT
  )
  target_link_libraries(OMVLL PRIVATE
    clang
    clangBasic
    clangFrontend
    clangInterpreter
  )
endif()

llvm_map_components_to_libnames(llvm_libs
  ${LLVM_LIBS_DEP}
)

target_link_libraries(OMVLL PRIVATE
  ${llvm_libs}
)

if(CMAKE_BUILD_TYPE MATCHES Release AND NOT OMVLL_FORCE_LOG_DEBUG)
  if(APPLE)
  add_custom_command(
    TARGET OMVLL
    COMMENT "Strip O-MVLL"
    POST_BUILD
    COMMAND ${CMAKE_STRIP} -x -S $<TARGET_FILE:OMVLL>
  )
  endif()
endif()

add_subdirectory("core")
add_subdirectory("passes")
add_subdirectory("test")
