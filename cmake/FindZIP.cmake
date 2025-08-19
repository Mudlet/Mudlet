# Locate ZIP library
# This module defines
#  ZIP_FOUND, if false, do not try to link to ZIP
#  ZIP_LIBRARIES
#  ZIP_INCLUDE_DIR, where to find zip*.h

find_package(PkgConfig)

pkg_search_module(PC_ZIP zip libzip)

find_path(
  ZIP_INCLUDE_DIR zip.h
  HINTS ${ZIP_DIR} $ENV{ZIP_DIR} ${PC_ZIP_INCLUDE_DIRS}
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
  ZIP_LIBRARY_RELEASE
  NAMES zip zip_s
  HINTS ${ZIP_DIR} $ENV{ZIP_DIR} ${PC_ZIP_LIBRARY_DIRS} ${PC_ZIP_LIBRARY_DIR}
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
  ZIP_LIBRARY_DEBUG
  NAMES zipd zip_sd
  HINTS ${ZIP_DIR} $ENV{ZIP_DIR} ${PC_ZIP_LIBRARY_DIRS} ${PC_ZIP_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(ZIP_LIBRARY_DEBUG AND ZIP_LIBRARY_RELEASE)
  set(ZIP_LIBRARY optimized ${ZIP_LIBRARY_RELEASE} debug ${ZIP_LIBRARY_DEBUG})
  get_filename_component(ZIP_FILENAME ${ZIP_LIBRARY_RELEASE} NAME)
elseif(ZIP_LIBRARY_RELEASE)
  set(ZIP_LIBRARY ${ZIP_LIBRARY_RELEASE})
  get_filename_component(ZIP_FILENAME ${ZIP_LIBRARY_RELEASE} NAME)
elseif(ZIP_LIBRARY_DEBUG)
  set(ZIP_LIBRARY ${ZIP_LIBRARY_DEBUG})
  get_filename_component(ZIP_FILENAME ${ZIP_LIBRARY_DEBUG} NAME)
endif()

if(PC_ZIP_zip_FOUND)
  set(ZIP_VERSION ${PC_ZIP_zip_VERSION})
elseif(PC_ZIP_libzip_FOUND)
  set(ZIP_VERSION ${PC_ZIP_libzip_VERSION})
else()
  set(ZIP_VERSION ${PC_ZIP_VERSION})
endif()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZIP_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(ZIP REQUIRED_VARS ZIP_LIBRARY ZIP_INCLUDE_DIR
                                  VERSION_VAR ZIP_VERSION)

string(FIND ${ZIP_FILENAME} zip_s ZIP_STATIC)
if(ZIP_STATIC EQUAL -1)
  string(FIND ${ZIP_FILENAME} .a ZIP_STATIC)
endif()

mark_as_advanced(ZIP_INCLUDE_DIR ZIP_LIBRARY ZIP_LIBRARY_RELEASE
                 ZIP_LIBRARY_DEBUG)

if(ZIP_FOUND AND NOT TARGET ZIP::ZIP)
  if(ZIP_STATIC EQUAL -1)
    add_library(ZIP::ZIP SHARED IMPORTED)
    set_target_properties(
      ZIP::ZIP
      PROPERTIES INTERFACE_COMPILE_DEFINITIONS ZIP_EXTERN= IMPORTED_LOCATION
                                                           "${ZIP_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${ZIP_INCLUDE_DIR}")
  else()
    add_library(ZIP::ZIP STATIC IMPORTED)
    set_target_properties(
      ZIP::ZIP PROPERTIES IMPORTED_LOCATION "${ZIP_LIBRARY}"
                          INTERFACE_INCLUDE_DIRECTORIES "${ZIP_INCLUDE_DIR}")
  endif()
endif()
