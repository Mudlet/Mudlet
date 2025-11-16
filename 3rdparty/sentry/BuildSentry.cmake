# Copyright (C) 2004-2024 Robert Griebl
# SPDX-License-Identifier: GPL-3.0-only
# credit to https://github.com/rgriebl/brickstore/blob/main/cmake/BuildSentry.cmake
include(FetchContent)
FetchContent_Declare(
  sentry
  URL "https://github.com/getsentry/sentry-native/releases/download/0.7.15/sentry-native.zip"
  URL_HASH SHA256=c4d62bcd72a67444edafc3d5bd2b53c9c63dc7f7e16a1a93e0b0aa2edd7e7b8b
  SOURCE_SUBDIR "."
)
FetchContent_MakeAvailable(sentry)
set(mll ${CMAKE_MESSAGE_LOG_LEVEL})
if (NOT VERBOSE_FETCH)
  set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)
endif()
# we need EXCLUDE_FROM_ALL to suppress the installation of sentry into the macOS bundle
# and Linux packages
# TODO - resolve this
add_subdirectory(${sentry_SOURCE_DIR} ${sentry_BINARY_DIR} EXCLUDE_FROM_ALL)
set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
