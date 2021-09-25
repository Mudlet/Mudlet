######################################################################
# Communi
######################################################################

include(pkg.pri)

pkgExists(uchardet) {
    CONFIG += link_pkgconfig
    PKGCONFIG += uchardet
    !build_pass:message("Using uchardet via pkg-config")
}

isEmpty(PKGCONFIG) {
    win32 {
        isEmpty(UCHARDET_DIR):UCHARDET_DIR = $$(UCHARDET_DIR)
        isEmpty(UCHARDET_DIR):UCHARDET_DIR = C:/uchardet
        !build_pass {
            !exists($$UCHARDET_DIR) {
                error("uchardet support has been enabled, but the uchardet installation \
                       has not been found at $${UCHARDET_DIR}. Please download and install \
                       uchardet from https://www.freedesktop.org/wiki/Software/uchardet/ \
                       and/or specify UCHARDET_DIR to match the installation location.")
            } else {
                message("Using uchardet from $${UCHARDET_DIR}")
            }
        }
        INCLUDEPATH += $$UCHARDET_DIR/include
        contains(QMAKE_TARGET.arch, x86_64):LIBS += -L$$ICU_DIR/lib64
        else:LIBS += -L$$UCHARDET_DIR/lib
    }
    LIBS += -luchardet
}
