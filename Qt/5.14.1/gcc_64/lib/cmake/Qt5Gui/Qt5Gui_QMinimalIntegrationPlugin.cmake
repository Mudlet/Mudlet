
add_library(Qt5::QMinimalIntegrationPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QMinimalIntegrationPlugin RELEASE "platforms/libqminimal.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QMinimalIntegrationPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforms Qt5::QMinimalIntegrationPlugin)
set_property(TARGET Qt5::QMinimalIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "platforms")
set_property(TARGET Qt5::QMinimalIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "-")
