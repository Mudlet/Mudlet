######################################################################
# Communi
######################################################################

TEMPLATE = subdirs
SUBDIRS += auto
!no_benchmarks:SUBDIRS += benchmarks

mac {
    # TODO: install_name_tool?
    check.commands += cd auto &&
    check.commands += DYLD_FRAMEWORK_PATH=$$OUT_PWD/../lib $(MAKE) check
    !no_benchmarks {
        check.commands += && cd ../benchmarks &&
        check.commands += DYLD_FRAMEWORK_PATH=$$OUT_PWD/../lib $(MAKE) check
    }
    QMAKE_EXTRA_TARGETS += check
}
