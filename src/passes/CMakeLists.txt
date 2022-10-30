target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/Metadata.cpp
)

include("${CMAKE_CURRENT_SOURCE_DIR}/passes/objcleaner/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/strings-encoding/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/arithmetic/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/flattening/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/opaque-field-access/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/opaque-constants/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/break-cfg/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/anti-hook/CMakeLists.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/passes/cleaning/CMakeLists.txt")
