######################################################################
# Communi
######################################################################

TEMPLATE = subdirs

SUBDIRS += ircmessage
SUBDIRS += irctextformat

# - windows has problems with symbols
# - mac with private headers (frameworks)
!win32:!mac:SUBDIRS += ircmessagedecoder
