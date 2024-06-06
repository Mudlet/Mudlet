
add_library(Qt5::QWaylandXCompositeEglPlatformIntegrationPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QWaylandXCompositeEglPlatformIntegrationPlugin RELEASE "platforms/libqwayland-xcomposite-egl.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QWaylandXCompositeEglPlatformIntegrationPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforms Qt5::QWaylandXCompositeEglPlatformIntegrationPlugin)
set_property(TARGET Qt5::QWaylandXCompositeEglPlatformIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "platforms")
set_property(TARGET Qt5::QWaylandXCompositeEglPlatformIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
