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

# Prevent Sentry from being installed into macOS bundles and Linux packages
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "SentryExcluded")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sentry)

set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
