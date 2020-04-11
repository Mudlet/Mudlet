# Lua can be compiled as either C or C++. Default configuration is C, set
# COMPILE_AS_CPP to ON to use C++. See
# http://stackoverflow.com/questions/13560945/c-and-c-library-using-longjmp for
# why would you want to do that. Primary differences: - Exceptions will be used
# instead of setjmp/longjmp - The name mangling for functions will be C++
# instead of C. - This is a source-incompatible change because extern "C" is
# chosen by the including application. - The lua.hpp header will not be
# available.

project(lua)

cmake_minimum_required(VERSION 2.8)

set(HDR_LIBLUA
    src/lapi.h
    src/lauxlib.h
    src/lcode.h
    src/ldebug.h
    src/ldo.h
    src/lfunc.h
    src/lgc.h
    src/llex.h
    src/llimits.h
    src/lmem.h
    src/lobject.h
    src/lopcodes.h
    src/lparser.h
    src/lstate.h
    src/lstring.h
    src/ltable.h
    src/ltm.h
    src/lua.h
    src/luaconf.h
    src/lualib.h
    src/lundump.h
    src/lvm.h
    src/lzio.h)

# Build Libraries
set(SRC_LIBLUA
    src/lapi.c
    src/lauxlib.c
    src/lbaselib.c
    src/lcode.c
    src/ldblib.c
    src/ldebug.c
    src/ldo.c
    src/ldump.c
    src/lfunc.c
    src/lgc.c
    src/linit.c
    src/liolib.c
    src/llex.c
    src/lmathlib.c
    src/lmem.c
    src/loadlib.c
    src/lobject.c
    src/lopcodes.c
    src/loslib.c
    src/lparser.c
    src/lstate.c
    src/lstring.c
    src/lstrlib.c
    src/ltable.c
    src/ltablib.c
    src/ltm.c
    src/lundump.c
    src/lvm.c
    src/lzio.c
    src/print.c)

if(COMPILE_AS_CPP)
  set_source_files_properties(${SRC_LIBLUA} src/lua.c src/luac.c
                              PROPERTIES LANGUAGE CXX)
else()
  set(CMAKE_C_STANDARD 99)
endif()

# append headers to sources to make them show up in MSVC GUI
list(APPEND SRC_LIBLUA ${HDR_LIBLUA})

if(WIN32)
  # remove warnings
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

if(UNIX)
  add_definitions(-DLUA_USE_POSIX)
  find_library(LIB_MATH NAMES m)
  if(LIB_MATH)
    link_libraries(${LIB_MATH})
  endif()
endif()

# DLL
add_library(lua ${SRC_LIBLUA})
if(COMPILE_AS_CPP)
  set_target_properties(lua PROPERTIES OUTPUT_NAME "lua-c++")
endif()

if(BUILD_SHARED_LIBS AND WIN32)
  target_compile_definitions(lua PUBLIC -DLUA_BUILD_AS_DLL)
endif()

if(UNIX)
  if(APPLE)
    target_compile_definitions(lua PUBLIC -DLUA_USE_DLOPEN)
  else()
    find_library(LIB_DLOPEN NAMES dl)
    if(LIB_DLOPEN)
      target_compile_definitions(lua PUBLIC -DLUA_USE_DLOPEN)
      target_link_libraries(lua ${LIB_DLOPEN})
    endif()
  endif()
endif()

install(
  TARGETS lua
  RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

if(NOT DEFINED SKIP_INSTALL_TOOLS)
  add_executable(
    luac src/luac.c ${SRC_LIBLUA}) # compiler uses non-exported APIs, so must
                                   # include sources directly.
  add_executable(luai src/lua.c) # interpreter
  target_link_libraries(luai lua)
  set_target_properties(luai PROPERTIES OUTPUT_NAME lua PDB_NAME luai)
  if(UNIX)
    if(CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
      set(_LIB_READLINE_NAME edit)
    else()
      set(_LIB_READLINE_NAME readline)
    endif()
    find_library(LIB_READLINE NAMES ${_LIB_READLINE_NAME})
    if(LIB_READLINE)
      target_compile_definitions(luai PUBLIC -DLUA_USE_READLINE)
      target_link_libraries(luai ${LIB_READLINE})
      if(_LIB_READLINE_NAME STREQUAL edit)
        target_include_directories(luai PUBLIC /usr/include/edit)
      endif()
    endif()
  endif()
  install(TARGETS luai luac
          RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools/lua)
endif()

if(NOT DEFINED SKIP_INSTALL_HEADERS)
  install(FILES src/lualib.h src/lua.h src/luaconf.h src/lauxlib.h
          DESTINATION include)
  # If using C++, don't install extern "C" wrapper.
  if(NOT COMPILE_AS_CPP)
    install(FILES etc/lua.hpp DESTINATION include)
  endif()
endif()
