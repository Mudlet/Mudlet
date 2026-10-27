# Lua 5.1.5 with the changes README.md lists. Linked into the executable on
# Linux and macOS, which exports the API to the C modules scripts load, as the
# stand-alone lua interpreter does; a DLL on Windows, where those modules import
# it by name.
set(LUA51_SOURCES
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
    src/lzio.c)

if(WIN32)
  add_library(lua51 SHARED ${LUA51_SOURCES})
  set_target_properties(lua51 PROPERTIES PREFIX "" OUTPUT_NAME lua51)
  # Beside mudlet.exe, which looks there before the MSYS2 lua51.dll on PATH
  if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(lua51 PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/src")
  endif()
  target_compile_definitions(lua51 PUBLIC LUA_BUILD_AS_DLL)
else()
  add_library(lua51 STATIC ${LUA51_SOURCES})
  set_target_properties(lua51 PROPERTIES POSITION_INDEPENDENT_CODE ON)
  target_compile_definitions(lua51 PRIVATE LUA_USE_LINUX)
  if(CMAKE_LIBRARY_ARCHITECTURE)
    target_compile_definitions(lua51 PRIVATE LUA_MULTIARCH="${CMAKE_LIBRARY_ARCHITECTURE}")
  endif()
  target_link_libraries(lua51 PRIVATE ${CMAKE_DL_LIBS})
  if(NOT APPLE)
    target_link_libraries(lua51 PRIVATE m)
  endif()
endif()

set_target_properties(lua51 PROPERTIES C_STANDARD 99 C_EXTENSIONS ON)
# The VM measures faster at -O2 than at the -O3 a Release build gives C
if(NOT MSVC)
  target_compile_options(lua51 PRIVATE $<$<CONFIG:Release>:-O2>)
endif()
target_include_directories(lua51 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
add_library(LUA51::LUA51 ALIAS lua51)
