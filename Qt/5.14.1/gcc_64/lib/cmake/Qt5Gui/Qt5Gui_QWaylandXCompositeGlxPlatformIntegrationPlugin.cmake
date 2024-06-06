
add_library(Qt5::QWaylandXCompositeGlxPlatformIntegrationPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QWaylandXCompositeGlxPlatformIntegrationPlugin RELEASE "platforms/libqwayland-xcomposite-glx.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QWaylandXCompositeGlxPlatformIntegrationPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_platforms Qt5::QWaylandXCompositeGlxPlatformIntegrationPlugin)
set_property(TARGET Qt5::QWaylandXCompositeGlxPlatformIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "platforms")
set_property(TARGET Qt5::QWaylandXCompositeGlxPlatformIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
