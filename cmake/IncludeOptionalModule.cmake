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
#  Exports a macro to check, whether optional modules were disabled
#  via environment variables.
#
#  Usage:
#    include_optional_module(
#      ENVIRONMENT_VARIABLE ENVIRONMENT_VARIABLE
#      OPTION_VARIABLE OPTION_VARIABLE
#      READABLE_NAME "option name"
#      [ SUPPORTED_SYSTEMS "List" "Of" "Systems"]
#    )
###########################################################################

macro(include_optional_module)

  # parse readable arguments
  set(OPTIONAL_MODULE_OPTIONS "") # not used
  set(OPTIONAL_MODULE_ONE_VALUE_ARGS ENVIRONMENT_VARIABLE OPTION_VARIABLE
                                     READABLE_NAME)
  set(OPTIONAL_MODULE_MULTI_VALUE_ARGS SUPPORTED_SYSTEMS)
  cmake_parse_arguments(
    OPTIONAL_MODULE "${OPTIONAL_MODULE_OPTIONS}"
    "${OPTIONAL_MODULE_ONE_VALUE_ARGS}" "${OPTIONAL_MODULE_MULTI_VALUE_ARGS}"
    ${ARGN})

  # check arguments for existence
  if(NOT OPTIONAL_MODULE_ENVIRONMENT_VARIABLE)
    message(
      FATAL_ERROR
        "Macro include_optional_module(): Required argument 'ENVIRONMENT_VARIABLE' missing."
    )
  endif()
  if(NOT OPTIONAL_MODULE_OPTION_VARIABLE)
    message(
      FATAL_ERROR
        "Macro include_optional_module(): Required argument 'OPTION_VARIABLE' missing."
    )
  endif()
  if(NOT OPTIONAL_MODULE_READABLE_NAME)
    message(
      FATAL_ERROR
        "Macro include_optional_module(): Required argument 'READABLE_NAME' missing."
    )
  endif()

  set(OPTIONAL_MODULE_TEST $ENV{${OPTIONAL_MODULE_ENVIRONMENT_VARIABLE}})
  if((NOT OPTIONAL_MODULE_SUPPORTED_SYSTEMS)
     OR (CMAKE_SYSTEM_NAME IN_LIST OPTIONAL_MODULE_SUPPORTED_SYSTEMS))
    if(DEFINED OPTIONAL_MODULE_TEST)
      string(TOUPPER ${OPTIONAL_MODULE_TEST} OPTIONAL_MODULE_TEST)
      if(OPTIONAL_MODULE_TEST STREQUAL "NO")
        # The specific tested for value was seen so set the option "no don't
        # include the module"
        set(OPTIONAL_MODULE_OPTION_VALUE OFF)
        message(
          STATUS
            "Excluding optional ${OPTIONAL_MODULE_READABLE_NAME} explicitly"
        )
      else()
        # Any other value was seen so ignore it and set "yes, include the
        # module"
        set(OPTIONAL_MODULE_OPTION_VALUE ON)
        message(
          STATUS
            "Including optional ${OPTIONAL_MODULE_READABLE_NAME} explicitly"
        )
      endif()
    else()
      # An environmental variable not detected, apply platform default of "yes,
      # include the module"
      set(OPTIONAL_MODULE_OPTION_VALUE ON)
      message(
        STATUS "Including optional ${OPTIONAL_MODULE_READABLE_NAME}")
    endif()
    option(${OPTIONAL_MODULE_OPTION_VARIABLE}
           "Include optional ${OPTIONAL_MODULE_READABLE_NAME}"
           ${OPTIONAL_MODULE_OPTION_VALUE})
  else()
    # Don't offer option to enable the module since it's not supported on this
    # platform
    set(${OPTIONAL_MODULE_OPTION_VARIABLE} OFF)
    message(
      STATUS
        "Excluding optional ${OPTIONAL_MODULE_READABLE_NAME} as it is not supported on this platform"
    )
  endif()

endmacro(include_optional_module)
