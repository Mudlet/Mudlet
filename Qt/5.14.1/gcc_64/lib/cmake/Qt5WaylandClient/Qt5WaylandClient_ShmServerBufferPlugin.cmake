
add_library(Qt5::ShmServerBufferPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(ShmServerBufferPlugin RELEASE "wayland-graphics-integration-client/libshm-emulation-server.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::ShmServerBufferPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_graphics_integration_client Qt5::ShmServerBufferPlugin)
set_property(TARGET Qt5::ShmServerBufferPlugin PROPERTY QT_PLUGIN_TYPE "wayland-graphics-integration-client")
set_property(TARGET Qt5::ShmServerBufferPlugin PROPERTY QT_PLUGIN_EXTENDS "")
