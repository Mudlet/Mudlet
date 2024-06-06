
add_library(Qt5::QGifPlugin MODULE IMPORTED)


_populate_Gui_plugin_properties(QGifPlugin RELEASE "imageformats/libqgif.so" FALSE)

list(APPEND Qt5Gui_PLUGINS Qt5::QGifPlugin)
set_property(TARGET Qt5::Gui APPEND PROPERTY QT_ALL_PLUGINS_imageformats Qt5::QGifPlugin)
set_property(TARGET Qt5::QGifPlugin PROPERTY QT_PLUGIN_TYPE "imageformats")
set_property(TARGET Qt5::QGifPlugin PROPERTY QT_PLUGIN_EXTENDS "")
