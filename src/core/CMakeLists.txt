
target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/omvll_config.cpp
  ${CMAKE_CURRENT_LIST_DIR}/log.cpp
  ${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
  ${CMAKE_CURRENT_LIST_DIR}/utils.cpp
  ${CMAKE_CURRENT_LIST_DIR}/Jitter.cpp
)

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/plugin.cpp
                            PROPERTIES COMPILE_FLAGS -fno-rtti)

include("${CMAKE_CURRENT_LIST_DIR}/python/CMakeLists.txt")
