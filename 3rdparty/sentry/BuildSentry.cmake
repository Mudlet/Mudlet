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
  URL "https://github.com/getsentry/sentry-native/releases/download/0.7.19/sentry-native.zip"
  URL_HASH SHA256=d96b8d7c3f6930d5320fc9ed3b006da23ce8dc4b8d31b63cf98a378f9482ca53
  SOURCE_SUBDIR "."
)

# Prevent Sentry from being installed into macOS bundles and Linux packages
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "SentryExcluded")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sentry)

set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
