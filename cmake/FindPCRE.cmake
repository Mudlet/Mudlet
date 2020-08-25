# Locate PCRE library
# This module exports the following targets
#
# PCRE::PCRE
#
# This module defines
#  PCRE_FOUND, if false, do not try to link to PCRE
#  PCRE_LIBRARIES
#  PCRE_INCLUDE_DIR, where to find pcre.h

find_package(PkgConfig)

pkg_search_module(PC_PCRE pcre libpcre)

find_path(
  PCRE_INCLUDE_DIR pcre.h
  HINTS ${PCRE_DIR} $ENV{PCRE_DIR} ${PC_PCRE_INCLUDE_DIRS}
  PATH_SUFFIXES include/pcre include
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw # Fink
        /opt/local # DarwinPorts
        /opt/csw # Blastwave
        /opt)

find_library(
  PCRE_LIBRARY_RELEASE
  NAMES pcre
  HINTS ${PCRE_DIR} $ENV{PCRE_DIR} ${PC_PCRE_LIBRARY_DIRS}
        ${PC_PCRE_LIBRARY_DIR}
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
  PCRE_LIBRARY_DEBUG
  NAMES pcred
  HINTS ${PCRE_DIR} $ENV{PCRE_DIR} ${PC_PCRE_LIBRARY_DIRS}
        ${PC_PCRE_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(PCRE_LIBRARY_DEBUG AND PCRE_LIBRARY_RELEASE)
  set(PCRE_LIBRARY optimized ${PCRE_LIBRARY_RELEASE} debug
                   ${PCRE_LIBRARY_DEBUG})
elseif(PCRE_LIBRARY_RELEASE)
  set(PCRE_LIBRARY ${PCRE_LIBRARY_RELEASE})
elseif(PCRE_LIBRARY_DEBUG)
  set(PCRE_LIBRARY ${PCRE_LIBRARY_DEBUG})
endif()

if(PC_PCRE_pcre_FOUND)
  set(PCRE_VERSION ${PC_PCRE_pcre_VERSION})
elseif(PC_PCRE_libpcre_FOUND)
  set(PCRE_VERSION ${PC_PCRE_libpcre_VERSION})
else()
  set(PCRE_VERSION ${PC_PCRE_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PCRE_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(PCRE REQUIRED_VARS PCRE_LIBRARY
                                  PCRE_INCLUDE_DIR VERSION_VAR PCRE_VERSION)

mark_as_advanced(PCRE_INCLUDE_DIR PCRE_LIBRARY PCRE_LIBRARY_RELEASE
                 PCRE_LIBRARY_DEBUG)

get_filename_component(PCRE_FILENAME ${PCRE_LIBRARY} NAME)
string(FIND ${PCRE_FILENAME} .a PCRE_STATIC)

if(PCRE_FOUND AND NOT TARGET PCRE::PCRE)
  if(PCRE_STATIC EQUAL -1)
    add_library(PCRE::PCRE SHARED IMPORTED)
    set_target_properties(
      PCRE::PCRE PROPERTIES IMPORTED_LOCATION "${PCRE_LIBRARY}"
                            INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
  else()
    add_library(PCRE::PCRE STATIC IMPORTED)
    set_target_properties(
      PCRE::PCRE
      PROPERTIES INTERFACE_COMPILE_DEFINITIONS PCRE_STATIC IMPORTED_LOCATION
                                                           "${PCRE_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${PCRE_INCLUDE_DIR}")
  endif()
endif()
