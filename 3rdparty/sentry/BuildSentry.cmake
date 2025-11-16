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

# Force C++17 for Sentry to avoid crashpad C++20 compatibility issues
# Crashpad's filesystem.h uses uint64_t without including <cstdint>
set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard for Sentry" FORCE)
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "Require C++ standard" FORCE)

# Prevent Sentry from being installed into macOS bundles and Linux packages
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "SentryExcluded")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sentry)

# Restore original C++ standard for main project
set(CMAKE_CXX_STANDARD 20 CACHE STRING "C++ standard" FORCE)

set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
