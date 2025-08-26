target_sources(OMVLL PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Metadata.cpp
)

add_subdirectory("objc-cleaner")
add_subdirectory("string-encoding")
add_subdirectory("arithmetic")
add_subdirectory("cfg-flattening")
add_subdirectory("opaque-field-access")
add_subdirectory("opaque-constants")
add_subdirectory("break-cfg")
add_subdirectory("anti-hook")
add_subdirectory("cleaning")
add_subdirectory("indirect-branch")
add_subdirectory("indirect-call")
add_subdirectory("basic-block-duplicate")
add_subdirectory("function-outline")
