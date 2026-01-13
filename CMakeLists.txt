############################################################################
#    Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            #
#    Copyright (C) 2015-2018, 2020, 2022, 2024-2025 by Stephen Lyons       #
#                                                - slysven@virginmedia.com #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

############################################################################
#                                                                          #
#    NOTICE: FreeBSD, OpenBSD and GNU/Hurd are not officially supported    #
#    platforms as such; the work on getting them working has been done by  #
#    myself, and other developers, unless they have explicitly said so,    #
#    are not able to address issues relating specifically to these         #
#    Operating Systems. Nevertheless users of these operating systems are  #
#    equally welcome to contribute to the development of Mudlet - bugfixes #
#    and enhancements are welcome from all!                                #
#        Stephen Lyons, February 2018, updated March 2021 & October 2022   #
#                                                                          #
############################################################################

# Should be called before PROJECT.
cmake_minimum_required(VERSION 3.25.1)

if(APPLE)
  # minimum supported version by Qt6
  set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "")
endif()

project(mudlet)

# All the other files that we want to see in the editor but which CMake doesn't
# do anything with.
add_custom_target(OTHER_FILES
  COMMENT "Refreshing OTHER_FILES list for passive file display in IDE"
  SOURCES
  .clang-tidy
  .claude/commands/pr-mudlet.md
  .DirIcon
  .crowdin.yml
  .cursorrules
  .devcontainer/Dockerfile
  .devcontainer/devcontainer.json
  .devcontainer/library-scripts/desktop-lite-debian.sh
  .github/CODEOWNERS
  .github/ISSUE_TEMPLATE/01-bug-report.md
  .github/ISSUE_TEMPLATE/02-feature-request.md
  .github/ISSUE_TEMPLATE/config.yml
  .github/PULL_REQUEST_TEMPLATE.md
  .github/codeql/codeql-config.yml
  .github/codespell-wordlist.txt
  .github/copilot-instructions.md
  .github/dependabot.yml
  .github/pr-labeler.yml
  .github/repo-metadata.yml
  .github/workflows/build-mudlet-win.yml
  .github/workflows/build-mudlet.yml
  .github/workflows/check_pr_labels.yml
  .github/workflows/clangtidy-diff-analysis.yml
  .github/workflows/clangtidy-post-comments.yml
  .github/workflows/codeql-analysis.yml
  .github/workflows/codespell-analysis.yml
  .github/workflows/dangerjs.yml
  .github/workflows/generate-changelog.yml
  .github/workflows/generate-coder-attribution.yml
  .github/workflows/link-ptbs-to-dblsqd.yml
  .github/workflows/performance-analysis.yml
  .github/workflows/qridlayout-ordering.yml
  .github/workflows/tag-pull-requests.yml
  .github/workflows/update-3rdparty.yml
  .github/workflows/update-autocompletion.yml
  .github/workflows/update-en-us-plural.yml
  .github/workflows/update-geyser-docs.yml
  .github/workflows/update-translations.yml
  .gitignore
  .gitmodules
  .imgbotconfig
  .vscode/c_cpp_properties.json
  .vscode/extensions.json
  .vscode/settings.json
  3rdparty/communi/CHANGELOG.md
  3rdparty/communi/LICENSE
  3rdparty/communi/README.md
  3rdparty/communi/communi.pri
  3rdparty/communi/version.pri
  3rdparty/discord/rpc/include/discord_register.h
  3rdparty/discord/rpc/include/discord_rpc.h
  3rdparty/discord/rpc/lib/discord-rpc32.dll
  3rdparty/discord/rpc/lib/discord-rpc64.dll
  3rdparty/discord/rpc/lib/libdiscord-rpc.dylib
  3rdparty/discord/rpc/lib/libdiscord-rpc.so
  3rdparty/kdtoolbox/singleshot_connect/singleshot_connect.h
  AGENTS.md
  CI/after_success.sh
  CI/before_install.sh
  CI/build-mudlet-for-windows.sh
  CI/deploy-mudlet-for-windows.sh
  CI/fix.grid.ui.ordering.js
  CI/generate-changelog.lua
  CI/install.sh
  CI/linux.after_success.sh
  CI/linux.install.sh
  CI/lua-5.1.5-so.patch
  CI/org.mudlet.mudlet.yml
  CI/osx.after_success.sh
  CI/osx.before_install.sh
  CI/osx.install.sh
  CI/package-mudlet-for-windows.sh
  CI/register-windows-release.sh
  CI/send_debug_files_to_sentry.sh
  CI/set-build-info.sh
  CI/setup-windows-sdk.sh
  CI/update-autocompletion.lua
  CI/validate-deployment-for-windows.sh
  CI/validate_deployment.sh
  CLAUDE.md
  COMMITMENT
  COMPILE
  COPYING
  CPPLINT.cfg
  README.md
  SECURITY.md
  UI-design-philosophy.md
  asan-suppressions.txt
  dangerfile.js
  docker/.env.template
  docker/Dockerfile
  docker/docker-compose.override.linux.yml
  docker/docker-compose.yml
  docs/AI-ASSISTANTS.md
  docs/CODE_OF_CONDUCT.md
  docs/CONTRIBUTING.md
  docs/FUNDING.yml
  docs/GITHUB.md
  docs/README.md
  docs/SUPPORT.md
  docs/ai-instructions.md
  icon_1024x1204.png
  icon_dev_1024x1204.png
  icon_ptb_1024x1204.png
  mudlet.desktop
  mudlet.png
  mudlet.svg
  src/.clang-format
  src/.gitignore
)

if(CMAKE_CONFIGURATION_TYPES)
  add_custom_target(
    check COMMAND ${CMAKE_CTEST_COMMAND} --force-new-ctest-process
                  --output-on-failure --build-config "$<CONFIGURATION>")
else()
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND}
                                  --force-new-ctest-process --output-on-failure)
endif()

enable_testing()

if(WIN32)
  set(APP_TARGET mudlet.exe)
elseif(APPLE)
  set(APP_TARGET Mudlet)
else()
  set(APP_TARGET mudlet)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WITH_SENTRY "Enable crash reporting via Sentry" OFF)
option(SENTRY_SEND_DEBUG "Send debug files to Sentry after build" OFF)

find_package(Git REQUIRED)
# Get Git SHA1 hash
execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_SHA1
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# If a CI/CB build system provides an alternative Git SHA1 to identify the build
# use that instead - reporting which is being used:
if(DEFINED ENV{BUILD_COMMIT} AND NOT $ENV{BUILD_COMMIT} STREQUAL "")
    string(TOLOWER $ENV{BUILD_COMMIT} GIT_SHA1)
    message(STATUS "Git SHA1 set from environemnt: ${GIT_SHA1}")
else()
    message(STATUS "Git SHA1 used: ${GIT_SHA1}")
endif()

# Set APP_VERSION
set(APP_VERSION 4.19.1)

# Set APP_BUILD based on environment variable MUDLET_VERSION_BUILD or default
if(DEFINED ENV{MUDLET_VERSION_BUILD} AND NOT $ENV{MUDLET_VERSION_BUILD} STREQUAL "")
    set(BUILD_VALUE "$ENV{MUDLET_VERSION_BUILD}-${GIT_SHA1}")
else()
    set(BUILD_VALUE "-dev-${GIT_SHA1}")
endif()

# Write to app-build.txt and set APP_BUILD
file(WRITE ${CMAKE_SOURCE_DIR}/src/app-build.txt "${BUILD_VALUE}")
message(STATUS "Value written to app-build.txt file: ${BUILD_VALUE}")
set(APP_BUILD ${BUILD_VALUE} CACHE STRING "Auto-set during CMake run. Do not alter manually." FORCE)

set(LIB_MUDLET_TARGET "mudlet_core")

# For release builds, comment out the above and uncomment below:
# file(WRITE ${CMAKE_SOURCE_DIR}/src/app-build.txt " ")
# message(STATUS "Value written to app-build.txt file: {nothing}")
# set(APP_BUILD "" CACHE STRING "Auto-set during CMake run. Do not alter manually." FORCE)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

# Static analysis support - always included, but only active when configured with
# -DENABLE_STATIC_ANALYSIS=ON (see cmake/StaticAnalysis.cmake for details)
include(StaticAnalysis)

include(IncludeOptionalModule)

include_optional_module(ENVIRONMENT_VARIABLE WITH_UPDATER
                        OPTION_VARIABLE USE_UPDATER
                        READABLE_NAME "updater"
                        SUPPORTED_SYSTEMS "Linux"
                                          "Windows"
                                          "Darwin")

include_optional_module(ENVIRONMENT_VARIABLE WITH_FONTS
                        OPTION_VARIABLE USE_FONTS
                        READABLE_NAME "fonts")
include_optional_module(ENVIRONMENT_VARIABLE WITH_3DMAPPER
                        OPTION_VARIABLE USE_3DMAPPER
                        READABLE_NAME "3D mapper")
include_optional_module(ENVIRONMENT_VARIABLE WITH_SHADER_HOT_RELOAD
                        OPTION_VARIABLE USE_SHADER_HOT_RELOAD
                        READABLE_NAME "shader hot-reloading"
                        DEFAULT OFF)
include_optional_module(ENVIRONMENT_VARIABLE WITH_VARIABLE_SPLASH_SCREEN
                        OPTION_VARIABLE USE_VARIABLE_SPLASH_SCREEN
                        READABLE_NAME "build-type splash screen")
include_optional_module(ENVIRONMENT_VARIABLE WITH_OWN_QTKEYCHAIN
                        OPTION_VARIABLE USE_OWN_QTKEYCHAIN
                        READABLE_NAME "own QtKeychain library")

find_package(Qt6 6.8.2 REQUIRED)

# Find Core5Compat for third-party libraries (communi, edbee-lib) that still need it
find_package(Qt6 COMPONENTS Core5Compat REQUIRED)

find_package(Lua 5.1 EXACT)

# Set Qt version for edbee-lib and dblsqd
option(BUILD_WITH_QT5 "" OFF)
# Set Qt version for qtkeychain
option(BUILD_WITH_QT6 "" ON)

include(InitGitSubmodule)

git_submodule_init(
  CHECK_FILE "3rdparty/edbee-lib/CMakeLists.txt" SUBMODULE_PATH
  "3rdparty/edbee-lib" READABLE_NAME "edbee-lib editor widget")

  git_submodule_init(
    CHECK_FILE "3rdparty/qt-tags-widget/CMakeLists.txt" SUBMODULE_PATH
    "3rdparty/qt-tags-widget" READABLE_NAME "qt-tags-widget widget")

if(USE_OWN_QTKEYCHAIN)
  git_submodule_init(
    CHECK_FILE "3rdparty/qtkeychain/CMakeLists.txt"
    SUBMODULE_PATH "3rdparty/qtkeychain"
    READABLE_NAME "QtKeychain")
  add_subdirectory(3rdparty/qtkeychain)

  # Handle both old (files in qtkeychain/) and new (files in qtkeychain/qtkeychain/) directory structures
  # so includes via <qtkeychain/keychain.h> work in both cases
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qtkeychain/qtkeychain/keychain.h")
    # Old structure: add 3rdparty to include path so <qtkeychain/keychain.h> resolves correctly
    target_include_directories(qt6keychain PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rdparty>)
  endif()
endif()

git_submodule_init(
  CHECK_FILE "3rdparty/lcf/lcf-scm-1.rockspec" SUBMODULE_PATH "3rdparty/lcf"
  READABLE_NAME "lua code formatter source code")

# PLACEMARKER: sample benchmarking code
# include(FetchContent)
# FetchContent_Declare(
#     nanobench
#     GIT_REPOSITORY https://github.com/martinus/nanobench.git
#     GIT_TAG v4.3.11
#     GIT_SHALLOW TRUE)
# FetchContent_MakeAvailable(nanobench)

if(USE_UPDATER)
  git_submodule_init(CHECK_FILE "3rdparty/dblsqd/CMakeLists.txt" SUBMODULE_PATH
                     "3rdparty/dblsqd" READABLE_NAME "DBLSQD updater")
endif()

if(APPLE)
  if(USE_UPDATER)
    if(NOT EXISTS "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
      message(
        STATUS "Sparkle is missing, fetching it..."
      )
      file(DOWNLOAD "https://github.com/sparkle-project/Sparkle/releases/download/2.7.1/Sparkle-2.7.1.tar.xz" "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle-2.7.1.tar.xz"
        TIMEOUT 60  # seconds
        TLS_VERIFY ON
      )
      file(ARCHIVE_EXTRACT INPUT "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle-2.7.1.tar.xz"
        DESTINATION "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle")
      file(REMOVE "${CMAKE_HOME_DIRECTORY}/3rdparty/cocoapods/Pods/Sparkle-2.7.1.tar.xz")
    endif()
  endif()
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

add_subdirectory(3rdparty/edbee-lib/edbee-lib)
add_subdirectory(3rdparty/communi)
add_subdirectory(3rdparty/qt-tags-widget)
add_subdirectory(translations/translated)
add_subdirectory(src)
add_subdirectory(test)

if(USE_UPDATER)
  add_subdirectory(3rdparty/dblsqd)
endif()

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE -O3)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG -O0)
