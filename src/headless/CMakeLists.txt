############################################################################
#    Copyright (C) 2026 by Mudlet Developers                               #
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

# What a frontend with no window builds on: mudlet_core and nothing that needs Qt Widgets, a terminal or
# a process of its own, so that a phone app can link it as the terminal runner below does.
add_library(mudlet_headless STATIC
    THeadlessSession.cpp
    THeadlessSession.h
)
set_target_properties(mudlet_headless PROPERTIES AUTOMOC ON POSITION_INDEPENDENT_CODE ON)
target_include_directories(mudlet_headless INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mudlet_headless PUBLIC ${LIB_MUDLET_TARGET})

if(WIN32)
  set(HEADLESS_APP_TARGET mudlet-headless.exe)
else()
  set(HEADLESS_APP_TARGET mudlet-headless)
endif()

# A console program on Windows too, unlike mudlet_executable, so that its standard streams work
add_executable(mudlet_headless_executable main.cpp)
target_compile_definitions(mudlet_headless_executable PRIVATE APP_TARGET="${HEADLESS_APP_TARGET}")
target_link_libraries(mudlet_headless_executable mudlet_headless mudlet_lsan_hooks)
set_target_properties(mudlet_headless_executable PROPERTIES
  OUTPUT_NAME mudlet-headless
  # Beside the GUI's binary, which on Windows has the Qt libraries deployed next to it
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.."
  # Lua C modules loaded at runtime resolve the Lua API from the executable, as for mudlet_executable
  ENABLE_EXPORTS ON)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # A distribution's qt6keychain (as on CI) needs Qt6DBus but has no search path; unlike a RUNPATH, an
  # RPATH finds it beside our Qt. The GUI loads it through Qt Widgets, first on its link line.
  target_link_options(mudlet_headless_executable PRIVATE "LINKER:--disable-new-dtags")
endif()
if(WIN32)
  target_link_libraries(mudlet_headless_executable ws2_32)
  if(MINGW)
    # See mudlet_executable: Qt's WinMain library can drop off the link line, and main() is called directly
    set_target_properties(mudlet_headless_executable PROPERTIES qt_no_entrypoint ON)
  endif()
endif()
