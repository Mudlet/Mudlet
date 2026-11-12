# Builds the Lua C modules Mudlet's Lua requires into mudlet_lua_modules, which mudlet_core links and
# registers in package.preload; see "Embedding it: Mudlet's own Lua files" in docs/headless.md. Each is
# fetched at configure time, pinned to the commit of the release Mudlet's own builds install, and
# FETCHCONTENT_SOURCE_DIR_<NAME> builds one from a local copy instead.

include(FetchContent)

set(mudlet_lua_module_sources)
function(mudlet_bundled_lua_module NAME)
  cmake_parse_arguments(MODULE "" "GIT_REPOSITORY;GIT_TAG" "SOURCES" ${ARGN})
  # Nothing is at this subdirectory, so FetchContent_MakeAvailable fetches the module and builds none
  # of its own build files, which make a shared module for a desktop Lua
  FetchContent_Declare(${NAME}
                       GIT_REPOSITORY ${MODULE_GIT_REPOSITORY}
                       GIT_TAG ${MODULE_GIT_TAG}
                       SOURCE_SUBDIR mudlet-builds-this-itself)
  FetchContent_MakeAvailable(${NAME})
  list(TRANSFORM MODULE_SOURCES PREPEND "${${NAME}_SOURCE_DIR}/")
  set(mudlet_lua_module_sources ${mudlet_lua_module_sources} ${MODULE_SOURCES} PARENT_SCOPE)
endfunction()

mudlet_bundled_lua_module(lfs
  GIT_REPOSITORY https://github.com/lunarmodules/luafilesystem.git
  GIT_TAG a186cca5833691e830ed255e38ace8ff6b870dbf # v1_9_0
  SOURCES src/lfs.c)

mudlet_bundled_lua_module(lpeg
  GIT_REPOSITORY https://github.com/roberto-ieru/LPeg.git
  GIT_TAG 73a96935337d78d0e1015d7ccce6cf7470c192c4 # v1.1.0
  SOURCES lpcap.c lpcode.c lpcset.c lpprint.c lptree.c lpvm.c)

mudlet_bundled_lua_module(lrexlib
  GIT_REPOSITORY https://github.com/rrthomas/lrexlib.git
  GIT_TAG d2bae6c69b0c1ce5442eab103b9782c9d13ba479 # rel-2-9-4
  SOURCES src/common.c src/pcre2/lpcre2.c src/pcre2/lpcre2_f.c)

mudlet_bundled_lua_module(luautf8
  GIT_REPOSITORY https://github.com/starwing/luautf8.git
  GIT_TAG 08b0fc930f5a52eff36348ed1ea39aadfc697fa6 # 0.2.1
  SOURCES lutf8lib.c)

mudlet_bundled_lua_module(lua_zip
  GIT_REPOSITORY https://github.com/brimworks/lua-zip.git
  GIT_TAG e01fac77e7ebf1feceed8f95a5de96aa2758e06f # v0.2.0
  SOURCES lua_zip.c)

mudlet_bundled_lua_module(lua_yajl
  GIT_REPOSITORY https://github.com/brimworks/lua-yajl.git
  GIT_TAG 167c294046d5bbdc5bc4a791e6397439b6c825df # v2.1
  SOURCES lua_yajl.c)

# What the luasql-sqlite3 2.6.1 rock that CI/install-lua-rocks.sh pins is built from: db:create fails
# on LuaSQL 2.7 and newer
mudlet_bundled_lua_module(luasql
  GIT_REPOSITORY https://github.com/lunarmodules/luasql.git
  GIT_TAG 22d4a911f35cf851af9db71124e3998d96fb3fa1 # 2.6.0
  SOURCES src/luasql.c src/ls_sqlite3.c)

FetchContent_GetProperties(lrexlib)
find_package(SQLite3 REQUIRED)
add_library(mudlet_lua_modules STATIC ${mudlet_lua_module_sources})
set_target_properties(mudlet_lua_modules PROPERTIES
    C_STANDARD 99
    C_EXTENSIONS ON
    POSITION_INDEPENDENT_CODE ON
    # Mudlet's executable exports its symbols, for the Lua modules scripts load. These modules' helpers have
    # names other code uses too, such as lrexlib's buffer_free(), so exported they would answer a module or
    # library loaded later that calls its own by that name, as another build of lrexlib does.
    C_VISIBILITY_PRESET hidden)
# Where Lua is a DLL, its headers mark what a module defines as imported from it unless this is defined
target_compile_definitions(mudlet_lua_modules PRIVATE LUA_LIB=)
set_source_files_properties("${lrexlib_SOURCE_DIR}/src/common.c" "${lrexlib_SOURCE_DIR}/src/pcre2/lpcre2.c"
                            "${lrexlib_SOURCE_DIR}/src/pcre2/lpcre2_f.c"
                            PROPERTIES COMPILE_DEFINITIONS "VERSION=\"2.9.4\";PCRE2_CODE_UNIT_WIDTH=8")
target_link_libraries(mudlet_lua_modules PUBLIC LUA51::LUA51 PCRE2::PCRE2 YAJL::YAJL ZIP::ZIP SQLite::SQLite3)
