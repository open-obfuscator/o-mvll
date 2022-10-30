
target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/StringEncoding.cpp
)

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/StringEncoding.cpp
                            PROPERTIES COMPILE_FLAGS -fno-rtti)
