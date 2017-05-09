TEMPLATE = subdirs

edbee_lib.subdir = ../3rdparty/edbee-lib/edbee-lib

mudlet.file = mudlet.pro
mudlet.depends = edbee_lib


SUBDIRS = \
    edbee_lib \
    mudlet
