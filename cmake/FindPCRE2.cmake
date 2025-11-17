# Locate PCRE2 library (8-bit version)
# This module exports the following targets
#
# PCRE2::PCRE2
#
# This module defines
#  PCRE2_FOUND, if false, do not try to link to PCRE2
#  PCRE2_LIBRARIES
#  PCRE2_INCLUDE_DIR, where to find pcre2.h

# First try to find PCRE2 via its own CMake config (vcpkg, newer system packages)
# Request the 8-bit component explicitly as some configs require it
find_package(PCRE2 CONFIG QUIET COMPONENTS 8BIT)

if(PCRE2_FOUND)
  # PCRE2's config file typically creates targets like PCRE2::8BIT or PCRE2::PCRE2-8
  if(TARGET PCRE2::8BIT AND NOT TARGET PCRE2::PCRE2)
    add_library(PCRE2::PCRE2 ALIAS PCRE2::8BIT)
  elseif(TARGET PCRE2::PCRE2-8 AND NOT TARGET PCRE2::PCRE2)
    add_library(PCRE2::PCRE2 ALIAS PCRE2::PCRE2-8)
  endif()
  set(PCRE2_FOUND TRUE)
  message(STATUS "Found PCRE2 via CMake config")
  return()
endif()

# Fall back to pkg-config based discovery
find_package(PkgConfig)

pkg_search_module(PC_PCRE2 pcre2-8 libpcre2-8)

find_path(
  PCRE2_INCLUDE_DIR pcre2.h
  HINTS ${PCRE2_DIR} $ENV{PCRE2_DIR} ${PC_PCRE2_INCLUDE_DIRS}
  PATH_SUFFIXES include/pcre2 include
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw # Fink
        /opt/local # DarwinPorts
        /opt/csw # Blastwave
        /opt)

find_library(
  PCRE2_LIBRARY_RELEASE
  NAMES pcre2-8
  HINTS ${PCRE2_DIR} $ENV{PCRE2_DIR} ${PC_PCRE2_LIBRARY_DIRS}
        ${PC_PCRE2_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

find_library(
  PCRE2_LIBRARY_DEBUG
  NAMES pcre2-8d
  HINTS ${PCRE2_DIR} $ENV{PCRE2_DIR} ${PC_PCRE2_LIBRARY_DIRS}
        ${PC_PCRE2_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(PCRE2_LIBRARY_RELEASE)
  set(PCRE2_LIBRARY ${PCRE2_LIBRARY_RELEASE})
elseif(PCRE2_LIBRARY_DEBUG)
  set(PCRE2_LIBRARY ${PCRE2_LIBRARY_DEBUG})
endif()

if(PC_PCRE2_pcre2-8_FOUND)
  set(PCRE2_VERSION ${PC_PCRE2_pcre2-8_VERSION})
elseif(PC_PCRE2_libpcre2-8_FOUND)
  set(PCRE2_VERSION ${PC_PCRE2_libpcre2-8_VERSION})
else()
  set(PCRE2_VERSION ${PC_PCRE2_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PCRE2_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(PCRE2 REQUIRED_VARS PCRE2_LIBRARY
                                  PCRE2_INCLUDE_DIR VERSION_VAR PCRE2_VERSION)

mark_as_advanced(PCRE2_INCLUDE_DIR PCRE2_LIBRARY PCRE2_LIBRARY_RELEASE
                 PCRE2_LIBRARY_DEBUG)

get_filename_component(PCRE2_FILENAME ${PCRE2_LIBRARY} NAME)
string(FIND ${PCRE2_FILENAME} .a PCRE2_STATIC)

if(PCRE2_FOUND AND NOT TARGET PCRE2::PCRE2)
  if(PCRE2_STATIC EQUAL -1)
    add_library(PCRE2::PCRE2 SHARED IMPORTED)
    set_target_properties(
      PCRE2::PCRE2 PROPERTIES IMPORTED_LOCATION "${PCRE2_LIBRARY}"
                            INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIR}")
  else()
    add_library(PCRE2::PCRE2 STATIC IMPORTED)
    set_target_properties(
      PCRE2::PCRE2
      PROPERTIES INTERFACE_COMPILE_DEFINITIONS PCRE2_STATIC IMPORTED_LOCATION
                                                           "${PCRE2_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${PCRE2_INCLUDE_DIR}")
  endif()
endif()
