######################################################################
# Communi
######################################################################

QT = core network gui testlib
greaterThan(QT_MAJOR_VERSION, 4):QT += widgets
CONFIG += testcase
CONFIG -= app_bundle

include(../tests.pri)
