
add_library(Qt5::QWaylandEglPlatformIntegrationPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QWaylandEglPlatformIntegrationPlugin RELEASE "platforms/libqwayland-egl.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QWaylandEglPlatformIntegrationPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforms Qt5::QWaylandEglPlatformIntegrationPlugin)
set_property(TARGET Qt5::QWaylandEglPlatformIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "platforms")
set_property(TARGET Qt5::QWaylandEglPlatformIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
