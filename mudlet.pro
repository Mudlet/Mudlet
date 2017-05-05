TEMPLATE = subdirs

edbee_lib.subdir = 3rdparty/edbee-lib/edbee-lib

src.subdir = src
src.depends = edbee_lib


SUBDIRS = \
    edbee_lib \
    src
