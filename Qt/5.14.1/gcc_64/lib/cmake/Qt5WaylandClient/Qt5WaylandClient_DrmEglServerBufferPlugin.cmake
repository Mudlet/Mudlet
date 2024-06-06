
add_library(Qt5::DrmEglServerBufferPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(DrmEglServerBufferPlugin RELEASE "wayland-graphics-integration-client/libdrm-egl-server.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::DrmEglServerBufferPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_graphics_integration_client Qt5::DrmEglServerBufferPlugin)
set_property(TARGET Qt5::DrmEglServerBufferPlugin PROPERTY QT_PLUGIN_TYPE "wayland-graphics-integration-client")
set_property(TARGET Qt5::DrmEglServerBufferPlugin PROPERTY QT_PLUGIN_EXTENDS "")
