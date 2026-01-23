######################################################################
# Communi
######################################################################

isEmpty(IRC_MODULE):error(IRC_MODULE must be set)

isEmpty(IRC_INSTALL_LIBS):IRC_INSTALL_LIBS = $$[QT_INSTALL_LIBS]
isEmpty(IRC_INSTALL_BINS):IRC_INSTALL_BINS = $$[QT_INSTALL_BINS]
isEmpty(IRC_INSTALL_HEADERS):IRC_INSTALL_HEADERS = $$[QT_INSTALL_HEADERS]/Communi

!no_install_libs {
    target.path = $$IRC_INSTALL_LIBS
    INSTALLS += target
}

!no_install_bins {
    dlltarget.path = $$IRC_INSTALL_BINS
    INSTALLS += dlltarget
}

macx:CONFIG(qt_framework, qt_framework|qt_no_framework) {
    CONFIG += lib_bundle debug_and_release
    CONFIG(debug, debug|release) {
        !build_pass:CONFIG += build_all
    } else { #release
        !debug_and_release|build_pass {
            FRAMEWORK_HEADERS.version = Versions
            FRAMEWORK_HEADERS.files = $$PUB_HEADERS $$CONV_HEADERS
            FRAMEWORK_HEADERS.path = Headers
        }
        QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
    }
    QMAKE_LFLAGS_SONAME = -Wl,-install_name,$$IRC_INSTALL_LIBS/
} else:!no_install_headers {
    headers.files = $$PUB_HEADERS $$CONV_HEADERS
    headers.path = $$IRC_INSTALL_HEADERS/$$IRC_MODULE
    INSTALLS += headers
}
