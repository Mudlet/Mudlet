######################################################################
# Communi
######################################################################

TEMPLATE = subdirs
contains(MEEGO_EDITION,harmattan):SUBDIRS += mobile
else:SUBDIRS += bot desktop
