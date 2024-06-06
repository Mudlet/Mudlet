
add_library(Qt5::QWaylandBradientDecorationPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(QWaylandBradientDecorationPlugin RELEASE "wayland-decoration-client/libbradient.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::QWaylandBradientDecorationPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_decoration_client Qt5::QWaylandBradientDecorationPlugin)
set_property(TARGET Qt5::QWaylandBradientDecorationPlugin PROPERTY QT_PLUGIN_TYPE "wayland-decoration-client")
set_property(TARGET Qt5::QWaylandBradientDecorationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
