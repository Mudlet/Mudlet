# Install script for directory: /media/vadi/SSDer/Programs/Mudlet/3rdparty/qtkeychain

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/qt5keychain/translations" TYPE FILE FILES
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qtkeychain_de.qm"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qtkeychain_fr.qm"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qtkeychain_ro.qm"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qtkeychain_ru.qm"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qtkeychain_zh.qm"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/qt5keychain" TYPE FILE FILES
    "/media/vadi/SSDer/Programs/Mudlet/3rdparty/qtkeychain/keychain.h"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qkeychain_export.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so.0.13.99"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "/media/vadi/SSDer/Programs/Qt/5.15.2/gcc_64/lib")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/libqt5keychain.so.0.13.99"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/libqt5keychain.so.1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so.0.13.99"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHANGE
           FILE "${file}"
           OLD_RPATH "/media/vadi/SSDer/Programs/Qt/5.15.2/gcc_64/lib:"
           NEW_RPATH "/media/vadi/SSDer/Programs/Qt/5.15.2/gcc_64/lib")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so"
         RPATH "/media/vadi/SSDer/Programs/Qt/5.15.2/gcc_64/lib")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/libqt5keychain.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so"
         OLD_RPATH "/media/vadi/SSDer/Programs/Qt/5.15.2/gcc_64/lib:"
         NEW_RPATH "/media/vadi/SSDer/Programs/Qt/5.15.2/gcc_64/lib")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libqt5keychain.so")
    endif()
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/mkspecs/modules" TYPE FILE FILES "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/qt_Qt5Keychain.pri")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends.cmake")
    file(DIFFERENT EXPORT_FILE_CHANGED FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends.cmake"
         "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/CMakeFiles/Export/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends.cmake")
    if(EXPORT_FILE_CHANGED)
      file(GLOB OLD_CONFIG_FILES "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends-*.cmake")
      if(OLD_CONFIG_FILES)
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends.cmake\" will be replaced.  Removing files [${OLD_CONFIG_FILES}].")
        file(REMOVE ${OLD_CONFIG_FILES})
      endif()
    endif()
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain" TYPE FILE FILES "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/CMakeFiles/Export/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends.cmake")
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain" TYPE FILE FILES "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/CMakeFiles/Export/lib/cmake/Qt5Keychain/Qt5KeychainLibraryDepends-debug.cmake")
  endif()
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/Qt5Keychain" TYPE FILE FILES
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/Qt5KeychainConfig.cmake"
    "/media/vadi/SSDer/Programs/Mudlet/cmake-build-debug-clang-event-trace/3rdparty/qtkeychain/Qt5KeychainConfigVersion.cmake"
    )
endif()

