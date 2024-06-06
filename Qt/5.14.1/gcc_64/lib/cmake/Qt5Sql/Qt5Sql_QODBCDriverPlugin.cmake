
add_library(Qt5::QODBCDriverPlugin MODULE IMPORTED)


_populate_Sql_plugin_properties(QODBCDriverPlugin RELEASE "sqldrivers/libqsqlodbc.so" FALSE)

list(APPEND Qt5Sql_PLUGINS Qt5::QODBCDriverPlugin)
set_property(TARGET Qt5::Sql APPEND PROPERTY QT_ALL_PLUGINS_sqldrivers Qt5::QODBCDriverPlugin)
set_property(TARGET Qt5::QODBCDriverPlugin PROPERTY QT_PLUGIN_TYPE "sqldrivers")
set_property(TARGET Qt5::QODBCDriverPlugin PROPERTY QT_PLUGIN_EXTENDS "")
