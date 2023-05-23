target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Metadata.cpp
)

add_subdirectory("objcleaner")
add_subdirectory("strings-encoding")
add_subdirectory("arithmetic")
add_subdirectory("flattening")
add_subdirectory("opaque-field-access")
add_subdirectory("opaque-constants")
add_subdirectory("break-cfg")
add_subdirectory("anti-hook")
add_subdirectory("cleaning")
