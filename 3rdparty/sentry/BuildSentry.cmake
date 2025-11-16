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
  URL "https://github.com/getsentry/sentry-native/releases/download/0.7.15/sentry-native.zip"
  URL_HASH SHA256=9880614984c75fc6ed1967b7aa29aebbea2f0c88f2d7c707b18391b5632091c0
  SOURCE_SUBDIR "."
)

# Prevent Sentry from being installed into macOS bundles and Linux packages
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME "SentryExcluded")
set(SENTRY_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sentry)

set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
