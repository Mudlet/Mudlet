
add_library(Qt5::QWaylandXdgShellV5IntegrationPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(QWaylandXdgShellV5IntegrationPlugin RELEASE "wayland-shell-integration/libxdg-shell-v5.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::QWaylandXdgShellV5IntegrationPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_shell_integration Qt5::QWaylandXdgShellV5IntegrationPlugin)
set_property(TARGET Qt5::QWaylandXdgShellV5IntegrationPlugin PROPERTY QT_PLUGIN_TYPE "wayland-shell-integration")
set_property(TARGET Qt5::QWaylandXdgShellV5IntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
