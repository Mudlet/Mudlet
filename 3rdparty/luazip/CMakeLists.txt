PROJECT(luazip LANGUAGES C)

FIND_PACKAGE(Lua51 REQUIRED)

# Break each step into a separate command so any status message is output straight away
# The include directory setup for Zip is unusual in that as well as e.g. /usr/include/zip.h
# we need the path to an interal header zipconf.g that it calls for using '<''>'s
# i.e. SYSTEM #include delimiters which are typically located at e.g. /usr/lib/libzip/include/zipconf.h
# and using pkg-config is the recommended way to get the details.
# Spotted recommendation to use pkg-config here https://github.com/Homebrew/homebrew/issues/13390
FIND_PACKAGE(PkgConfig)
IF(NOT(PKG_CONFIG_FOUND))
  MESSAGE(WARNING "Unable to use pkg_config - will possibly fail to find/use Zip library...")
ENDIF()

IF(PKG_CONFIG_FOUND)
  # Examining Homebrew (for MacOs) for libzzip:
  # https://bintray.com/homebrew/bottles/libzzip found that they use pkg-config
  # So use that to try and find what we want
  PKG_SEARCH_MODULE(PC_ZZIPLIB zziplib libzzip zzip)
  IF(PC_ZZIPLIB_FOUND)
    IF(PC_ZZIPLIB_zziplib_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"zziplib\" version: ${PC_ZZIPLIB_zziplib_VERSION} with:")
    ELSEIF(PC_ZZIPLIB_libzzip_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"libzzip\" version: ${PC_ZZIPLIB_libzzip_VERSION} with:")
    ELSEIF(PC_ZZIPLIB_zzip_FOUND)
      MESSAGE(STATUS "Using pkg_config, found \"zzip\" version: ${PC_ZZIPLIB_zzip_VERSION} with:")
    ELSE()
      MESSAGE(STATUS "Using pkg_config, found Zzip version: ${PC_ZZIPLIB_VERSION} with:")
    ENDIF()
    MESSAGE(STATUS "  include directory(ies), ZZIPLIB_INCLUDE_DIRS: ${PC_ZZIPLIB_INCLUDE_DIRS} .")
    MESSAGE(STATUS "  library(ies): ZZIPLIB_LIBRARY_DIRS: ${PC_ZZIPLIB_LIBRARY_DIRS}; ZZIPLIB_LIBDIR: ${PC_ZZIPLIB_LIBDIR}. ")
  ELSEIF()
    MESSAGE(WARNING "Using pkg_config, failed to find any version of Zziplib library!")
  ENDIF()
ENDIF()

IF(NOT(PC_ZZIPLIB_FOUND))
  FIND_PACKAGE(ZZIPLIB)
ENDIF()
IF(NOT((ZZIPLIB_FOUND) OR (PC_ZZIPLIB_FOUND) OR (PC_ZZIPLIB_zziplib_FOUND) OR (PC_ZZIPLIB_libzzip_FOUND) OR (PC_ZZIPLIB_zzip_FOUND)))
  MESSAGE(WARNING "Failed to find any trace of zziplib (or zzip or libzzip)\n- so will not be able to build the (internal version for Mac builds) of the lua zip module that Mudlet needs.")
ENDIF()

ADD_LIBRARY(luazip INTERFACE)

TARGET_LINK_LIBRARIES(luazip INTERFACE ${LUA_LIBRARIES})

IF(PC_ZZIPLIB_FOUND)
  TARGET_LINK_LIBRARIES(luazip INTERFACE ${PC_ZZIPLIB_LIBRARIES})
ELSE()
  TARGET_LINK_LIBRARIES(luazip INTERFACE ${ZZIPLIB_LIBRARIES})
ENDIF()

TARGET_INCLUDE_DIRECTORIES(luazip INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} ${LUA_INCLUDE_DIR})

IF(PC_ZZIPLIB_FOUND)
  TARGET_INCLUDE_DIRECTORIES(luazip INTERFACE ${PC_ZZIPLIB_INCLUDE_DIRS})
ELSE()
  TARGET_INCLUDE_DIRECTORIES(luazip INTERFACE ${ZZIPLIB_INCLUDE_DIRS})
ENDIF()
