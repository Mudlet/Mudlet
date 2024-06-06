
add_library(Qt5::QWaylandIviShellIntegrationPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(QWaylandIviShellIntegrationPlugin RELEASE "wayland-shell-integration/libivi-shell.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::QWaylandIviShellIntegrationPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_shell_integration Qt5::QWaylandIviShellIntegrationPlugin)
set_property(TARGET Qt5::QWaylandIviShellIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "wayland-shell-integration")
set_property(TARGET Qt5::QWaylandIviShellIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
