
add_library(Qt5::QWaylandEglClientBufferPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(QWaylandEglClientBufferPlugin RELEASE "wayland-graphics-integration-client/libqt-plugin-wayland-egl.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::QWaylandEglClientBufferPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_graphics_integration_client Qt5::QWaylandEglClientBufferPlugin)
set_property(TARGET Qt5::QWaylandEglClientBufferPlugin PROPERTY QT_PLUGIN_TYPE "wayland-graphics-integration-client")
set_property(TARGET Qt5::QWaylandEglClientBufferPlugin PROPERTY QT_PLUGIN_EXTENDS "")
