######################################################################
# Communi
######################################################################

include(../pkg.pri)

pkgExists(icu) {
    CONFIG += link_pkgconfig
    PKGCONFIG += icu
} else:pkgExists(icu-i18n) {
    CONFIG += link_pkgconfig
    PKGCONFIG += icu-i18n
}

isEmpty(PKGCONFIG) {
    win32 {
        isEmpty(ICU_DIR):ICU_DIR = $$(ICU_DIR)
        isEmpty(ICU_DIR):ICU_DIR = C:/ICU
        !build_pass {
            !exists($$ICU_DIR) {
                error("ICU support has been enabled, but the ICU installation \
                       has not been found at $${ICU_DIR}. Please download and \
                       install ICU from http://icu-project.org and/or specify \
                       ICU_DIR to match the installation location.")
            } else {
                message("Using ICU from $${ICU_DIR}")
            }
        }
        INCLUDEPATH += $$ICU_DIR/include
        LIBS += -L$$ICU_DIR/lib -licuuc

        # icudata vs. icudt
        exists($$ICU_DIR/bin/icudata*.dll):LIBS += -licudata
        else:LIBS += -licudt

        # icui18n vs. icuin
        exists($$ICU_DIR/bin/icui18n*.dll):LIBS += -licui18n
        else:LIBS += -licuin
    } else {
        LIBS += -licui18n -licudata -licuuc
    }
}
