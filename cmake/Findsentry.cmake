# - Try to find Sentry
# Once done, this will define
#
#  SENTRY_FOUND - system has Sentry
#  SENTRY_INCLUDE_DIRS - the Sentry include directory
#  SENTRY_LIBRARIES - Link these to use Sentry
#
#  Imported Targets
#    sentry::sentry

if (SENTRY_INCLUDE_DIR)
  # Already in cache, be silent
  set(SENTRY_FIND_QUIETLY TRUE)
endif()

find_path(SENTRY_INCLUDE_DIR sentry.h
  HINTS ${SENTRY_DIR}
  PATH_SUFFIXES include
  PATHS /usr/local /usr /sw /opt/local /opt c:/libs
  NO_DEFAULT_PATH
)

find_library(SENTRY_LIBRARIES
  NAMES sentry
  HINTS ${SENTRY_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS /usr/local /usr /sw /opt/local /opt c:/libs
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sentry DEFAULT_MSG SENTRY_LIBRARIES SENTRY_INCLUDE_DIR)

if(SENTRY_FOUND)
  set(SENTRY_LIBRARIES ${SENTRY_LIBRARIES} CACHE STRING "Sentry libraries")
  set(SENTRY_INCLUDE_DIR ${SENTRY_INCLUDE_DIR} CACHE STRING "Sentry include directory")
endif()

mark_as_advanced(SENTRY_LIBRARIES SENTRY_INCLUDE_DIR)
