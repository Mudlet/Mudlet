find_path(
  SPARKLE_INCLUDE_DIR
  NAMES Sparkle/Sparkle.h
  PATHS ${CMAKE_SOURCE_DIR}/3rdparty/cocoapods/Pods/Sparkle)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set SPARKLE_FOUND to TRUE if all
# listed variables are TRUE
find_package_handle_standard_args(SPARKLE REQUIRED_VARS SPARKLE_INCLUDE_DIR)

mark_as_advanced(SPARKLE_INCLUDE_DIR)

if(SPARKLE_FOUND AND NOT TARGET Sparkle::Sparkle)
  add_library(Sparkle::Sparkle SHARED IMPORTED)
  set_target_properties(
    Sparkle::Sparkle
    PROPERTIES MACOSX_RPATH OFF FRAMEWORK ON IMPORTED_LOCATION
                                             ${SPARKLE_INCLUDE_DIR}/Sparkle
               INTERFACE_INCLUDE_DIRECTORIES ${SPARKLE_INCLUDE_DIR})
endif()
