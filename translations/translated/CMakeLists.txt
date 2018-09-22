############################################################################
#    Copyright (C) 2018 by Vadim Peretokin - vperetokin@gmail.com          #
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

file(REMOVE "${CMAKE_CURRENT_BINARY_DIR}/lrelease_output.txt")
foreach(ts_file ${TS_FILES_NOEXT})
  string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" qm_file ${ts_file})
  execute_process(
          WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
          COMMAND ${LRELEASE_LOCATION} ${ts_file}.ts -compress -qm ${qm_file}.qm
          OUTPUT_VARIABLE "lrelease_output")
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/lrelease_output.txt" ${lrelease_output})
endforeach()

find_program(LUA_LOCATION lua5.1)
if(NOT LUA_LOCATION)
  find_program(LUA_LOCATION lua)
endif()

execute_process(
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMAND ${LUA_LOCATION} ${CMAKE_CURRENT_SOURCE_DIR}/generate-translation-stats.lua)

#

#file(GLOB QBT_TS_FILES ../translations/translated/*.ts)
#get_filename_component(QBT_QM_FILES_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../translations/translated" ABSOLUTE)
#set_source_files_properties(${QBT_TS_FILES} PROPERTIES OUTPUT_LOCATION "${QBT_QM_FILES_BINARY_DIR}")
#
#find_package(Qt5 COMPONENTS LinguistTools REQUIRED)
#qt5_add_translation(QBT_QM_FILES ${QBT_TS_FILES})
#
#get_filename_component(_lang_qrc_src "${CMAKE_CURRENT_SOURCE_DIR}/../translations/translated/qm.qrc" ABSOLUTE)
#get_filename_component(_lang_qrc_dst "${CMAKE_CURRENT_BINARY_DIR}/../translations/translated/qm.qrc" ABSOLUTE)
#get_filename_component(_lang_qrc_dst_dir "${CMAKE_CURRENT_BINARY_DIR}/../translations/translated" ABSOLUTE)
#
#file(COPY ${_lang_qrc_src} DESTINATION ${_lang_qrc_dst_dir})
#
#set_source_files_properties("${_lang_qrc_dst}" PROPERTIES GENERATED True)
#foreach(qm_file ${QBT_QM_FILES})
#  set_source_files_properties("${_lang_qrc_dst}" PROPERTIES OBJECT_DEPENDS ${qm_file})
#endforeach()
#
#QT5_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
#QT5_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
#if(USE_FONTS)
#    QT5_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS} ${mudlet_fonts_RCCS} "${_lang_qrc_dst}")
#else()
#    QT5_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS} "${_lang_qrc_dst}")
#endif()
