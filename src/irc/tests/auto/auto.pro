######################################################################
# Communi
######################################################################

TEMPLATE = subdirs

# IrcCore
SUBDIRS += irc
SUBDIRS += ircconnection
SUBDIRS += irccommand
SUBDIRS += ircmessage
SUBDIRS += ircnetwork

# IrcModel
SUBDIRS += ircbuffer
SUBDIRS += ircbuffermodel
SUBDIRS += ircchannel
SUBDIRS += ircuser
SUBDIRS += ircusermodel

# IrcUtil
SUBDIRS += irccommandparser
SUBDIRS += irccommandqueue
SUBDIRS += irccompleter
SUBDIRS += irclagtimer
SUBDIRS += ircpalette
SUBDIRS += irctextformat
