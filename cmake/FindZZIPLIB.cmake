# Locate ZZIPLIB library This module exports the following targets
#
# ZZIPLIB::ZZIPLIB
#
# This module defines ZZIPLIB_FOUND, if false, do not try to link to ZZIPLIB
# ZZIPLIB_LIBRARY ZZIPLIB_INCLUDE_DIR, where to find *.h

# Break each step into a separate command so any status message is output
# straight away The include directory setup for Zip is unusual in that as well
# as e.g. /usr/include/zip.h we need the path to an interal header zipconf.g
# that it calls for using '<''>'s i.e. SYSTEM #include delimiters which are
# typically located at e.g. /usr/lib/libzip/include/zipconf.h and using pkg-
# config is the recommended way to get the details. Spotted recommendation to
# use pkg-config here https://github.com/Homebrew/homebrew/issues/13390
find_package(PkgConfig)
if(NOT (PKG_CONFIG_FOUND))
  message(
    WARNING
      "Unable to use pkg_config - will possibly fail to find/use Zip library..."
  )
endif()

if(PKG_CONFIG_FOUND)
  # Examining Homebrew (for MacOs) for libzzip:
  # https://bintray.com/homebrew/bottles/libzzip found that they use pkg-config
  # So use that to try and find what we want
  pkg_search_module(PC_ZZIPLIB zziplib libzzip zzip)
endif()

find_path(
  ZZIPLIB_INCLUDE_DIR zziplib.h
  HINTS ${ZZIPLIB_INCLUDE_DIR} $ENV{ZZIPLIB_INCLUDE_DIR}
        ${PC_ZZIPLIB_INCLUDE_DIRS}
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
  ZZIPLIB_LIBRARY
  NAMES zziplib libzzip zzip
  HINTS ${ZZIPLIB_DIR} $ENV{ZZIPLIB_DIR} ${PC_ZZIPLIB_LIBRARY_DIRS}
        ${PC_ZZIPLIB_LIBRARY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw
        /opt/local
        /opt/csw
        /opt)

if(PC_ZZIPLIB_FOUND)
  if(PC_ZZIPLIB_zziplib_FOUND)
    set(ZZIPLIB_VERSION ${PC_ZZIPLIB_zziplib_VERSION})
  elseif(PC_ZZIPLIB_libzzip_FOUND)
    set(ZZIPLIB_VERSION ${PC_ZZIPLIB_libzzip_VERSION})
  elseif(PC_ZZIPLIB_zzip_FOUND)
    set(ZZIPLIB_VERSION ${PC_ZZIPLIB_zzip_VERSION})
  else()
    set(ZZIPLIB_VERSION ${PC_ZZIPLIB_VERSION})
  endif()
endif()

find_package(ZLIB REQUIRED)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set HUNSPELL_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(
  ZZIPLIB REQUIRED_VARS ZZIPLIB_LIBRARY ZZIPLIB_INCLUDE_DIR VERSION_VAR
  ZZIPLIB_VERSION)

mark_as_advanced(ZZIPLIB_INCLUDE_DIR ZZIPLIB_LIBRARY)

get_filename_component(ZZIPLIB_FILENAME ${ZZIPLIB_LIBRARY} NAME_WE)
string(FIND ${ZZIPLIB_FILENAME} .a ZZIPLIB_STATIC)

if(ZZIPLIB_FOUND AND NOT TARGET ZZIPLIB::ZZIPLIB)
  if(ZZIPLIB_STATIC EQUAL -1)
    add_library(ZZIPLIB::ZZIPLIB SHARED IMPORTED)
    set_target_properties(
      ZZIPLIB::ZZIPLIB
      PROPERTIES IMPORTED_LOCATION "${ZZIPLIB_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${ZZIPLIB_INCLUDE_DIR}"
                 INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
  else()
    add_library(ZZIPLIB::ZZIPLIB STATIC IMPORTED)
    set_target_properties(
      ZZIPLIB::ZZIPLIB
      PROPERTIES INTERFACE_COMPILE_DEFINITIONS ZZIPLIB_STATIC
                 IMPORTED_LOCATION "${ZZIPLIB_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${ZZIPLIB_INCLUDE_DIR}"
                 INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
  endif()
endif()
