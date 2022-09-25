# - Config file for the QtKeychain package
# It defines the following variables
#  QTKEYCHAIN_INCLUDE_DIRS - include directories for QtKeychain
#  QTKEYCHAIN_LIBRARIES    - libraries to link against
# as well as the following imported targets
#  qt5keychain / qt6keychain
#  Qt5Keychain::Qt5Keychain / Qt6Keychain::Qt6Keychain


####### Expanded from @PACKAGE_INIT@ by configure_package_config_file() #######
####### Any changes to this file will be overwritten by the next CMake run ####
####### The input file was QtKeychainConfig.cmake.in                            ########

get_filename_component(PACKAGE_PREFIX_DIR "${CMAKE_CURRENT_LIST_DIR}/../" ABSOLUTE)

macro(set_and_check _var _file)
  set(${_var} "${_file}")
  if(NOT EXISTS "${_file}")
    message(FATAL_ERROR "File or directory ${_file} referenced by variable ${_var} does not exist !")
  endif()
endmacro()

macro(check_required_components _NAME)
  foreach(comp ${${_NAME}_FIND_COMPONENTS})
    if(NOT ${_NAME}_${comp}_FOUND)
      if(${_NAME}_FIND_REQUIRED_${comp})
        set(${_NAME}_FOUND FALSE)
      endif()
    endif()
  endforeach()
endmacro()

####################################################################################

include("${CMAKE_CURRENT_LIST_DIR}/Qt5KeychainLibraryDepends.cmake")

include(CMakeFindDependencyMacro)

find_dependency(Qt5Core)

if(UNIX AND NOT APPLE AND NOT ANDROID)
    find_dependency(Qt5DBus)
endif()

set(QTKEYCHAIN_LIBRARIES "qt5keychain")
get_target_property(QTKEYCHAIN_INCLUDE_DIRS "qt5keychain" INTERFACE_INCLUDE_DIRECTORIES)

add_library(Qt5Keychain::Qt5Keychain ALIAS qt5keychain)
