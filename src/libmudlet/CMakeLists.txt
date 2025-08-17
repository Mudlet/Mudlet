#############################################################################
#    Copyright (C) 2025 by Rishi Mondal - mavrickrishi@gmail.com           #
#                                                                           #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                           #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                           #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
#############################################################################

cmake_minimum_required(VERSION 3.25.1)

project(libmudlet
    VERSION 1.0.0
    DESCRIPTION "Core MUD client library - UI framework independent"
    LANGUAGES CXX
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(LIBMUDLET_BUILD_SHARED "Build libmudlet as a shared library" ON)
option(LIBMUDLET_BUILD_TESTS "Build libmudlet tests" ON)
option(LIBMUDLET_BUILD_EXAMPLES "Build libmudlet examples" OFF)
option(LIBMUDLET_ENABLE_PROFILING "Enable profiling support" OFF)

# Find required packages
find_package(Lua 5.1 EXACT REQUIRED)

# Platform-specific dependencies
if(WIN32)
    # Windows-specific libraries
    set(PLATFORM_LIBS ws2_32 winmm)
elseif(APPLE)
    # macOS-specific libraries
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    set(PLATFORM_LIBS ${COREFOUNDATION_LIBRARY})
else()
    # Linux/Unix-specific libraries
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PCRE REQUIRED libpcre)
    set(PLATFORM_LIBS ${PCRE_LIBRARIES} pthread)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LUA_INCLUDE_DIR}
)

# Source files - PR #1 Phase: Core engines and foundation
set(LIBMUDLET_SOURCES
    # Core implementation
    src/libmudlet.cpp
    src/core_impl.cpp
    src/session_impl.cpp
    
    # Engines (implemented)
    src/engines/trigger_engine_impl.cpp
    src/engines/alias_engine_impl.cpp
    src/engines/timer_engine_impl.cpp
    
    # Utilities (implemented)
    src/utils/pattern_matcher.cpp
    src/utils/logger.cpp
)

# Header files for installation
set(LIBMUDLET_HEADERS
    include/mudlet/libmudlet.h
    include/mudlet/core.h
    include/mudlet/session_simple.h
    include/mudlet/engines.h
    include/mudlet/network.h
)

# Create the library
if(LIBMUDLET_BUILD_SHARED)
    add_library(mudlet SHARED ${LIBMUDLET_SOURCES})
    target_compile_definitions(mudlet PRIVATE LIBMUDLET_BUILD_SHARED)
else()
    add_library(mudlet STATIC ${LIBMUDLET_SOURCES})
endif()

# Set library properties
set_target_properties(mudlet PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${LIBMUDLET_HEADERS}"
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN NO
)

# Compiler-specific flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mudlet PRIVATE
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -fvisibility=default
    )
    
    if(LIBMUDLET_ENABLE_PROFILING)
        target_compile_options(mudlet PRIVATE -pg)
        target_link_options(mudlet PRIVATE -pg)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(mudlet PRIVATE
        /W4 /WX-
        /wd4100  # unreferenced formal parameter
    )
endif()

# Link libraries
target_link_libraries(mudlet
    PRIVATE
        ${LUA_LIBRARIES}
        ${PLATFORM_LIBS}
)

# Export compile commands for clang-based tools
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Install targets
include(GNUInstallDirs)

install(TARGETS mudlet
    EXPORT libmudletTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/mudlet
)

install(EXPORT libmudletTargets
    FILE libmudletTargets.cmake
    NAMESPACE libmudlet::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmudlet
)

# Create config file
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/libmudletConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/libmudletConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmudlet
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libmudletConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libmudletConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libmudletConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libmudlet
)

# Subdirectories
if(LIBMUDLET_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(LIBMUDLET_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Print configuration summary
message(STATUS "libmudlet Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared library: ${LIBMUDLET_BUILD_SHARED}")
message(STATUS "  Build tests: ${LIBMUDLET_BUILD_TESTS}")
message(STATUS "  Build examples: ${LIBMUDLET_BUILD_EXAMPLES}")
message(STATUS "  Profiling: ${LIBMUDLET_ENABLE_PROFILING}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")