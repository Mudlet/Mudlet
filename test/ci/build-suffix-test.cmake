cmake_minimum_required(VERSION 3.25.1)

# Run as "MUDLET_VERSION_BUILD= cmake -P <this file>": checks the version suffix
# CMakeLists.txt writes into app-build.txt, which is how Mudlet tells release,
# PTB and development builds apart and what it reports to game servers.

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/MudletBuildSuffix.cmake)

function(expect_suffix description append_sha expected)
  mudlet_build_suffix(actual "abc1234" ${append_sha})
  if(NOT actual STREQUAL expected)
    message(SEND_ERROR "${description}: expected \"${expected}\", got \"${actual}\"")
  endif()
endfunction()

# set(ENV{...} "") unsets the variable, so the defined-but-empty value a release
# build exports can only come from the caller
if(NOT DEFINED ENV{MUDLET_VERSION_BUILD} OR NOT "$ENV{MUDLET_VERSION_BUILD}" STREQUAL "")
  message(FATAL_ERROR "run with MUDLET_VERSION_BUILD set to an empty value")
endif()
expect_suffix("a release" ON "")
expect_suffix("a release with the commit opted out" OFF "")

unset(ENV{MUDLET_VERSION_BUILD})
expect_suffix("a local build" ON "-dev-abc1234")
expect_suffix("a local build ignores the commit opt-out" OFF "-dev-abc1234")

set(ENV{MUDLET_VERSION_BUILD} "-ptb-2026-10-31")
expect_suffix("a PTB" ON "-ptb-2026-10-31-abc1234")

set(ENV{MUDLET_VERSION_BUILD} "-gesslar")
expect_suffix("a distributor's label" ON "-gesslar-abc1234")
expect_suffix("a distributor's label with the commit opted out" OFF "-gesslar")
