# Locate zstd (Zstandard) library
# This module exports the following targets
#
# zstd::libzstd_shared or zstd::libzstd_static
#
# This module defines
#  zstd_FOUND, if false, do not try to link to zstd
#  ZSTD_LIBRARIES
#  ZSTD_INCLUDE_DIR, where to find zstd.h

# First try to find zstd via its own CMake config (vcpkg, newer system packages)
find_package(zstd CONFIG QUIET)

if(zstd_FOUND)
  # Ensure zstd::libzstd_shared target exists (some configs only create libzstd_static)
  if(NOT TARGET zstd::libzstd_shared AND TARGET zstd::libzstd_static)
    add_library(zstd::libzstd_shared ALIAS zstd::libzstd_static)
  endif()
  message(STATUS "Found zstd via CMake config")
  return()
endif()

# Fall back to pkg-config based discovery
find_package(PkgConfig)

pkg_search_module(PC_ZSTD libzstd)

find_path(
  ZSTD_INCLUDE_DIR zstd.h
  HINTS ${ZSTD_DIR} $ENV{ZSTD_DIR} ${PC_ZSTD_INCLUDE_DIRS}
  PATH_SUFFIXES include
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw # Fink
        /opt/local # DarwinPorts
        /opt/csw # Blastwave
        /opt)

find_library(
  ZSTD_LIBRARY_RELEASE
  NAMES zstd
  HINTS ${ZSTD_DIR} $ENV{ZSTD_DIR} ${PC_ZSTD_LIBRARY_DIRS}
        ${PC_ZSTD_LIBRARY_DIR}
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
  ZSTD_LIBRARY_DEBUG
  NAMES zstdd
  HINTS ${ZSTD_DIR} $ENV{ZSTD_DIR} ${PC_ZSTD_LIBRARY_DIRS}
        ${PC_ZSTD_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(ZSTD_LIBRARY_RELEASE)
  set(ZSTD_LIBRARY ${ZSTD_LIBRARY_RELEASE})
elseif(ZSTD_LIBRARY_DEBUG)
  set(ZSTD_LIBRARY ${ZSTD_LIBRARY_DEBUG})
endif()

if(PC_ZSTD_FOUND)
  set(ZSTD_VERSION ${PC_ZSTD_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set zstd_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(zstd REQUIRED_VARS ZSTD_LIBRARY
                                  ZSTD_INCLUDE_DIR VERSION_VAR ZSTD_VERSION)

mark_as_advanced(ZSTD_INCLUDE_DIR ZSTD_LIBRARY ZSTD_LIBRARY_RELEASE
                 ZSTD_LIBRARY_DEBUG)

get_filename_component(ZSTD_FILENAME ${ZSTD_LIBRARY} NAME)
string(FIND ${ZSTD_FILENAME} .a ZSTD_STATIC)

# Always create zstd::libzstd_shared target since that's what the code expects
if(zstd_FOUND AND NOT TARGET zstd::libzstd_shared)
  if(ZSTD_STATIC EQUAL -1)
    add_library(zstd::libzstd_shared SHARED IMPORTED)
  else()
    add_library(zstd::libzstd_shared STATIC IMPORTED)
  endif()
  set_target_properties(
    zstd::libzstd_shared PROPERTIES IMPORTED_LOCATION "${ZSTD_LIBRARY}"
                          INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}")
endif()
