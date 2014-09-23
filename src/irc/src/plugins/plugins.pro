######################################################################
# Communi
######################################################################

TEMPLATE = subdirs
!no_uchardet:SUBDIRS += uchardet
greaterThan(QT_MAJOR_VERSION, 4) {
    SUBDIRS += quick2
} else {
    !lessThan(QT_MAJOR_VERSION, 4):!lessThan(QT_MINOR_VERSION, 7):SUBDIRS += declarative
}

# respect the icu/no_icu config if specified,
# otherwise try to auto detect using pkg-config
include(pkg.pri)
!icu:!no_icu {
    pkgExists(icu)|pkgExists(icu-i18n):SUBDIRS += icu
} else:CONFIG(icu, icu|no_icu):SUBDIRS *= icu
