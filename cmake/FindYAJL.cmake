# Locate YAJL library
# This module exports the following targets
#
# YAJL::YAJL
#
# This module defines
#  YAJL_FOUND, if false, do not try to link to YAJL
#  YAJL_LIBRARY
#  YAJL_INCLUDE_DIR, where to find yajl_*.h

find_package(PkgConfig)

pkg_search_module(PC_YAJL yajl libyajl)

find_path(
  YAJL_INCLUDE_DIR yajl/yajl_version.h
  HINTS ${YAJL_DIR} $ENV{YAJL_DIR} ${PC_YAJL_INCLUDE_DIRS}
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
  YAJL_LIBRARY_RELEASE
  NAMES yajl yajl_s
  HINTS ${YAJL_DIR} $ENV{YAJL_DIR} ${PC_YAJL_LIBRARY_DIRS}
        ${PC_YAJL_LIBRARY_DIR}
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
  YAJL_LIBRARY_DEBUG
  NAMES yajld yajl_sd
  HINTS ${YAJL_DIR} $ENV{YAJL_DIR} ${PC_YAJL_LIBRARY_DIRS}
        ${PC_YAJL_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(YAJL_LIBRARY_DEBUG AND YAJL_LIBRARY_RELEASE)
  set(YAJL_LIBRARY optimized ${YAJL_LIBRARY_RELEASE} debug
                   ${YAJL_LIBRARY_DEBUG})
  get_filename_component(YAJL_FILENAME ${YAJL_LIBRARY_RELEASE} NAME)
elseif(YAJL_LIBRARY_RELEASE)
  set(YAJL_LIBRARY ${YAJL_LIBRARY_RELEASE})
  get_filename_component(YAJL_FILENAME ${YAJL_LIBRARY_RELEASE} NAME)
elseif(YAJL_LIBRARY_DEBUG)
  set(YAJL_LIBRARY ${YAJL_LIBRARY_DEBUG})
  get_filename_component(YAJL_FILENAME ${YAJL_LIBRARY_DEBUG} NAME)
endif()

if(PC_YAJL_yajl_FOUND)
  set(YAJL_VERSION ${PC_YAJL_yajl_VERSION})
elseif(PC_YAJL_libyajl_FOUND)
  set(YAJL_VERSION ${PC_YAJL_libyajl_VERSION})
else()
  set(YAJL_VERSION ${PC_YAJL_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set YAJL_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(YAJL REQUIRED_VARS YAJL_LIBRARY
                                  YAJL_INCLUDE_DIR VERSION_VAR YAJL_VERSION)

mark_as_advanced(YAJL_INCLUDE_DIR YAJL_LIBRARY YAJL_LIBRARY_RELEASE
                 YAJL_LIBRARY_DEBUG)

string(FIND ${YAJL_FILENAME} yajl_s YAJL_STATIC)
if(YAJL_STATIC EQUAL -1)
  string(FIND ${YAJL_FILENAME} .a YAJL_STATIC)
endif()

if(YAJL_FOUND AND NOT TARGET YAJL::YAJL)
  if(YAJL_STATIC EQUAL -1)
    add_library(YAJL::YAJL SHARED IMPORTED)
    set_target_properties(
      YAJL::YAJL
      PROPERTIES INTERFACE_COMPILE_DEFINITIONS YAJL_SHARED IMPORTED_LOCATION
                                                           "${YAJL_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${YAJL_INCLUDE_DIR}")
  else()
    add_library(YAJL::YAJL STATIC IMPORTED)
    set_target_properties(
      YAJL::YAJL PROPERTIES IMPORTED_LOCATION "${YAJL_LIBRARY}"
                            INTERFACE_INCLUDE_DIRECTORIES "${YAJL_INCLUDE_DIR}")
  endif()
endif()
