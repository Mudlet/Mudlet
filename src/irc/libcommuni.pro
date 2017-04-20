######################################################################
# Communi
######################################################################

TEMPLATE = subdirs
SUBDIRS += src
CONFIG += ordered

!no_tests {
    SUBDIRS += tests
} else {
    message(Tests disabled)
}

!no_examples {
    SUBDIRS += examples
} else {
    message(Examples disabled)
}

lessThan(QT_MAJOR_VERSION, 5) {
    lessThan(QT_MAJOR_VERSION, 4) | lessThan(QT_MINOR_VERSION, 6) {
        error(Communi requires Qt 4.6 or newer but Qt $$[QT_VERSION] was detected.)
    }
    defineTest(write_file) {
        first = true
        for(line, $$2) {
            !isEmpty(first):system(echo $$line > $$1)
            else:system(echo $$line >> $$1)
            first =
        }
        return(true)
    }
}

static:CONFIG_LINES += "DEFINES+=IRC_STATIC"
else:CONFIG_LINES += "DEFINES+=IRC_SHARED"

include(version.pri)
IRC_VERSION_MAJOR = $$section(IRC_VERSION, ., 0, 0)
IRC_VERSION_MINOR = $$section(IRC_VERSION, ., 1, 1)
IRC_VERSION_PATCH = $$section(IRC_VERSION, ., 2, 2)

CONFIG_LINES += "IRC_VERSION=$$IRC_VERSION"
CONFIG_LINES += "IRC_VERSION_MAJOR=$$IRC_VERSION_MAJOR"
CONFIG_LINES += "IRC_VERSION_MINOR=$$IRC_VERSION_MINOR"
CONFIG_LINES += "IRC_VERSION_PATCH=$$IRC_VERSION_PATCH"

isEmpty(IRC_INSTALL_LIBS):IRC_INSTALL_LIBS = $$[QT_INSTALL_LIBS]
isEmpty(IRC_INSTALL_BINS):IRC_INSTALL_BINS = $$[QT_INSTALL_BINS]
isEmpty(IRC_INSTALL_HEADERS):IRC_INSTALL_HEADERS = $$[QT_INSTALL_HEADERS]/Communi
isEmpty(IRC_INSTALL_FEATURES) {
    isEqual(QT_MAJOR_VERSION, 5):IRC_INSTALL_FEATURES = $$[QT_HOST_DATA]/mkspecs/features
    else:IRC_INSTALL_FEATURES = $$[QMAKE_MKSPECS]/features
}

# qt4/win: WARNING: Unescaped backslashes are deprecated
!win32|greaterThan(QT_MAJOR_VERSION, 4) {
    CONFIG_LINES += "IRC_INSTALL_LIBS=$$IRC_INSTALL_LIBS"
    CONFIG_LINES += "IRC_INSTALL_BINS=$$IRC_INSTALL_BINS"
    CONFIG_LINES += "IRC_INSTALL_HEADERS=$$IRC_INSTALL_HEADERS"
    CONFIG_LINES += "IRC_INSTALL_FEATURES=$$IRC_INSTALL_FEATURES"
}

IRC_CONFIG = $${OUT_PWD}$${QMAKE_DIR_SEP}communi-config.prf
write_file($$IRC_CONFIG, CONFIG_LINES)

CONFIG_VARS = $${OUT_PWD}$${QMAKE_DIR_SEP}.config.vars
exists($$CONFIG_VARS) {
    CONFIG_LINES += "include\\\($$CONFIG_VARS\\\)"
}
QMAKE_CACHE = $${OUT_PWD}$${QMAKE_DIR_SEP}.qmake.cache
write_file($$QMAKE_CACHE, CONFIG_LINES)

OTHER_FILES += .gitignore
OTHER_FILES += .travis.yml
OTHER_FILES += AUTHORS
OTHER_FILES += CHANGELOG
OTHER_FILES += configure
OTHER_FILES += Doxyfile
OTHER_FILES += INSTALL
OTHER_FILES += LICENSE
OTHER_FILES += README
OTHER_FILES += features/communi.prf

include(doc/doc.pri)

features.files += features/communi.prf
features.files += $$OUT_PWD/communi-config.prf
features.path = $$IRC_INSTALL_FEATURES
INSTALLS += features

!build_pass {
    macx {
        !qt_no_framework {
            message(Building Communi $$IRC_VERSION (framework))
        } else {
            message(Building Communi $$IRC_VERSION (dylib))
        }
    } else {
        message(Building Communi $$IRC_VERSION)
    }
}

coverage {
    cov_zerocounters.CONFIG += recursive
    cov_zerocounters.recurse = src
    QMAKE_EXTRA_TARGETS += cov_zerocounters

    cov_capture.CONFIG += recursive
    cov_capture.recurse = src
    QMAKE_EXTRA_TARGETS += cov_capture

    cov_genhtml.CONFIG += recursive
    cov_genhtml.recurse = src
    QMAKE_EXTRA_TARGETS += cov_genhtml

    coverage.depends += first cov_zerocounters check cov_capture cov_genhtml
    QMAKE_EXTRA_TARGETS += coverage
    !build_pass:message(Code coverage collection enabled)
}
