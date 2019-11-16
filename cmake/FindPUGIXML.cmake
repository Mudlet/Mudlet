# Find the pugixml XML parsing library.
#
# This module exports the following targets that should be used to link against:
# PUGIXML::PUGIXML
#
# Sets the usual variables expected for find_package scripts:
#
# PUGIXML_INCLUDE_DIR - header location
# PUGIXML_LIBRARIES - library to link against
# PUGIXML_FOUND - true if pugixml was found.

FIND_PACKAGE(PkgConfig)

PKG_SEARCH_MODULE(PC_PUGIXML pugixml libpugixml)

FIND_PATH(PUGIXML_INCLUDE_DIR pugixml.hpp
  HINTS
    ${PUGIXML_DIR}
    $ENV{PUGIXML_DIR}
    ${PC_PUGIXML_INCLUDE_DIRS}
  PATHS
    ${PUGIXML_HOME}/include
    /usr/local/include
    /usr/local/include/pugixml-1.9
)

FIND_LIBRARY(PUGIXML_LIBRARY
  NAMES
    pugixml
  HINTS
    ${PUGIXML_DIR}
    $ENV{PUGIXML_DIR}
    ${PC_PUGIXML_LIBRARY_DIRS}
    ${PC_PUGIXML_LIBRARY_DIR}
  PATHS
    ${PUGIXML_HOME}/lib
    /usr/local/lib
    /usr/local/lib/pugixml-1.9
)

IF(PC_PUGIXML_pugixml_FOUND)
  SET(PUGIXML_VER ${PC_PUGIXML_pugixml_VERSION})
ELSEIF(PC_PUGIXML_libpugixml_FOUND)
  SET(PUGIXML_VER ${PC_PUGIXML_libpugixml_VERSION})
ELSE()
  SET(PUGIXML_VER ${PC_PUGIXML_VERSION})
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
# Support the REQUIRED and QUIET arguments, and set PUGIXML_FOUND if found.
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PugiXML
  REQUIRED_VARS
    PUGIXML_LIBRARY
    PUGIXML_INCLUDE_DIR
  VERSION_VAR
    PUGIXML_VER
)

IF(PUGIXML_FOUND AND NOT TARGET PUGIXML::PUGIXML)
  ADD_LIBRARY(PUGIXML::PUGIXML UNKNOWN IMPORTED)
  SET_TARGET_PROPERTIES(PUGIXML::PUGIXML PROPERTIES
    IMPORTED_LOCATION
      "${PUGIXML_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES
      "${PUGIXML_INCLUDE_DIR}"
  )
ENDIF()

MARK_AS_ADVANCED(PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)