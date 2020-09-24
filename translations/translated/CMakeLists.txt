############################################################################
#    Copyright (C) 2018 by Vadim Peretokin - vperetokin@gmail.com          #
#    Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         #
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

find_package(Qt5LinguistTools REQUIRED)
get_target_property(LRELEASE_LOCATION Qt5::lrelease LOCATION)

file(GLOB TS_FILES *.ts)
string(REPLACE ".ts" "" TS_FILES_NOEXT "${TS_FILES}")

file(REMOVE "${CMAKE_CURRENT_SOURCE_DIR}/lrelease_output.txt")
if(NOT WIN32)
  set(OLDLANG $ENV{LANG})
#  message(STATUS "The environmental variable LANG was set to: $ENV{LANG}")
  set(ENV{LANG} "C")
#  message(STATUS "The environmental variable LANG is now set to: $ENV{LANG}")
endif()

foreach(ts_file ${TS_FILES_NOEXT})
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" qm_file ${ts_file})
  execute_process(
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMAND ${LRELEASE_LOCATION} ${ts_file}.ts -compress -qm ${qm_file}.qm
          OUTPUT_VARIABLE "lrelease_output")
#  message(STATUS ${lrelease_output})
  file(APPEND "${CMAKE_CURRENT_SOURCE_DIR}/lrelease_output.txt" ${lrelease_output})
endforeach()

if(NOT WIN32)
  set(ENV{LANG} ${OLDLANG})
#  message(STATUS "The environmental variable LANG should now be reset to: ${OLDLANG} and it is now: $ENV{LANG}")
endif()

find_program(LUA_LOCATION lua5.1)
if(NOT LUA_LOCATION)
  find_program(LUA_LOCATION lua)
endif()

execute_process(
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ${LUA_LOCATION} ${CMAKE_CURRENT_SOURCE_DIR}/generate-translation-stats.lua)

