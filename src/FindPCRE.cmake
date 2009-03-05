# Locate PCRE library
# This module defines
#  PCRE_FOUND, if false, do not try to link to PCRE 
#  PCRE_LIBRARIES
#  PCRE_INCLUDE_DIR, where to find pcre.h 


FIND_PATH(PCRE_INCLUDE_DIR pcre.h
  HINTS
  $ENV{PCRE_DIR}
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

FIND_LIBRARY(PCRE_LIBRARY 
  NAMES pcre
  HINTS
  $ENV{PCRE_DIR}
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

FIND_LIBRARY(PCRECPP_LIBRARY 
  NAMES pcrecpp
  HINTS
  $ENV{PCRE_DIR}
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

FIND_LIBRARY(PCREPOSIX_LIBRARY 
  NAMES pcreposix
  HINTS
  $ENV{PCRE_DIR}
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

SET( PCRE_LIBRARIES "${PCRE_LIBRARY};${PCRECPP_LIBRARY};${PCREPOSIX_LIBRARY}" CACHE STRING "PCRE Libraries")

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PCRE_FOUND to TRUE if 
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PCRE  DEFAULT_MSG  PCRE_LIBRARIES PCRE_INCLUDE_DIR)

MARK_AS_ADVANCED(PCRE_INCLUDE_DIR PCRE_LIBRARIES PCRE_LIBRARY PCRECPP_LIBRARY PCREPOSIX_LIBRARY)

