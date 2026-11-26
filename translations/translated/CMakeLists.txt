############################################################################
#    Copyright (C) 2018 by Vadim Peretokin - vperetokin@gmail.com          #
#    Copyright (C) 2020, 2023, 2025 by Stephen Lyons                       #
#                                                - slysven@virginmedia.com #
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

# translations, credit to qBittorrent

message(STATUS "Building translations")

# In a cross build only this form looks for the host tools under QT_HOST_PATH
find_package(Qt6 6.8.2 REQUIRED COMPONENTS LinguistTools)
get_target_property(LRELEASE_LOCATION Qt6::lrelease LOCATION)

file(GLOB TS_FILES *.ts)
string(REPLACE ".ts" "" TS_FILES_NOEXT "${TS_FILES}")

file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/lrelease_output.txt")

foreach(ts_file ${TS_FILES_NOEXT})
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" qm_file ${ts_file})
  execute_process(
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMAND ${LRELEASE_LOCATION} ${ts_file}.ts -compress -qm ${qm_file}.qm
          OUTPUT_VARIABLE "lrelease_output")
#  message(STATUS ${lrelease_output})
  file(APPEND "${CMAKE_CURRENT_SOURCE_DIR}/lrelease_output.txt" ${lrelease_output})
endforeach()

find_program(LUA_LOCATION lua-5.1)
if(NOT LUA_LOCATION)
  find_program(LUA_LOCATION lua5.1)
  if(NOT LUA_LOCATION)
    find_program(LUA_LOCATION lua51)
    if(NOT LUA_LOCATION)
      find_program(LUA_LOCATION lua)
    endif()
  endif()
endif()

set(translation_stats_file "${CMAKE_CURRENT_SOURCE_DIR}/translation-stats.json")

if(LUA_LOCATION)
  # The generator's output is deliberately not captured: its own warning about a
  # missing lua-yajl is what explains empty statistics and English-only detection.
  execute_process(
          WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
          COMMAND ${LUA_LOCATION} ${CMAKE_CURRENT_SOURCE_DIR}/generate-translation-stats.lua
          RESULT_VARIABLE stats_result)
  if(NOT stats_result EQUAL 0)
    message(WARNING
          "Generating the translation statistics failed with exit code ${stats_result}. "
          "See the generator output above.")
  endif()
endif()

# mudlet.qrc embeds the statistics, so the compile cannot start without the file.
# With no interpreter the generator cannot run at all: write the same empty
# statistics the script itself falls back to when it has no lua-yajl, instead of
# letting configure succeed and the build fail later on a file no rule makes.
if(NOT EXISTS "${translation_stats_file}")
  if(LUA_LOCATION)
    message(FATAL_ERROR
          "The translation statistics generator ran but left no ${translation_stats_file}, "
          "which mudlet.qrc needs. See its output above.")
  endif()
  message(WARNING
          "No Lua 5.1 interpreter was found, so the translation statistics will be empty. "
          "Install Lua 5.1, or pass -DLUA_LOCATION=<interpreter>, to get them.")
  file(WRITE "${translation_stats_file}" "{}")
endif()

