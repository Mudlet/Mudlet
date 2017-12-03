project(luazip LANGUAGES C)

find_package(Lua51 REQUIRED)

# Break each step into a separate command so any status message is output straight away
# The include directory setup for Zip is unusual in that as well as e.g. /usr/include/zip.h
# we need the path to an interal header zipconf.g that it calls for using '<''>'s
# i.e. SYSTEM #include delimiters which are typically located at e.g. /usr/lib/libzip/include/zipconf.h
# and using pkg-config is the recommended way to get the details.
# Spotted recommendation to use pkg-config here https://github.com/Homebrew/homebrew/issues/13390
find_package(PkgConfig)
if(NOT(PKG_CONFIG_FOUND))
  message(WARNING "Unable to use pkg_config - will possibly fail to find/use Zip library...")
endif()

if(PKG_CONFIG_FOUND)
  # Examining Homebrew (for MacOs) for libzzip:
  # https://bintray.com/homebrew/bottles/libzzip found that they use pkg-config
  # So use that to try and find what we want
  PKG_SEARCH_MODULE(PC_ZZIPLIB zziplib libzzip zzip)
  if(PC_ZZIPLIB_FOUND)
    if(PC_ZZIPLIB_zziplib_FOUND)
      message(STATUS "Using pkg_config, found \"zziplib\" version: ${PC_ZZIPLIB_zziplib_VERSION} with:")
    elseif(PC_ZZIPLIB_libzzip_FOUND)
      message(STATUS "Using pkg_config, found \"libzzip\" version: ${PC_ZZIPLIB_libzzip_VERSION} with:")
    elseif(PC_ZZIPLIB_zzip_FOUND)
      message(STATUS "Using pkg_config, found \"zzip\" version: ${PC_ZZIPLIB_zzip_VERSION} with:")
    else()
      message(STATUS "Using pkg_config, found Zzip version: ${PC_ZZIPLIB_VERSION} with:")
    endif()
    message(STATUS "  include directory(ies), ZZIPLIB_INCLUDE_DIRS: ${PC_ZZIPLIB_INCLUDE_DIRS} .")
    message(STATUS "  library(ies): ZZIPLIB_LIBRARY_DIRS: ${PC_ZZIPLIB_LIBRARY_DIRS}; ZZIPLIB_LIBDIR: ${PC_ZZIPLIB_LIBDIR}. ")
  elseif()
    message(WARNING "Using pkg_config, failed to find any version of Zziplib library!")
  endif()
endif()

if(NOT(PC_ZZIPLIB_FOUND))
  find_package(ZZIPLIB)
endif()
if(NOT((ZZIPLIB_FOUND) OR (PC_ZZIPLIB_FOUND) OR (PC_ZZIPLIB_zziplib_FOUND) OR (PC_ZZIPLIB_libzzip_FOUND) OR (PC_ZZIPLIB_zzip_FOUND)))
  message(WARNING "Failed to find any trace of zziplib (or zzip or libzzip)\n- so will not be able to build the (internal version for Mac builds) of the lua zip module that Mudlet needs.")
endif()

add_library(luazip INTERFACE)

target_link_libraries(luazip INTERFACE ${LUA_LIBRARIES})

if(PC_ZZIPLIB_FOUND)
  target_link_libraries(luazip INTERFACE ${PC_ZZIPLIB_LIBRARIES})
else()
  target_link_libraries(luazip INTERFACE ${ZZIPLIB_LIBRARIES})
endif()

target_include_directories(luazip INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${LUA_INCLUDE_DIR})

if(PC_ZZIPLIB_FOUND)
  target_include_directories(luazip INTERFACE ${PC_ZZIPLIB_INCLUDE_DIRS})
else()
  target_include_directories(luazip INTERFACE ${ZZIPLIB_INCLUDE_DIRS})
endif()
