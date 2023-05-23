target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/pylog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pyllvm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pyobf_opt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PyConfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PyObfuscationConfig.cpp
)

set_source_files_properties(
  ${CMAKE_CURRENT_SOURCE_DIR}/pylog.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pyllvm.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/pyobf_opt.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PyConfig.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PyObfuscationConfig.cpp
  TARGET_DIRECTORY OMVLL
  PROPERTIES COMPILE_FLAGS -frtti)
