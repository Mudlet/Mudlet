# Find the pugixml XML parsing library.
#
# This module exports the following targets that should be used to link against:
# PUGIXML::PUGIXML
#
# Sets the usual variables expected for find_package scripts:
#
# PUGIXML_INCLUDE_DIR - header location PUGIXML_LIBRARIES - library to link
# against PUGIXML_FOUND - true if pugixml was found.

find_package(PkgConfig)

pkg_search_module(PC_PUGIXML pugixml libpugixml)

find_path(
  PUGIXML_INCLUDE_DIR pugixml.hpp
  HINTS ${PUGIXML_DIR} $ENV{PUGIXML_DIR} ${PC_PUGIXML_INCLUDE_DIRS}
  PATHS ${PUGIXML_HOME}/include /usr/local/include
        /usr/local/include/pugixml-1.9)

find_library(
  PUGIXML_LIBRARY
  NAMES pugixml
  HINTS ${PUGIXML_DIR} $ENV{PUGIXML_DIR} ${PC_PUGIXML_LIBRARY_DIRS}
        ${PC_PUGIXML_LIBRARY_DIR}
  PATHS ${PUGIXML_HOME}/lib /usr/local/lib /usr/local/lib/pugixml-1.9)

if(PC_PUGIXML_pugixml_FOUND)
  set(PUGIXML_VER ${PC_PUGIXML_pugixml_VERSION})
elseif(PC_PUGIXML_libpugixml_FOUND)
  set(PUGIXML_VER ${PC_PUGIXML_libpugixml_VERSION})
else()
  set(PUGIXML_VER ${PC_PUGIXML_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# Support the REQUIRED and QUIET arguments, and set PUGIXML_FOUND if found.
find_package_handle_standard_args(PUGIXML REQUIRED_VARS PUGIXML_LIBRARY
                                  PUGIXML_INCLUDE_DIR VERSION_VAR PUGIXML_VER)

if(PUGIXML_FOUND AND NOT TARGET PUGIXML::PUGIXML)
  add_library(PUGIXML::PUGIXML UNKNOWN IMPORTED)
  set_target_properties(
    PUGIXML::PUGIXML
    PROPERTIES IMPORTED_LOCATION "${PUGIXML_LIBRARY}"
               INTERFACE_INCLUDE_DIRECTORIES "${PUGIXML_INCLUDE_DIR}")
endif()

mark_as_advanced(PUGIXML_LIBRARY PUGIXML_INCLUDE_DIR)
