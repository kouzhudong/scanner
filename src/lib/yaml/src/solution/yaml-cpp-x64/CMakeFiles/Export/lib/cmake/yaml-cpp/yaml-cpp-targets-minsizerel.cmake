#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "yaml-cpp::yaml-cpp" for configuration "MinSizeRel"
set_property(TARGET yaml-cpp::yaml-cpp APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/yaml-cpp.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/yaml-cpp.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS yaml-cpp::yaml-cpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_yaml-cpp::yaml-cpp "${_IMPORT_PREFIX}/lib/yaml-cpp.lib" "${_IMPORT_PREFIX}/bin/yaml-cpp.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
