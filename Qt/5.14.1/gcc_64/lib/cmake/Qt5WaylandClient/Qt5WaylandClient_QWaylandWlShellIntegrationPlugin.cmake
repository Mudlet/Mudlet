
add_library(Qt5::QWaylandWlShellIntegrationPlugin MODULE IMPORTED)


_populate_WaylandClient_plugin_properties(QWaylandWlShellIntegrationPlugin RELEASE "wayland-shell-integration/libwl-shell.so" FALSE)

list(APPEND Qt5WaylandClient_PLUGINS Qt5::QWaylandWlShellIntegrationPlugin)
set_property(TARGET Qt5::WaylandClient APPEND PROPERTY QT_ALL_PLUGINS_wayland_shell_integration Qt5::QWaylandWlShellIntegrationPlugin)
set_property(TARGET Qt5::QWaylandWlShellIntegrationPlugin PROPERTY QT_PLUGIN_TYPE "wayland-shell-integration")
set_property(TARGET Qt5::QWaylandWlShellIntegrationPlugin PROPERTY QT_PLUGIN_EXTENDS "")
