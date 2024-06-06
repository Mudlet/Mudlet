
if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 MultimediaQuick module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt5MultimediaQuick_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt5MultimediaQuick_VERSION instead.
set(Qt5MultimediaQuick_VERSION_STRING 5.14.1)

set(Qt5MultimediaQuick_LIBRARIES Qt5::MultimediaQuick)

macro(_qt5_MultimediaQuick_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::MultimediaQuick\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    endif()
endmacro()


macro(_populate_MultimediaQuick_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::MultimediaQuick APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt5MultimediaQuick_install_prefix}/lib/${LIB_LOCATION}")
    _qt5_MultimediaQuick_check_file_exists(${imported_location})
    set(_deps
        ${_Qt5MultimediaQuick_LIB_DEPENDENCIES}
    )
    set(_static_deps
    )

    set_target_properties(Qt5::MultimediaQuick PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        "IMPORTED_SONAME_${Configuration}" "libQt5MultimediaQuick.so.5"
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::MultimediaQuick APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )


endmacro()

if (NOT TARGET Qt5::MultimediaQuick)

    set(_Qt5MultimediaQuick_OWN_INCLUDE_DIRS "${_qt5MultimediaQuick_install_prefix}/include/" "${_qt5MultimediaQuick_install_prefix}/include/QtMultimediaQuick")
    set(Qt5MultimediaQuick_PRIVATE_INCLUDE_DIRS
        "${_qt5MultimediaQuick_install_prefix}/include/QtMultimediaQuick/5.14.1"
        "${_qt5MultimediaQuick_install_prefix}/include/QtMultimediaQuick/5.14.1/QtMultimediaQuick"
    )

    foreach(_dir ${_Qt5MultimediaQuick_OWN_INCLUDE_DIRS})
        _qt5_MultimediaQuick_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt5MultimediaQuick_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt5MultimediaQuick_PRIVATE_INCLUDE_DIRS})
            _qt5_MultimediaQuick_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt5MultimediaQuick_INCLUDE_DIRS ${_Qt5MultimediaQuick_OWN_INCLUDE_DIRS})

    set(Qt5MultimediaQuick_DEFINITIONS -DQT_QTMULTIMEDIAQUICKTOOLS_LIB)
    set(Qt5MultimediaQuick_COMPILE_DEFINITIONS QT_QTMULTIMEDIAQUICKTOOLS_LIB)
    set(_Qt5MultimediaQuick_MODULE_DEPENDENCIES "Multimedia;Quick;Core")


    set(Qt5MultimediaQuick_OWN_PRIVATE_INCLUDE_DIRS ${Qt5MultimediaQuick_PRIVATE_INCLUDE_DIRS})

    set(_Qt5MultimediaQuick_FIND_DEPENDENCIES_REQUIRED)
    if (Qt5MultimediaQuick_FIND_REQUIRED)
        set(_Qt5MultimediaQuick_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt5MultimediaQuick_FIND_DEPENDENCIES_QUIET)
    if (Qt5MultimediaQuick_FIND_QUIETLY)
        set(_Qt5MultimediaQuick_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt5MultimediaQuick_FIND_VERSION_EXACT)
    if (Qt5MultimediaQuick_FIND_VERSION_EXACT)
        set(_Qt5MultimediaQuick_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt5MultimediaQuick_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt5MultimediaQuick_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.14.1 ${_Qt5MultimediaQuick_FIND_VERSION_EXACT}
                ${_Qt5MultimediaQuick_DEPENDENCIES_FIND_QUIET}
                ${_Qt5MultimediaQuick_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt5MultimediaQuick_FOUND False)
            return()
        endif()

        list(APPEND Qt5MultimediaQuick_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt5MultimediaQuick_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt5MultimediaQuick_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt5MultimediaQuick_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt5MultimediaQuick_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt5MultimediaQuick_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5MultimediaQuick_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5MultimediaQuick_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5MultimediaQuick_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5MultimediaQuick_EXECUTABLE_COMPILE_FLAGS)

    # It can happen that the same FooConfig.cmake file is included when calling find_package()
    # on some Qt component. An example of that is when using a Qt static build with auto inclusion
    # of plugins:
    #
    # Qt5WidgetsConfig.cmake -> Qt5GuiConfig.cmake -> Qt5Gui_QSvgIconPlugin.cmake ->
    # Qt5SvgConfig.cmake -> Qt5WidgetsConfig.cmake ->
    # finish processing of second Qt5WidgetsConfig.cmake ->
    # return to first Qt5WidgetsConfig.cmake ->
    # add_library cannot create imported target Qt5::Widgets.
    #
    # Make sure to return early in the original Config inclusion, because the target has already
    # been defined as part of the second inclusion.
    if(TARGET Qt5::MultimediaQuick)
        return()
    endif()

    set(_Qt5MultimediaQuick_LIB_DEPENDENCIES "Qt5::Multimedia;Qt5::Quick;Qt5::Core")


    add_library(Qt5::MultimediaQuick SHARED IMPORTED)

    set_property(TARGET Qt5::MultimediaQuick PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt5MultimediaQuick_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::MultimediaQuick PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_QTMULTIMEDIAQUICKTOOLS_LIB)

    set_property(TARGET Qt5::MultimediaQuick PROPERTY INTERFACE_QT_ENABLED_FEATURES )
    set_property(TARGET Qt5::MultimediaQuick PROPERTY INTERFACE_QT_DISABLED_FEATURES )

    set_property(TARGET Qt5::MultimediaQuick PROPERTY INTERFACE_QT_PLUGIN_TYPES "")

    set(_Qt5MultimediaQuick_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt5MultimediaQuick_PRIVATE_DIR ${Qt5MultimediaQuick_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt5MultimediaQuick_PRIVATE_DIR})
            set(_Qt5MultimediaQuick_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt5MultimediaQuick_PRIVATE_DIRS_EXIST)
        add_library(Qt5::MultimediaQuickPrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::MultimediaQuickPrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt5MultimediaQuick_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt5MultimediaQuick_PRIVATEDEPS)
        foreach(dep ${_Qt5MultimediaQuick_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt5MultimediaQuick_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::MultimediaQuickPrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::MultimediaQuick ${_Qt5MultimediaQuick_PRIVATEDEPS}
        )
    endif()

    _populate_MultimediaQuick_target_properties(RELEASE "libQt5MultimediaQuick.so.5.14.1" "" FALSE)








_qt5_MultimediaQuick_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt5MultimediaQuickConfigVersion.cmake")

endif()
