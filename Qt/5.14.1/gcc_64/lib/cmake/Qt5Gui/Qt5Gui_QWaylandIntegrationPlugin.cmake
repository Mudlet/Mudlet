
add_library(Qt5::QWaylandIntegrationPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QWaylandIntegrationPlugin RELEASE "platforms/libqwayland-generic.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QWaylandIntegrationPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforms Qt5::QWaylandIntegrationPlugin)
set_property(TARGET Qt5::QWaylandIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "platforms")
set_property(TARGET Qt5::QWaylandIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
