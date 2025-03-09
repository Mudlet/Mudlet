# Locate HUNSPELL library
# This module exports the following targets
#
# HUNSPELL::HUNSPELL
#
# This module defines
#  HUNSPELL_FOUND, if false, do not try to link to HUNSPELL
#  HUNSPELL_LIBRARY
#  HUNSPELL_INCLUDE_DIR, where to find hunspell/*.h

find_package(PkgConfig)

pkg_search_module(PC_HUNSPELL hunspell libhunspell)

find_path(
  HUNSPELL_INCLUDE_DIR hunspell/hunspell.h
  HINTS ${HUNSPELL_DIR} $ENV{HUNSPELL_DIR} ${PC_HUNSPELL_INCLUDE_DIRS}
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
  HUNSPELL_LIBRARY_RELEASE
  NAMES hunspell
        libhunspell
        hunspell-1.7
        hunspell-1.6
        hunspell-1.5
        hunspell-1.4
        hunspell-1.3
  HINTS ${HUNSPELL_DIR} $ENV{HUNSPELL_DIR} ${PC_HUNSPELL_LIBRARY_DIRS}
        ${PC_HUNSPELL_LIBRARY_DIR}
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
  HUNSPELL_LIBRARY_DEBUG
  NAMES hunspelld
        libhunspelld
        hunspelld-1.7
        hunspelld-1.6
        hunspelld-1.5
        hunspelld-1.4
        hunspelld-1.3
  HINTS ${HUNSPELL_DIR} $ENV{HUNSPELL_DIR} ${PC_HUNSPELL_LIBRARY_DIRS}
        ${PC_HUNSPELL_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(HUNSPELL_LIBRARY_DEBUG AND HUNSPELL_LIBRARY_RELEASE)
  set(HUNSPELL_LIBRARY optimized ${HUNSPELL_LIBRARY_RELEASE} debug
                       ${HUNSPELL_LIBRARY_DEBUG})
elseif(HUNSPELL_LIBRARY_RELEASE)
  set(HUNSPELL_LIBRARY ${HUNSPELL_LIBRARY_RELEASE})
elseif(HUNSPELL_LIBRARY_DEBUG)
  set(HUNSPELL_LIBRARY ${HUNSPELL_LIBRARY_DEBUG})
endif()

if(PC_HUNSPELL_hunspell_FOUND)
  set(HUNSPELL_VERSION ${PC_HUNSPELL_hunspell_VERSION})
elseif(PC_HUNSPELL_libhunspell_FOUND)
  set(HUNSPELL_VERSION ${PC_HUNSPELL_libhunspell_VERSION})
else()
  set(HUNSPELL_VERSION ${PC_HUNSPELL_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set HUNSPELL_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(
  HUNSPELL REQUIRED_VARS HUNSPELL_LIBRARY HUNSPELL_INCLUDE_DIR VERSION_VAR
  HUNSPELL_VERSION)

mark_as_advanced(HUNSPELL_INCLUDE_DIR HUNSPELL_LIBRARY HUNSPELL_LIBRARY_RELEASE
                 HUNSPELL_LIBRARY_DEBUG)

get_filename_component(HUNSPELL_FILENAME ${HUNSPELL_LIBRARY} NAME)
string(FIND ${HUNSPELL_FILENAME} .a HUNSPELL_STATIC)

if(HUNSPELL_FOUND AND NOT TARGET HUNSPELL::HUNSPELL)
  if(HUNSPELL_STATIC EQUAL -1)
    add_library(HUNSPELL::HUNSPELL SHARED IMPORTED)
    set_target_properties(
      HUNSPELL::HUNSPELL
      PROPERTIES IMPORTED_LOCATION "${HUNSPELL_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${HUNSPELL_INCLUDE_DIR}")
  else()
    add_library(HUNSPELL::HUNSPELL STATIC IMPORTED)
    set_target_properties(
      HUNSPELL::HUNSPELL
      PROPERTIES INTERFACE_COMPILE_DEFINITIONS HUNSPELL_STATIC
                 IMPORTED_LOCATION "${HUNSPELL_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${HUNSPELL_INCLUDE_DIR}")
  endif()
endif()
