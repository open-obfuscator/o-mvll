target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/StringEncoding.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Routines.cpp
)

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/StringEncoding.cpp
                            PROPERTIES COMPILE_FLAGS -fno-rtti)
