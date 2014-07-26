# Locate ZIP library
# This module defines
#  ZIP_FOUND, if false, do not try to link to ZIP 
#  ZIP_LIBRARIES
#  ZIP_INCLUDE_DIR, where to find zip*.h 


FIND_PATH(ZIP_INCLUDE_DIR zip.h
  HINTS
  ${ZIP_DIR} $ENV{ZIP_DIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt
)

FIND_LIBRARY(ZIP_LIBRARY_RELEASE 
  NAMES zip zip_s
  HINTS
  ${ZIP_DIR} $ENV{ZIP_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

FIND_LIBRARY(ZIP_LIBRARY_DEBUG 
  NAMES zipd zip_sd
  HINTS
  ${ZIP_DIR} $ENV{ZIP_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
)

IF(ZIP_LIBRARY_DEBUG AND ZIP_LIBRARY_RELEASE)
  SET(ZIP_LIBRARY optimized ${ZIP_LIBRARY_RELEASE} debug ${ZIP_LIBRARY_DEBUG} )
  GET_FILENAME_COMPONENT(ZIP_FILENAME ${ZIP_LIBRARY_RELEASE} NAME_WE)
ELSEIF(ZIP_LIBRARY_RELEASE)
  SET(ZIP_LIBRARY ${ZIP_LIBRARY_RELEASE} )
  GET_FILENAME_COMPONENT(ZIP_FILENAME ${ZIP_LIBRARY_RELEASE} NAME_WE)
ELSEIF(ZIP_LIBRARY_DEBUG)
  SET(ZIP_LIBRARY ${ZIP_LIBRARY_DEBUG} )
  GET_FILENAME_COMPONENT(ZIP_FILENAME ${ZIP_LIBRARY_DEBUG} NAME_WE)
ENDIF()

STRING(FIND ${ZIP_FILENAME} zip_s ZIP_STATIC)

IF(ZIP_STATIC EQUAL -1)
  ADD_DEFINITIONS(-DZIP_EXTERN=)
ENDIF()

SET( ZIP_LIBRARIES "${ZIP_LIBRARY}" CACHE STRING "ZIP Libraries")

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZIP_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZIP  DEFAULT_MSG  ZIP_LIBRARIES ZIP_INCLUDE_DIR)

MARK_AS_ADVANCED(ZIP_INCLUDE_DIR ZIP_LIBRARIES ZIP_LIBRARY ZIP_LIBRARY_RELEASE ZIP_LIBRARY_DEBUG)
