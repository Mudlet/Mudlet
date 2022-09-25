#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "qt5keychain" for configuration "Debug"
set_property(TARGET qt5keychain APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(qt5keychain PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libqt5keychain.so.0.13.99"
  IMPORTED_SONAME_DEBUG "libqt5keychain.so.1"
  )

list(APPEND _IMPORT_CHECK_TARGETS qt5keychain )
list(APPEND _IMPORT_CHECK_FILES_FOR_qt5keychain "${_IMPORT_PREFIX}/lib/libqt5keychain.so.0.13.99" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
