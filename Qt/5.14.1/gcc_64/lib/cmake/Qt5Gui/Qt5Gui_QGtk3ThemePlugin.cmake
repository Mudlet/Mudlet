
add_library(Qt5::QGtk3ThemePlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QGtk3ThemePlugin RELEASE "platformthemes/libqgtk3.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QGtk3ThemePlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platformthemes Qt5::QGtk3ThemePlugin)
set_property(TARGET Qt5::QGtk3ThemePlugin PROPERTY QT_PLUGIN_TYPE "platformthemes")
set_property(TARGET Qt5::QGtk3ThemePlugin PROPERTY QT_PLUGIN_EXTENDS "-")
