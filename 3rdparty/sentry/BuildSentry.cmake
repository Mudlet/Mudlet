# Copyright (C) 2004-2024 Robert Griebl
# SPDX-License-Identifier: GPL-3.0-only
# credit to https://github.com/rgriebl/brickstore/blob/main/cmake/BuildSentry.cmake
include(FetchContent)

set(mll ${CMAKE_MESSAGE_LOG_LEVEL})
if (NOT VERBOSE_FETCH)
  set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)
endif()

FetchContent_Declare(
  sentry
  URL "https://github.com/getsentry/sentry-native/releases/download/0.7.17/sentry-native.zip"
  URL_HASH SHA256=c1341a0ac02440db65f41b968a46979ceab8de765c2407efb61a99511346e098
  SOURCE_SUBDIR "."
)

# Download and extract, but don't configure yet
FetchContent_Populate(sentry)

# Patch crashpad's filesystem.h to include <cstdint> for C++20 compatibility
set(FILESYSTEM_H "${sentry_SOURCE_DIR}/external/crashpad/util/file/filesystem.h")
if(EXISTS "${FILESYSTEM_H}")
  file(READ "${FILESYSTEM_H}" FILESYSTEM_CONTENT)
  # Check if already patched
  string(FIND "${FILESYSTEM_CONTENT}" "#include <cstdint>" ALREADY_PATCHED)
  if(ALREADY_PATCHED EQUAL -1)
    # Add #include <cstdint> after the existing includes
    string(REPLACE
      "#include \"util/file/file_io.h\""
      "#include \"util/file/file_io.h\"\n#include <cstdint>"
      FILESYSTEM_CONTENT "${FILESYSTEM_CONTENT}")
    file(WRITE "${FILESYSTEM_H}" "${FILESYSTEM_CONTENT}")
    message(STATUS "Patched crashpad filesystem.h for C++20 compatibility")
  endif()
endif()

# Prevent Sentry from being installed into macOS bundles and Linux packages
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "SentryExcluded")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# Now configure and build
add_subdirectory(${sentry_SOURCE_DIR} ${sentry_BINARY_DIR} EXCLUDE_FROM_ALL)

set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
