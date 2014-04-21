
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../cmake ${CMAKE_MODULE_PATH})
SET(mudlet_RCCS mudlet_alpha.qrc)
SET( mudlet_SRCS 
TConsole.cpp
    ctelnet.cpp
    main.cpp
    Host.cpp
    HostManager.cpp
    HostPool.cpp
    dlgConnectionProfiles.cpp
    dlgTriggerEditor.cpp
    TTrigger.cpp
    TriggerUnit.cpp
    TLuaInterpreter.cpp
    dlgTriggersMainArea.cpp
    dlgOptionsAreaTriggers.cpp
    dlgOptionsAreaTimers.cpp
    dlgOptionsAreaScripts.cpp
    TCommandLine.cpp
    TTreeWidget.cpp
    TTreeWidgetItem.cpp
    TTimer.cpp
    TScript.cpp
    TAlias.cpp
    dlgTimersMainArea.cpp
    dlgSystemMessageArea.cpp
    dlgSourceEditorArea.cpp
    TimerUnit.cpp
    ScriptUnit.cpp
    AliasUnit.cpp
    dlgScriptsMainArea.cpp
    dlgAliasMainArea.cpp
    dlgOptionsAreaAlias.cpp
    dlgSearchArea.cpp
    TAction.cpp
    ActionUnit.cpp
    dlgActionMainArea.cpp
    dlgOptionsAreaAction.cpp
    EAction.cpp
    dlgAboutDialog.cpp
    TDebug.cpp
    dlgKeysMainArea.cpp
    TKey.cpp
    KeyUnit.cpp
    dlgProfilePreferences.cpp
    TTextEdit.cpp
    XMLexport.cpp
    XMLimport.cpp
    FontManager.cpp
    TFlipButton.cpp
    TToolBar.cpp
    TLabel.cpp
    TEasyButtonBar.cpp
    TForkedProcess.cpp
    dlgColorTrigger.cpp
    dlgTriggerPatternEdit.cpp
    TSplitter.cpp
    TSplitterHandle.cpp
    mudlet.cpp
    dlgNotepad.cpp
    THighlighter.cpp
    dlgComposer.cpp
    TArea.cpp
    glwidget.cpp
    dlgMapper.cpp
    TRoom.cpp
    TMap.cpp
    TBuffer.cpp
    T2DMap.cpp
    dlgRoomExits.cpp
	lua_yajl.c
	dlgIrc.cpp
)

SET(mudlet_UIS 
ui/connection_profiles.ui
    ui/main_window.ui
    ui/trigger_editor.ui
    ui/options_area_triggers.ui
    ui/options_area_timers.ui
    ui/options_area_aliases.ui
    ui/options_area_scripts.ui
    ui/triggers_main_area.ui
    ui/scripts_main_area.ui
    ui/aliases_main_area.ui
    ui/system_message_area.ui
    ui/source_editor_area.ui
    ui/extended_search_area.ui
    ui/actions_main_area.ui
    ui/options_area_actions.ui
    ui/timers_main_area.ui
    ui/about_dialog.ui
    ui/keybindings_main_area.ui
    ui/color_trigger.ui
    ui/notes_editor.ui
    ui/trigger_pattern_edit.ui
    ui/composer.ui
    ui/mapper.ui
    ui/profile_preferences.ui
    ui/mapper_room_color.ui
    ui/room_exits.ui
    ui/lacking_mapper_script.ui
)

SET(mudlet_MOC_HDRS 
mudlet.h
    TTimer.h
    EAction.h
    TConsole.h
    ctelnet.h
    Host.h
    TMap.h
    TAStar.h
    HostManager.h
    HostPool.h
    dlgConnectionProfiles.h
    dlgTriggerEditor.h
    TTrigger.h
    TLuaInterpreter.h
    dlgTriggers_main_area.h
    dlgOptionsAreaTriggers.h
    dlgTriggerPatternEdit.h
    TCommandLine.h
    TTreeWidget.h
    TTreeWidgetItem.h
    TScript.h
    TAlias.h
    dlgTimersMainArea.h
    dlgSourceEditorArea.h
    dlgSystemMessageArea.h
    TimerUnit.h
    ScriptUnit.h
    AliasUnit.h
    dlgScriptsMainArea.h
    dlgAliasMainArea.h
    dlgOptionsAreaAlias.h
    dlgOptionsAreaScripts.h
    dlgOptionsAreaTimers.h
    dlgSearchArea.h
    TAction.h
    ActionUnit.h
    dlgActionMainArea.h
    dlgOptionsAreaAction.h
    dlgAboutDialog.h
    TMatchState.h
    TEvent.h
    TDebug.h
    dlgKeysMainArea.h
    TKey.h
    KeyUnit.h
    dlgProfilePreferences.h
    TTextEdit.h
    TFlipButton.h
    TToolBar.h
    TBuffer.h
    TriggerUnit.h
    TLabel.h
    TEasyButtonBar.h
    TForkedProcess.h
    dlgColorTrigger.h
    TSplitter.h
    TSplitterHandle.h
    dlgNotepad.h
    THighlighter.h
    dlgComposer.h
    TRoom.h
    TArea.h
    TMap.h
    glwidget.h
    dlgMapper.h
    Tree.h
    T2DMap.h
    dlgRoomExits.h
	dlgIrc.h
)

SET(PCRE_DIR "${PROJECT_SOURCE_DIR}/.." CACHE PATH "PCRE install directory.")

IF(EXISTS "${PROJECT_SOURCE_DIR}/../include/lua.h")
  SET(LUA_INCLUDE_DIR_TEMP "${PROJECT_SOURCE_DIR}/../include")
  GET_FILENAME_COMPONENT(LUA_INCLUDE_DIR ${LUA_INCLUDE_DIR_TEMP} ABSOLUTE CACHE)
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/lualib.lib")
    SET(LUA_LIBRARY_RELEASE_TEMP "${PROJECT_SOURCE_DIR}/../lib/lualib.lib")
    GET_FILENAME_COMPONENT(LUA_LIBRARY_RELEASE ${LUA_LIBRARY_RELEASE_TEMP} ABSOLUTE CACHE)
  ENDIF()
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/lualibd.lib")
    SET(LUA_LIBRARY_DEBUG_TEMP "${PROJECT_SOURCE_DIR}/../lib/lualibd.lib")
    GET_FILENAME_COMPONENT(LUA_LIBRARY_DEBUG ${LUA_LIBRARY_DEBUG_TEMP} ABSOLUTE CACHE)
  ENDIF()
  IF(LUA_LIBRARY_DEBUG AND LUA_LIBRARY_RELEASE)
    SET(LUA_LIBRARY optimized ${LUA_LIBRARY_RELEASE} debug ${LUA_LIBRARY_DEBUG} )
  ELSEIF(LUA_LIBRARY_RELEASE)
    SET(LUA_LIBRARY ${LUA_LIBRARY_RELEASE} )
  ELSEIF(LUA_LIBRARY_DEBUG)
    SET(LUA_LIBRARY ${LUA_LIBRARY_DEBUG} )
  ENDIF()
ENDIF()

IF(EXISTS "${PROJECT_SOURCE_DIR}/../include/zlib.h")
  SET(ZLIB_INCLUDE_DIR_TEMP "${PROJECT_SOURCE_DIR}/../include")
  GET_FILENAME_COMPONENT(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIR_TEMP} ABSOLUTE CACHE)
  SET(ZLIB_ROOT "${PROJECT_SOURCE_DIR}/..")
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/zlibstat.lib")
    SET(ZLIB_LIBRARY_RELEASE_TEMP "${PROJECT_SOURCE_DIR}/../lib/zlibstat.lib")
    GET_FILENAME_COMPONENT(ZLIB_LIBRARY_RELEASE ${ZLIB_LIBRARY_RELEASE_TEMP} ABSOLUTE CACHE)
  ENDIF()
  IF(EXISTS "${PROJECT_SOURCE_DIR}/../lib/zlibstatd.lib")
    SET(ZLIB_LIBRARY_DEBUG_TEMP "${PROJECT_SOURCE_DIR}/../lib/zlibstatd.lib")
    GET_FILENAME_COMPONENT(ZLIB_LIBRARY_DEBUG ${ZLIB_LIBRARY_DEBUG_TEMP} ABSOLUTE CACHE)
  ENDIF()
  IF(ZLIB_LIBRARY_DEBUG AND ZLIB_LIBRARY_RELEASE)
    SET(ZLIB_LIBRARY optimized ${ZLIB_LIBRARY_RELEASE} debug ${ZLIB_LIBRARY_DEBUG} )
  ELSEIF(ZLIB_LIBRARY_RELEASE)
    SET(ZLIB_LIBRARY ${ZLIB_LIBRARY_RELEASE} )
  ELSEIF(ZLIB_LIBRARY_DEBUG)
    SET(ZLIB_LIBRARY ${ZLIB_LIBRARY_DEBUG} )
  ENDIF()
ENDIF()

FIND_PACKAGE(Qt5Core REQUIRED)
FIND_PACKAGE(Qt5Multimedia REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5UiTools REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(Lua51 REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(PCRE REQUIRED)

SET(Boost_USE_STATIC_LIBS ON)
FIND_PACKAGE(Boost 1.44 COMPONENTS graph)

SET(LUA_CPP OFF CACHE BOOL "Lua is compiled as C++ rather than C")

IF(LUA_CPP)
  ADD_DEFINITIONS(-DLUA_CPP)
ENDIF(LUA_CPP)

INCLUDE_DIRECTORIES(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5OpenGL_INCLUDE_DIRS}
    ${Qt5Multimedia_INCLUDE_DIRS}
    ${Qt5UiTools_INCLUDE_DIRS}
)

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${mudlet_BINARY_DIR}/src
    ${PCRE_INCLUDE_DIR}
    ${LUA_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
"C:\\mudlet_package_MSVC\\hunspell-1.3.1\\src"
"C:\\mudlet_package_MSVC\\lloyd-yajl-f4b2b1a\\yajl-2.0.1\\include"
"C:\\mudlet_package_MSVC\\libircclient-qt-0.5.0\\include"
)

IF(CMAKE_COMPILER_IS_GNUCXX)
  ADD_DEFINITIONS(${CMAKE_CXX_FLAGS} "-Wno-deprecated  -DUNICODE  -D_UNICODE")
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

QT5_WRAP_UI(mudlet_UIS_H ${mudlet_UIS})
QT5_WRAP_CPP(mudlet_MOC_SRCS ${mudlet_MOC_HDRS})
QT5_ADD_RESOURCES(mudlet_RCC_SRCS ${mudlet_RCCS})

add_executable(mudlet WIN32 ${mudlet_SRCS} ${mudlet_UIS_H} ${mudlet_MOC_SRCS} ${mudlet_RCC_SRCS})

TARGET_LINK_LIBRARIES( 
mudlet 
    ${Qt5Core_QTMAIN_LIBRARIES}
    ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Multimedia_LIBRARIES}
    ${Qt5OpenGL_LIBRARIES}
    ${Qt5UiTools_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LUA_LIBRARIES}
    ${PCRE_LIBRARIES}
C:\\mudlet_package_MSVC\\hunspell-1.3.1\\src\\win_api\\Release_dll\\libhunspell\\libhunspell.lib
    ${OPENGL_LIBRARIES}
"C:\\Users\\heiko\\Desktop\\lloyd-yajl-f4b2b1a\\yajl-2.0.1\\lib\\Release\\yajl.lib"
"C:\\mudlet_package_MSVC\\libircclient-qt-0.5.0\\lib\\ircclient-qt.lib"
    ${Boost_LIBRARIES}
)

install( PROGRAMS mudlet DESTINATION "/usr/local/bin" )
install( FILES LuaGlobal.lua mudlet_documentation.html fonts/ttf-bitstream-vera-1.10/COPYRIGHT.TXT fonts/ttf-bitstream-vera-1.10/local.conf fonts/ttf-bitstream-vera-1.10/README.TXT fonts/ttf-bitstream-vera-1.10/RELEASENOTES.TXT fonts/ttf-bitstream-vera-1.10/VeraBd.ttf fonts/ttf-bitstream-vera-1.10/VeraBI.ttf fonts/ttf-bitstream-vera-1.10/VeraIt.ttf fonts/ttf-bitstream-vera-1.10/VeraMoBd.ttf fonts/ttf-bitstream-vera-1.10/VeraMoBI.ttf fonts/ttf-bitstream-vera-1.10/VeraMoIt.ttf fonts/ttf-bitstream-vera-1.10/VeraMono.ttf fonts/ttf-bitstream-vera-1.10/VeraSeBd.ttf fonts/ttf-bitstream-vera-1.10/VeraSe.ttf fonts/ttf-bitstream-vera-1.10/Vera.ttf DESTINATION "/usr/local/share/mudlet" )



