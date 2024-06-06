
if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 SerialBus module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt5SerialBus_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt5SerialBus_VERSION instead.
set(Qt5SerialBus_VERSION_STRING 5.14.1)

set(Qt5SerialBus_LIBRARIES Qt5::SerialBus)

macro(_qt5_SerialBus_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::SerialBus\" references the file
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


macro(_populate_SerialBus_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::SerialBus APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt5SerialBus_install_prefix}/lib/${LIB_LOCATION}")
    _qt5_SerialBus_check_file_exists(${imported_location})
    set(_deps
        ${_Qt5SerialBus_LIB_DEPENDENCIES}
    )
    set(_static_deps
    )

    set_target_properties(Qt5::SerialBus PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        "IMPORTED_SONAME_${Configuration}" "libQt5SerialBus.so.5"
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::SerialBus APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )


endmacro()

if (NOT TARGET Qt5::SerialBus)

    set(_Qt5SerialBus_OWN_INCLUDE_DIRS "${_qt5SerialBus_install_prefix}/include/" "${_qt5SerialBus_install_prefix}/include/QtSerialBus")
    set(Qt5SerialBus_PRIVATE_INCLUDE_DIRS
        "${_qt5SerialBus_install_prefix}/include/QtSerialBus/5.14.1"
        "${_qt5SerialBus_install_prefix}/include/QtSerialBus/5.14.1/QtSerialBus"
    )

    foreach(_dir ${_Qt5SerialBus_OWN_INCLUDE_DIRS})
        _qt5_SerialBus_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt5SerialBus_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt5SerialBus_PRIVATE_INCLUDE_DIRS})
            _qt5_SerialBus_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt5SerialBus_INCLUDE_DIRS ${_Qt5SerialBus_OWN_INCLUDE_DIRS})

    set(Qt5SerialBus_DEFINITIONS -DQT_SERIALBUS_LIB)
    set(Qt5SerialBus_COMPILE_DEFINITIONS QT_SERIALBUS_LIB)
    set(_Qt5SerialBus_MODULE_DEPENDENCIES "Core")


    set(Qt5SerialBus_OWN_PRIVATE_INCLUDE_DIRS ${Qt5SerialBus_PRIVATE_INCLUDE_DIRS})

    set(_Qt5SerialBus_FIND_DEPENDENCIES_REQUIRED)
    if (Qt5SerialBus_FIND_REQUIRED)
        set(_Qt5SerialBus_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt5SerialBus_FIND_DEPENDENCIES_QUIET)
    if (Qt5SerialBus_FIND_QUIETLY)
        set(_Qt5SerialBus_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt5SerialBus_FIND_VERSION_EXACT)
    if (Qt5SerialBus_FIND_VERSION_EXACT)
        set(_Qt5SerialBus_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt5SerialBus_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt5SerialBus_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.14.1 ${_Qt5SerialBus_FIND_VERSION_EXACT}
                ${_Qt5SerialBus_DEPENDENCIES_FIND_QUIET}
                ${_Qt5SerialBus_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt5SerialBus_FOUND False)
            return()
        endif()

        list(APPEND Qt5SerialBus_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt5SerialBus_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt5SerialBus_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt5SerialBus_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt5SerialBus_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt5SerialBus_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5SerialBus_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5SerialBus_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5SerialBus_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5SerialBus_EXECUTABLE_COMPILE_FLAGS)

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
    if(TARGET Qt5::SerialBus)
        return()
    endif()

    set(_Qt5SerialBus_LIB_DEPENDENCIES "Qt5::Core")


    add_library(Qt5::SerialBus SHARED IMPORTED)

    set_property(TARGET Qt5::SerialBus PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt5SerialBus_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::SerialBus PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_SERIALBUS_LIB)

    set_property(TARGET Qt5::SerialBus PROPERTY INTERFACE_QT_ENABLED_FEATURES modbus-serialport)
    set_property(TARGET Qt5::SerialBus PROPERTY INTERFACE_QT_DISABLED_FEATURES )

    set_property(TARGET Qt5::SerialBus PROPERTY INTERFACE_QT_PLUGIN_TYPES "canbus")

    set(_Qt5SerialBus_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt5SerialBus_PRIVATE_DIR ${Qt5SerialBus_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt5SerialBus_PRIVATE_DIR})
            set(_Qt5SerialBus_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt5SerialBus_PRIVATE_DIRS_EXIST)
        add_library(Qt5::SerialBusPrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::SerialBusPrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt5SerialBus_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt5SerialBus_PRIVATEDEPS)
        foreach(dep ${_Qt5SerialBus_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt5SerialBus_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::SerialBusPrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::SerialBus ${_Qt5SerialBus_PRIVATEDEPS}
        )
    endif()

    _populate_SerialBus_target_properties(RELEASE "libQt5SerialBus.so.5.14.1" "" FALSE)





    file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt5SerialBus_*Plugin.cmake")

    macro(_populate_SerialBus_plugin_properties Plugin Configuration PLUGIN_LOCATION
          IsDebugAndRelease)
        set_property(TARGET Qt5::${Plugin} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

        set(imported_location "${_qt5SerialBus_install_prefix}/plugins/${PLUGIN_LOCATION}")
        _qt5_SerialBus_check_file_exists(${imported_location})
        set_target_properties(Qt5::${Plugin} PROPERTIES
            "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        )

    endmacro()

    if (pluginTargets)
        foreach(pluginTarget ${pluginTargets})
            include(${pluginTarget})
        endforeach()
    endif()





_qt5_SerialBus_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt5SerialBusConfigVersion.cmake")

endif()
