#
# Copyright (C) 2018-2024 by George Cave - gcave@stablecoder.ca
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

# USAGE:

# The most basic way to enable sanitizers is to simply call
# `add_sanitizer_support` with the desired sanitizer names, whereupon it will
# check for the availability and compatability of the combined flags and apply
# to following compile targets:
# ~~~
# # apply address and leak sanitizers
# add_sanitizer_support(address leak)
# # future targets will be compiled with '-fsanitize=address -fsanitize=leak'
# ~~~
#
# Compile options on a per-sanitizer basis can be accomplished by calling
# `set_sanitizer_options` before with the name of the sanitizer and desired
# compile options:
# ~~~
# # set custom options that will be applies with that specific sanitizer
# set_sanitizer_options(address -fno-omit-frame-pointer)
#
# add_sanitizer_support(address leak)
# # future targets will be compiled with '-fsanitize=address -fno-omit-frame-pointer -fsanitize=leak'
# ~~~
#
# Per-sanitizer compile options can also be set by setting the named
# `SANITIZER_${SANITIZER_NAME}_OPTIONS` variable before, either in script or via
# the command line.
# ~~~
# # CMake called from command line as `cmake -S . -B build -D SANITIZER_ADDRESS_OPTION='-fno-omit-frame-pointer'`
#
# add_sanitizer_support(address)
# # future targets will be compiled with '-fsanitize=address -fno-omit-frame-pointer'
# # despite no call to `set_sanitizer_options`
# ~~~
#
# To prevent custom sanitizer options from an external source being overwritten,
# the `DEFAULT` option can be used, so that the flags are only used if none have
# been set previously:
# ~~~
# # command line has options set via command-line: `cmake -S . -B build -D SANITIZER_ADDRESS_OPTION='-fno-omit-frame-pointer'`
#
# # attempt to set custom options that will not apply since the variable already exists
# # and `DEFAULT` option is passed in.
# set_sanitizer_options(address DEFAULT -some-other-flag)
#
# add_sanitizer_support(address)
# # future targets will be compiled with '-fsanitize=address -fno-omit-frame-pointer'
# ~~~
#
# Different sets of options used with the sanitizer can be accomplished by
# defining the sanitizer serparately with the call to `set_sanitizer_option`:
# ~~~
# set_sanitizer_options(memory DEFAULT -fno-omit-frame-pointer)
#
# set_sanitizer_options(memorywithorigins DEFAULT
#                       SANITIZER memory
#                       -fno-omit-frame-pointer
#                       -fsanitize-memory-track-origins)
#
# # Despite both using the 'memory' sanitizer, which specific set of flags can be chosen
# # when calling `add_sanitizer_support` with either 'memory' or 'memorywithorigins'
# ~~~

# LEGACY USAGE:

# Previous versions had a strict set of options that could be used via having a
# set CMake variable either in the script or from the command line:
# ~~~
# # this can also be set via command line as `-D USE_SANITIZER='address leak'`
# set(USE_SANITIZER address leak)
# ~~~
# This is now deprecated to be removed in a future version, but should still be
# functional until then, either by defining `USE_SANITIZER` or
# `SANITIZER_ENABLE_LEGACY_SUPPORT` before including the script file.

include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

function(append_quoteless value)
  foreach(variable ${ARGN})
    set(${variable}
        ${${variable}} ${value}
        PARENT_SCOPE)
  endforeach(variable)
endfunction()

function(test_san_flags RETURN_VAR LINK_OPTIONS)
  set(QUIET_BACKUP ${CMAKE_REQUIRED_QUIET})
  set(CMAKE_REQUIRED_QUIET TRUE)
  unset(${RETURN_VAR} CACHE)

  # backup test flags
  set(OPTION_FLAGS_BACKUP ${CMAKE_REQUIRED_FLAGS})
  set(LINK_FLAGS_BACKUP ${CMAKE_REQUIRED_LINK_OPTIONS})

  # set link options
  unset(CMAKE_REQUIRED_LINK_OPTIONS)
  foreach(ARG ${${LINK_OPTIONS}})
    set(CMAKE_REQUIRED_LINK_OPTIONS ${CMAKE_REQUIRED_LINK_OPTIONS};${ARG})
  endforeach()

  # set compile options
  unset(CMAKE_REQUIRED_FLAGS)
  unset(test_san_flags_OPTION_TEST CACHE)
  foreach(ARG ${ARGN})
    unset(test_san_flags_OPTION_TEST CACHE)
    check_cxx_compiler_flag(${ARG} test_san_flags_OPTION_TEST)
    if(NOT test_san_flags_OPTION_TEST)
      break()
    endif()
    set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${ARG}")
  endforeach()

  # actually test if compilation can occur with given compiler/link options
  if(NOT DEFINED test_san_flags_OPTION_TEST OR test_san_flags_OPTION_TEST)
    check_cxx_source_compiles("int main() { return 0; }" ${RETURN_VAR})
  endif()

  # reset backed-up flags
  set(CMAKE_REQUIRED_LINK_OPTIONS "${LINK_FLAGS_BACKUP}")
  set(CMAKE_REQUIRED_FLAGS "${OPTION_FLAGS_BACKUP}")

  set(CMAKE_REQUIRED_QUIET "${QUIET_BACKUP}")
endfunction()

# Adds/sets compile flags for a given sanitizer, and checks for
# compatability/availability with the current compiler.
#
# Each time the compile options for a sanitizer is modified, the availability
# will be re-checked and cached.
#
# After the check, the compile options will be stored in the CMake cache as the
# `SANITIZER_${SANITIZER_NAME}_OPTIONS` name which will be available as a
# modifiable CMake string.
#
# ~~~
# Required:
# SANITIZER_NAME - Name of the sanitizer. When selected, this name also doubles
#                  as the name given to the compiler in the form of
#                  `-fsanitize=<SANITIZER_NAME>` in lower-case lettering.
#
# Optional:
# DEFAULT - Passed compile flags will only be applied if there is no currently defined
#           `SANITIZER_${SANITIZER_NAME}_OPTIONS` variable
# SANITIZER <str> - If defined, this replaces the SANITIZER_NAME for the compile/link
#                   in the form of `-fsanitize=<str>`
#
# Additional parameters are added as compile flags
function(set_sanitizer_options SANITIZER_NAME)
  # Argument parsing
  set(options DEFAULT)
  set(single_value_keywords SANITIZER)
  set(multi_value_keywords)
  cmake_parse_arguments(
    set_sanitizer_options "${options}" "${single_value_keywords}"
    "${multi_value_keywords}" ${ARGN})

  string(TOUPPER ${SANITIZER_NAME} UPPER_SANITIZER_NAME)
  string(TOLOWER ${SANITIZER_NAME} LOWER_SANITIZER_NAME)

  unset(USED_SANITIZER_OPTION)
  if(NOT set_sanitizer_options_SANITIZER)
    set(set_sanitizer_options_SANITIZER ${LOWER_SANITIZER_NAME})
  endif()

  # if `DEFAULT` is specified, only apply new arguments if there is no previous
  # cache
  if(set_sanitizer_options_DEFAULT
     AND DEFINED SANITIZER_${UPPER_SANITIZER_NAME}_OPTIONS)
    return()
  endif()

  # check if the cache does not match what we have here, update the cache and
  # check for availability
  if(NOT DEFINED SANITIZER_${UPPER_SANITIZER_NAME}_OPTIONS
     OR NOT (ARGN STREQUAL SANITIZER_${UPPER_SANITIZER_NAME}_OPTIONS))
    # don't overwrite options set via non-legacy path
    if(DEFINED SANITIZER_${UPPER_SANITIZER_NAME}_OPTIONS
       AND SANITIZER_LEGACY_SUPPORT)
      # @todo remove when legacy support is
      return()
    endif()

    # set as the new cache
    set(SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER
        ${set_sanitizer_options_SANITIZER}
        CACHE INTERNAL "" FORCE)
    set(SANITIZER_${UPPER_SANITIZER_NAME}_OPTIONS
        "${set_sanitizer_options_UNPARSED_ARGUMENTS}"
        CACHE STRING "${LOWER_SANITIZER_NAME} sanitizer compile options" FORCE)

    # check if sanitizer is available
    message(CHECK_START
            "Checking if '${LOWER_SANITIZER_NAME}' sanitizer is available")

    # check if the compile option combination can compile
    unset(SANITIZER_${UPPER_SANITIZER_NAME}_AVAILABLE CACHE)
    set(set_sanitizer_options_LINK_OPTIONS
        -fsanitize=${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER})
    test_san_flags(
      SANITIZER_${UPPER_SANITIZER_NAME}_AVAILABLE
      set_sanitizer_options_LINK_OPTIONS
      -fsanitize=${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER};${set_sanitizer_options_UNPARSED_ARGUMENTS}
    )

    if(SANITIZER_${UPPER_SANITIZER_NAME}_AVAILABLE)
      message(CHECK_PASS "available")

      # add sanitizer to available options
      if(NOT DEFINED SANITIZERS_AVAILABLE_LIST)
        set(SANITIZERS_AVAILABLE_LIST
            "${LOWER_SANITIZER_NAME}"
            CACHE INTERNAL "" FORCE)
      elseif(NOT SANITIZERS_AVAILABLE_LIST MATCHES "(${LOWER_SANITIZER_NAME})")
        set(SANITIZERS_AVAILABLE_LIST
            "${SANITIZERS_AVAILABLE_LIST};${LOWER_SANITIZER_NAME}"
            CACHE INTERNAL "" FORCE)
      endif()
    else()
      message(CHECK_FAIL "not available")

      # remove from available list if it is not available
      if(SANITIZERS_AVAILABLE_LIST MATCHES "(${LOWER_SANITIZER_NAME})")
        string(REPLACE "${LOWER_SANITIZER_NAME};" "" REPLACED_STRING
                       SANITIZERS_AVAILABLE_LIST)
        string(REPLACE ";${LOWER_SANITIZER_NAME}" "" REPLACED_STRING2
                       REPLACED_STRING)
        string(REPLACE "${LOWER_SANITIZER_NAME}" "" REPLACED_STRING3
                       REPLACED_STRING2)
        set(SANITIZERS_AVAILABLE_LIST
            "${REPLACED_STRING3}"
            CACHE INTERNAL "" FORCE)
      endif()
    endif()
  endif()
endfunction()

# Adds the given sanitizer compile options together, checks the combined
# compatability and adds the compile_options and link_options
# ~~~
# Required:
# All given parameters are either sanitizer/options set previously via
# `set_sanitizer_options` or are created/checked dynamically with no
# options, in the form of `-fsanitize=${SANITIZER_NAME}` in lower case.
function(add_sanitizer_support)
  unset(SANITIZER_COMPILE_OPTIONS_SELECTED)
  unset(SANITIZER_SELECTED_LINK_OPTIONS)

  # iterate selected sanitizers, check availability of each
  foreach(SELECTED_SANITIZER ${ARGN})
    string(TOUPPER ${SELECTED_SANITIZER} UPPER_SANITIZER_NAME)
    string(TOLOWER ${SELECTED_SANITIZER} LOWER_SANITIZER_NAME)

    # if the sanitizer is not yet known/checked, check it now
    if(NOT DEFINED SANITIZER_${UPPER_SANITIZER_NAME}_AVAILABLE)
      set_sanitizer_options(${LOWER_SANITIZER_NAME})
    endif()

    if(SANITIZER_${UPPER_SANITIZER_NAME}_AVAILABLE)
      # sanitizer is available, add the flags to the selection
      set(SANITIZER_COMPILE_OPTIONS_SELECTED
          ${SANITIZER_COMPILE_OPTIONS_SELECTED}
          -fsanitize=${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER}
          ${SANITIZER_${UPPER_SANITIZER_NAME}_OPTIONS})

      set(SANITIZER_SELECTED_LINK_OPTIONS
          ${SANITIZER_SELECTED_LINK_OPTIONS}
          -fsanitize=${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER})

      # special for AFL
      if(AFL)
        if(${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER} MATCHES "address")
          append_quoteless(AFL_USE_ASAN=1 CMAKE_C_COMPILER_LAUNCHER
                           CMAKE_CXX_COMPILER_LAUNCHER)
        elseif(${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER} MATCHES "leak")
          append_quoteless(AFL_USE_LSAN=1 CMAKE_C_COMPILER_LAUNCHER
                           CMAKE_CXX_COMPILER_LAUNCHER)
        elseif(${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER} MATCHES "memory")
          append_quoteless(AFL_USE_MSAN=1 CMAKE_C_COMPILER_LAUNCHER
                           CMAKE_CXX_COMPILER_LAUNCHER)
        elseif(${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER} MATCHES
               "undefined")
          append_quoteless(AFL_USE_UBSAN=1 CMAKE_C_COMPILER_LAUNCHER
                           CMAKE_CXX_COMPILER_LAUNCHER)
        elseif(${SANITIZER_${UPPER_SANITIZER_NAME}_SANITIZER} MATCHES "cfi")
          append_quoteless(AFL_USE_CFISAN=1 CMAKE_C_COMPILER_LAUNCHER
                           CMAKE_CXX_COMPILER_LAUNCHER)
        endif()
      endif()
    else()
      message(
        SEND_ERROR "'${LOWER_SANITIZER_NAME}' sanitizer set not available")
    endif()
  endforeach()

  # check if all selected sanitizer options/flags are compatible together
  if(NOT DEFINED SANITIZER_COMPILE_OPTIONS_GLOBAL_CACHE
     OR NOT SANITIZER_COMPILE_OPTIONS_SELECTED STREQUAL
        SANITIZER_COMPILE_OPTIONS_GLOBAL_CACHE)
    # set of flags needs to be tested for compatability
    unset(SANITIZER_SELECTED_OPTIONS_AVAILABLE CACHE)
    test_san_flags(
      SANITIZER_SELECTED_OPTIONS_AVAILABLE SANITIZER_SELECTED_LINK_OPTIONS
      ${SANITIZER_COMPILE_OPTIONS_SELECTED})

    # whatever the result, cache it to reduce repeating test
    set(SANITIZER_COMPILE_OPTIONS_GLOBAL_CACHE
        ${SANITIZER_COMPILE_OPTIONS_SELECTED}
        CACHE INTERNAL "")
  endif()

  if(SANITIZER_SELECTED_OPTIONS_AVAILABLE)
    # sanitizer selection is compatible, apply it
    add_compile_options(${SANITIZER_COMPILE_OPTIONS_SELECTED})
    add_link_options(${SANITIZER_SELECTED_LINK_OPTIONS})
  else()
    message(FATAL_ERROR "Selected sanitizer options not compatible: ${ARGN}")
  endif()
endfunction()

if(SANITIZER_ENABLE_LEGACY_SUPPORT OR USE_SANITIZER)
  set(SANITIZER_LEGACY_SUPPORT ON)

  # The older variants used to add this flag, but since MSVC doesn't support it,
  # do a check and add it only if available
  set(QUIET_BACKUP ${CMAKE_REQUIRED_QUIET})
  set(CMAKE_REQUIRED_QUIET TRUE)
  unset(SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS)
  check_cxx_compiler_flag(-fno-omit-frame-pointer
                          SANITIZER_OMIT_FRAME_POINTER_AVAILABLE)
  set(CMAKE_REQUIRED_QUIET "${QUIET_BACKUP}")
  if(SANITIZER_OMIT_FRAME_POINTER_AVAILABLE)
    set(SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS -fno-omit-frame-pointer)
  endif()

  set_sanitizer_options(address DEFAULT
                        ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS})
  set_sanitizer_options(leak DEFAULT ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS})
  set_sanitizer_options(memory DEFAULT
                        ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS})
  set_sanitizer_options(
    memorywithorigins DEFAULT SANITIZER memory
    ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS} -fsanitize-memory-track-origins)
  set_sanitizer_options(undefined DEFAULT
                        ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS})
  set_sanitizer_options(thread DEFAULT
                        ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS})
  set_sanitizer_options(cfi DEFAULT ${SANITIZER_LEGACY_DEFAULT_COMMON_OPTIONS})

  set(USE_SANITIZER
      ""
      CACHE
        STRING
        "(DEPRECATED) Compile with sanitizers. Available sanitizers are: ${SANITIZERS_AVAILABLE_LIST}"
  )

  if(USE_SANITIZER)
    add_sanitizer_support(${USE_SANITIZER})
  endif()

  unset(SANITIZER_LEGACY_SUPPORT)
endif()
