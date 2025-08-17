#----------------------------------------------------------------
# Generated CMake target import file.
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libmudlet::mudlet" for configuration ""
set_property(TARGET libmudlet::mudlet APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG)
set_target_properties(libmudlet::mudlet PROPERTIES
  IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libmudlet.so.1.0.0"
  IMPORTED_SONAME_NOCONFIG "libmudlet.so.1"
  )

list(APPEND _cmake_import_check_targets libmudlet::mudlet )
list(APPEND _cmake_import_check_files_for_libmudlet::mudlet "${_IMPORT_PREFIX}/lib/libmudlet.so.1.0.0" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
