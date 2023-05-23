
target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/omvll_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Jitter.cpp
)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
                            PROPERTIES COMPILE_FLAGS -fno-rtti)

add_subdirectory("python")
