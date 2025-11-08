# Copyright (C) 2004-2024 Robert Griebl
# SPDX-License-Identifier: GPL-3.0-only
# credit to https://github.com/rgriebl/brickstore/blob/main/cmake/BuildSentry.cmake
include(FetchContent)
FetchContent_Declare(
    sentry
    URL https://github.com/getsentry/sentry-native/releases/download/0.10.0/sentry-native.zip
    SOURCE_SUBDIR "NeedManualAddSubDir"
    URL_HASH SHA256=fe9c40e0f677405f61a8bae1bfdbb9760a84b9594665d92781ed4e18e4b1da1f
)
FetchContent_MakeAvailable(sentry)
set(mll ${CMAKE_MESSAGE_LOG_LEVEL})
if (NOT VERBOSE_FETCH)
  set(CMAKE_MESSAGE_LOG_LEVEL NOTICE)
endif()
# we need EXCLUDE_FROM_ALL to suppress the installation of sentry into the macOS bundle
# and Linux packages
# TODO - resolve this
add_subdirectory(${sentry_SOURCE_DIR} ${sentry_BINARY_DIR})
set(CMAKE_MESSAGE_LOG_LEVEL ${mll})
