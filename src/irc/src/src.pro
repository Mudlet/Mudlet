######################################################################
# Communi
######################################################################

TEMPLATE = subdirs
SUBDIRS += core model util imports

model.depends = core
util.depends = core model
imports.depends = core model util

coverage {
    cov_zerocounters.CONFIG += recursive
    cov_zerocounters.recurse = core model util
    cov_zerocounters.recurse_target = zerocounters
    QMAKE_EXTRA_TARGETS += cov_zerocounters

    cov_capture.CONFIG += recursive
    cov_capture.recurse = core model util
    cov_capture.recurse_target = capture
    QMAKE_EXTRA_TARGETS += cov_capture

    cov_genhtml.CONFIG += recursive
    cov_genhtml.recurse = core model util
    cov_genhtml.recurse_target = genhtml
    QMAKE_EXTRA_TARGETS += cov_genhtml
}
