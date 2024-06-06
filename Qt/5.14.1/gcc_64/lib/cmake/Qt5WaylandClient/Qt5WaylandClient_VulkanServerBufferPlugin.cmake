
add_library(Qt5::VulkanServerBufferPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(VulkanServerBufferPlugin RELEASE "wayland-graphics-integration-client/libvulkan-server.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::VulkanServerBufferPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_graphics_integration_client Qt5::VulkanServerBufferPlugin)
set_property(TARGET Qt5::VulkanServerBufferPlugin PROPERTY QT_PLUGIN_TYPE "wayland-graphics-integration-client")
set_property(TARGET Qt5::VulkanServerBufferPlugin PROPERTY QT_PLUGIN_EXTENDS "")
