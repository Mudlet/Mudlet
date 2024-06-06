
add_library(Qt5::DmaBufServerBufferPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(DmaBufServerBufferPlugin RELEASE "wayland-graphics-integration-client/libdmabuf-server.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::DmaBufServerBufferPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_graphics_integration_client Qt5::DmaBufServerBufferPlugin)
set_property(TARGET Qt5::DmaBufServerBufferPlugin PROPERTY QT_PLUGIN_TYPE "wayland-graphics-integration-client")
set_property(TARGET Qt5::DmaBufServerBufferPlugin PROPERTY QT_PLUGIN_EXTENDS "")
