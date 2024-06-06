
add_library(Qt5::QWebEngineWebViewPlugin MODULE IMPORTED)


_populate_WebView_plugin_properties(QWebEngineWebViewPlugin RELEASE "webview/libqtwebview_webengine.so" FALSE)

list(APPEND Qt5WebView_PLUGINS Qt5::QWebEngineWebViewPlugin)
set_property(TARGET Qt5::WebView APPEND PROPERTY QT_ALL_PLUGINS_webview Qt5::QWebEngineWebViewPlugin)
set_property(TARGET Qt5::QWebEngineWebViewPlugin PROPERTY QT_PLUGIN_TYPE "webview")
set_property(TARGET Qt5::QWebEngineWebViewPlugin PROPERTY QT_PLUGIN_EXTENDS "")
