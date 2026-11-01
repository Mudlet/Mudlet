# Builds the Lua C modules that mudlet-lua requires as static libraries linked into
# mudlet_core, for a platform that cannot dlopen them; see "Lua C modules without dlopen" in
# docs/platform-builds.md. Each is pinned to a release's commit; FETCHCONTENT_SOURCE_DIR_<NAME>
# builds one from a local copy instead.

include(FetchContent)

# Builds what the module's rockspec builds: its own build files, where it has any, make a
# shared module for a desktop Lua
function(mudlet_static_lua_module NAME)
  cmake_parse_arguments(MODULE "" "GIT_REPOSITORY;GIT_TAG" "SOURCES;DEFINITIONS;LIBRARIES" ${ARGN})
  # No CMakeLists.txt is in this subdirectory, so FetchContent_MakeAvailable only fetches
  FetchContent_Declare(${NAME}
                       GIT_REPOSITORY ${MODULE_GIT_REPOSITORY}
                       GIT_TAG ${MODULE_GIT_TAG}
                       SOURCE_SUBDIR mudlet-builds-this-itself)
  FetchContent_MakeAvailable(${NAME})

  set(sources ${MODULE_SOURCES})
  list(TRANSFORM sources PREPEND "${${NAME}_SOURCE_DIR}/")
  set(target mudlet_lua_${NAME})
  add_library(${target} STATIC ${sources})
  target_include_directories(${target} PRIVATE "${${NAME}_SOURCE_DIR}")
  # As the rockspecs pass it: under the Windows bundled Lua's LUA_BUILD_AS_DLL, LUALIB_API is
  # dllimport without it, which clang refuses on the luaopen_* definitions
  target_compile_definitions(${target} PRIVATE LUA_LIB= ${MODULE_DEFINITIONS})
  target_link_libraries(${target} PRIVATE LUA51::LUA51 ${MODULE_LIBRARIES})
  set_target_properties(${target} PROPERTIES C_STANDARD 99 C_EXTENSIONS ON POSITION_INDEPENDENT_CODE ON)
  # mudlet_core links with the plain signature, which cannot be mixed with PRIVATE
  target_link_libraries(${LIB_MUDLET_TARGET} ${target})
endfunction()

mudlet_static_lua_module(lfs
  GIT_REPOSITORY https://github.com/lunarmodules/luafilesystem.git
  GIT_TAG a186cca5833691e830ed255e38ace8ff6b870dbf # v1_9_0
  SOURCES src/lfs.c)

mudlet_static_lua_module(lpeg
  GIT_REPOSITORY https://github.com/roberto-ieru/LPeg.git
  GIT_TAG 73a96935337d78d0e1015d7ccce6cf7470c192c4 # v1.1.0
  SOURCES lpcap.c lpcode.c lpcset.c lpprint.c lptree.c lpvm.c)

mudlet_static_lua_module(rex_pcre2
  GIT_REPOSITORY https://github.com/rrthomas/lrexlib.git
  GIT_TAG d2bae6c69b0c1ce5442eab103b9782c9d13ba479 # rel-2-9-4
  SOURCES src/common.c src/pcre2/lpcre2.c src/pcre2/lpcre2_f.c
  DEFINITIONS VERSION="2.9.4" PCRE2_CODE_UNIT_WIDTH=8
  LIBRARIES PCRE2::PCRE2)

mudlet_static_lua_module(luautf8
  GIT_REPOSITORY https://github.com/starwing/luautf8.git
  GIT_TAG 08b0fc930f5a52eff36348ed1ea39aadfc697fa6 # 0.2.1
  SOURCES lutf8lib.c)
