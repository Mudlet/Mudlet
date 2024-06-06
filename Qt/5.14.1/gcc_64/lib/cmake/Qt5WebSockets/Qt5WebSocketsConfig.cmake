
if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 WebSockets module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt5WebSockets_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt5WebSockets_VERSION instead.
set(Qt5WebSockets_VERSION_STRING 5.14.1)

set(Qt5WebSockets_LIBRARIES Qt5::WebSockets)

macro(_qt5_WebSockets_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::WebSockets\" references the file
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


macro(_populate_WebSockets_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::WebSockets APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt5WebSockets_install_prefix}/lib/${LIB_LOCATION}")
    _qt5_WebSockets_check_file_exists(${imported_location})
    set(_deps
        ${_Qt5WebSockets_LIB_DEPENDENCIES}
    )
    set(_static_deps
    )

    set_target_properties(Qt5::WebSockets PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        "IMPORTED_SONAME_${Configuration}" "libQt5WebSockets.so.5"
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::WebSockets APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )


endmacro()

if (NOT TARGET Qt5::WebSockets)

    set(_Qt5WebSockets_OWN_INCLUDE_DIRS "${_qt5WebSockets_install_prefix}/include/" "${_qt5WebSockets_install_prefix}/include/QtWebSockets")
    set(Qt5WebSockets_PRIVATE_INCLUDE_DIRS
        "${_qt5WebSockets_install_prefix}/include/QtWebSockets/5.14.1"
        "${_qt5WebSockets_install_prefix}/include/QtWebSockets/5.14.1/QtWebSockets"
    )

    foreach(_dir ${_Qt5WebSockets_OWN_INCLUDE_DIRS})
        _qt5_WebSockets_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt5WebSockets_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt5WebSockets_PRIVATE_INCLUDE_DIRS})
            _qt5_WebSockets_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt5WebSockets_INCLUDE_DIRS ${_Qt5WebSockets_OWN_INCLUDE_DIRS})

    set(Qt5WebSockets_DEFINITIONS -DQT_WEBSOCKETS_LIB)
    set(Qt5WebSockets_COMPILE_DEFINITIONS QT_WEBSOCKETS_LIB)
    set(_Qt5WebSockets_MODULE_DEPENDENCIES "Network;Core")


    set(Qt5WebSockets_OWN_PRIVATE_INCLUDE_DIRS ${Qt5WebSockets_PRIVATE_INCLUDE_DIRS})

    set(_Qt5WebSockets_FIND_DEPENDENCIES_REQUIRED)
    if (Qt5WebSockets_FIND_REQUIRED)
        set(_Qt5WebSockets_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt5WebSockets_FIND_DEPENDENCIES_QUIET)
    if (Qt5WebSockets_FIND_QUIETLY)
        set(_Qt5WebSockets_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt5WebSockets_FIND_VERSION_EXACT)
    if (Qt5WebSockets_FIND_VERSION_EXACT)
        set(_Qt5WebSockets_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt5WebSockets_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt5WebSockets_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.14.1 ${_Qt5WebSockets_FIND_VERSION_EXACT}
                ${_Qt5WebSockets_DEPENDENCIES_FIND_QUIET}
                ${_Qt5WebSockets_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt5WebSockets_FOUND False)
            return()
        endif()

        list(APPEND Qt5WebSockets_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt5WebSockets_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt5WebSockets_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt5WebSockets_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt5WebSockets_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt5WebSockets_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5WebSockets_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5WebSockets_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5WebSockets_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5WebSockets_EXECUTABLE_COMPILE_FLAGS)

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
    if(TARGET Qt5::WebSockets)
        return()
    endif()

    set(_Qt5WebSockets_LIB_DEPENDENCIES "Qt5::Network;Qt5::Core")


    add_library(Qt5::WebSockets SHARED IMPORTED)

    set_property(TARGET Qt5::WebSockets PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt5WebSockets_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::WebSockets PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_WEBSOCKETS_LIB)

    set_property(TARGET Qt5::WebSockets PROPERTY INTERFACE_QT_ENABLED_FEATURES )
    set_property(TARGET Qt5::WebSockets PROPERTY INTERFACE_QT_DISABLED_FEATURES )

    set_property(TARGET Qt5::WebSockets PROPERTY INTERFACE_QT_PLUGIN_TYPES "")

    set(_Qt5WebSockets_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt5WebSockets_PRIVATE_DIR ${Qt5WebSockets_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt5WebSockets_PRIVATE_DIR})
            set(_Qt5WebSockets_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt5WebSockets_PRIVATE_DIRS_EXIST)
        add_library(Qt5::WebSocketsPrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::WebSocketsPrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt5WebSockets_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt5WebSockets_PRIVATEDEPS)
        foreach(dep ${_Qt5WebSockets_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt5WebSockets_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::WebSocketsPrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::WebSockets ${_Qt5WebSockets_PRIVATEDEPS}
        )
    endif()

    _populate_WebSockets_target_properties(RELEASE "libQt5WebSockets.so.5.14.1" "" FALSE)





    file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt5WebSockets_*Plugin.cmake")

    macro(_populate_WebSockets_plugin_properties Plugin Configuration PLUGIN_LOCATION
          IsDebugAndRelease)
        set_property(TARGET Qt5::${Plugin} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

        set(imported_location "${_qt5WebSockets_install_prefix}/plugins/${PLUGIN_LOCATION}")
        _qt5_WebSockets_check_file_exists(${imported_location})
        set_target_properties(Qt5::${Plugin} PROPERTIES
            "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        )

    endmacro()

    if (pluginTargets)
        foreach(pluginTarget ${pluginTargets})
            include(${pluginTarget})
        endforeach()
    endif()





_qt5_WebSockets_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt5WebSocketsConfigVersion.cmake")

endif()
