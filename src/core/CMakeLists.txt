target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/omvll_config.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/log.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/plugin.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/utils.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/jitter.cpp
)

add_subdirectory("python")
