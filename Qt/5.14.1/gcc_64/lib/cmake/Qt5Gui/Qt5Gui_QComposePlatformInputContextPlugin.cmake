
add_library(Qt5::QComposePlatformInputContextPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QComposePlatformInputContextPlugin RELEASE "platforminputcontexts/libcomposeplatforminputcontextplugin.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QComposePlatformInputContextPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforminputcontexts Qt5::QComposePlatformInputContextPlugin)
set_property(TARGET Qt5::QComposePlatformInputContextPlugin PROPERTY QT_PLUGIN_TYPE "platforminputcontexts")
set_property(TARGET Qt5::QComposePlatformInputContextPlugin PROPERTY QT_PLUGIN_EXTENDS "-")
