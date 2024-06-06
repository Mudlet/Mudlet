
add_library(Qt5::QWaylandXdgShellIntegrationPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(QWaylandXdgShellIntegrationPlugin RELEASE "wayland-shell-integration/libxdg-shell.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::QWaylandXdgShellIntegrationPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_shell_integration Qt5::QWaylandXdgShellIntegrationPlugin)
set_property(TARGET Qt5::QWaylandXdgShellIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "wayland-shell-integration")
set_property(TARGET Qt5::QWaylandXdgShellIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
