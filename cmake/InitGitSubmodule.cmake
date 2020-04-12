###########################################################################
#   Copyright (C) 2019 Florian Scheel - keneanung@gmail.com               #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################

###########################################################################
#
#  Exports a function to initialize a git
#  submodule if it is not there yet.
#
#  Usage:
#    git_submodule_init(
#      CHECK_FILE "path/to/file/to/check"
#      SUBMODULE_PATH "path/to/submodule"
#      READABLE_NAME "submodule name"
#    )
###########################################################################

find_package(Git REQUIRED QUIET)

function(git_submodule_init)

  # parse readable arguments
  set(OPTIONS "") # not used
  set(ONE_VALUE_ARGS CHECK_FILE SUBMODULE_PATH READABLE_NAME)
  set(MULTI_VALUE_ARGS "") # not used
  cmake_parse_arguments(GIT_SM "${OPTIONS}" "${ONE_VALUE_ARGS}"
                        "${MULTI_VALUE_ARGS}" ${ARGN})

  # check arguments for existence
  if(NOT GIT_SM_CHECK_FILE)
    message(
      FATAL_ERROR
        "Function git_submodule_init(): Required argument 'CHECK_FILE' missing."
    )
  endif()
  if(NOT GIT_SM_SUBMODULE_PATH)
    message(
      FATAL_ERROR
        "Function git_submodule_init(): Required argument 'SUBMODULE_PATH' missing."
    )
  endif()
  if(NOT GIT_SM_READABLE_NAME)
    message(
      FATAL_ERROR
        "Function git_submodule_init(): Required argument 'READABLE_NAME' missing."
    )
  endif()

  # actual code
  if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${GIT_SM_CHECK_FILE}")
    message(
      STATUS
        "git submodule for ${GIT_SM_READABLE_NAME} missing from source code, will attempt to get it..."
    )
    execute_process(
      COMMAND ${GIT_EXECUTABLE} submodule update --init
              "${GIT_SM_SUBMODULE_PATH}"
      TIMEOUT 30
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      RESULT_VARIABLE result
      OUTPUT_VARIABLE output_text
      ERROR_VARIABLE error_text)
    if(NOT result EQUAL "0")
      message(FATAL_ERROR ${output_text} ${error_text})
    endif()
  endif()

endfunction(git_submodule_init)
