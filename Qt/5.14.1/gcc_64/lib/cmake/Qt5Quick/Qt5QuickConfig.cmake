
if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 Quick module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt5Quick_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt5Quick_VERSION instead.
set(Qt5Quick_VERSION_STRING 5.14.1)

set(Qt5Quick_LIBRARIES Qt5::Quick)

macro(_qt5_Quick_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::Quick\" references the file
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


macro(_populate_Quick_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::Quick APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt5Quick_install_prefix}/lib/${LIB_LOCATION}")
    _qt5_Quick_check_file_exists(${imported_location})
    set(_deps
        ${_Qt5Quick_LIB_DEPENDENCIES}
    )
    set(_static_deps
    )

    set_target_properties(Qt5::Quick PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        "IMPORTED_SONAME_${Configuration}" "libQt5Quick.so.5"
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::Quick APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )


endmacro()

if (NOT TARGET Qt5::Quick)

    set(_Qt5Quick_OWN_INCLUDE_DIRS "${_qt5Quick_install_prefix}/include/" "${_qt5Quick_install_prefix}/include/QtQuick")
    set(Qt5Quick_PRIVATE_INCLUDE_DIRS
        "${_qt5Quick_install_prefix}/include/QtQuick/5.14.1"
        "${_qt5Quick_install_prefix}/include/QtQuick/5.14.1/QtQuick"
    )

    foreach(_dir ${_Qt5Quick_OWN_INCLUDE_DIRS})
        _qt5_Quick_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt5Quick_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt5Quick_PRIVATE_INCLUDE_DIRS})
            _qt5_Quick_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt5Quick_INCLUDE_DIRS ${_Qt5Quick_OWN_INCLUDE_DIRS})

    set(Qt5Quick_DEFINITIONS -DQT_QUICK_LIB)
    set(Qt5Quick_COMPILE_DEFINITIONS QT_QUICK_LIB)
    set(_Qt5Quick_MODULE_DEPENDENCIES "QmlModels;Qml;Gui;Core")


    set(Qt5Quick_OWN_PRIVATE_INCLUDE_DIRS ${Qt5Quick_PRIVATE_INCLUDE_DIRS})

    set(_Qt5Quick_FIND_DEPENDENCIES_REQUIRED)
    if (Qt5Quick_FIND_REQUIRED)
        set(_Qt5Quick_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt5Quick_FIND_DEPENDENCIES_QUIET)
    if (Qt5Quick_FIND_QUIETLY)
        set(_Qt5Quick_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt5Quick_FIND_VERSION_EXACT)
    if (Qt5Quick_FIND_VERSION_EXACT)
        set(_Qt5Quick_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt5Quick_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt5Quick_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.14.1 ${_Qt5Quick_FIND_VERSION_EXACT}
                ${_Qt5Quick_DEPENDENCIES_FIND_QUIET}
                ${_Qt5Quick_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt5Quick_FOUND False)
            return()
        endif()

        list(APPEND Qt5Quick_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt5Quick_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt5Quick_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt5Quick_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt5Quick_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt5Quick_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5Quick_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5Quick_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5Quick_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5Quick_EXECUTABLE_COMPILE_FLAGS)

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
    if(TARGET Qt5::Quick)
        return()
    endif()

    set(_Qt5Quick_LIB_DEPENDENCIES "Qt5::QmlModels;Qt5::Qml;Qt5::Gui;Qt5::Core")


    add_library(Qt5::Quick SHARED IMPORTED)

    set_property(TARGET Qt5::Quick PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt5Quick_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::Quick PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_QUICK_LIB)

    set_property(TARGET Qt5::Quick PROPERTY INTERFACE_QT_ENABLED_FEATURES quick-draganddrop)
    set_property(TARGET Qt5::Quick PROPERTY INTERFACE_QT_DISABLED_FEATURES d3d12)

    set_property(TARGET Qt5::Quick PROPERTY INTERFACE_QT_PLUGIN_TYPES "scenegraph")

    set(_Qt5Quick_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt5Quick_PRIVATE_DIR ${Qt5Quick_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt5Quick_PRIVATE_DIR})
            set(_Qt5Quick_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt5Quick_PRIVATE_DIRS_EXIST)
        add_library(Qt5::QuickPrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::QuickPrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt5Quick_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt5Quick_PRIVATEDEPS)
        foreach(dep ${_Qt5Quick_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt5Quick_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::QuickPrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::Quick ${_Qt5Quick_PRIVATEDEPS}
        )
    endif()

    _populate_Quick_target_properties(RELEASE "libQt5Quick.so.5.14.1" "" FALSE)





    file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt5Quick_*Plugin.cmake")

    macro(_populate_Quick_plugin_properties Plugin Configuration PLUGIN_LOCATION
          IsDebugAndRelease)
        set_property(TARGET Qt5::${Plugin} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

        set(imported_location "${_qt5Quick_install_prefix}/plugins/${PLUGIN_LOCATION}")
        _qt5_Quick_check_file_exists(${imported_location})
        set_target_properties(Qt5::${Plugin} PROPERTIES
            "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        )

    endmacro()

    if (pluginTargets)
        foreach(pluginTarget ${pluginTargets})
            include(${pluginTarget})
        endforeach()
    endif()





_qt5_Quick_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt5QuickConfigVersion.cmake")

endif()
