#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "sz" for configuration "Release"
set_property(TARGET sz APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(sz PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE "m"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libSZ.a"
  )

list(APPEND _cmake_import_check_targets sz )
list(APPEND _cmake_import_check_files_for_sz "${_IMPORT_PREFIX}/lib/libSZ.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
