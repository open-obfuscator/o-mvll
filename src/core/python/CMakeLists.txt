target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/pylog.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pyllvm.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pyobf_opt.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PyConfig.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PyObfuscationConfig.cpp
)

set_source_files_properties(
  ${CMAKE_CURRENT_LIST_DIR}/pylog.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pyllvm.cpp
  ${CMAKE_CURRENT_LIST_DIR}/pyobf_opt.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PyConfig.cpp
  ${CMAKE_CURRENT_LIST_DIR}/PyObfuscationConfig.cpp
  PROPERTIES COMPILE_FLAGS -frtti)
