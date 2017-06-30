############################################################################
#    Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         #
#                                                                          #
#    This program is free software; you can redistribute it and/or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

# This is a Mudlet created qmake project "include" file which the Qt IDE treats
# as a sub-project - it's purpose is to group the three sub-modules needed from
# the Communi (IRC) project libraries into one group in the editor...

# To generate the files in this sub-project it is necessary to copy sets of
# files (sub-directories) from:
# https://github.com/communi/libcommuni into the Mudlet source tree as:
# "/src/core/*"  ==> "/3rdparty/communi/src/core/*"
# "/src/model/*" ==> "/3rdparty/communi/src/model/*"
# "/src/util/*"  ==> "/3rdparty/communi/src/util/*"

# The effective contents of this file is - at the time of creation - exactly the
# same as the "libcommuni/src/src.pri" in the upstream (libcommuni) source but
# is not called "src.pri" so that it can be identified with a specific rather
# than a generic name {Qt tends to call a primary project file with the same
# name as the directory it creates at the same time to put it in - which is fine
# until you have several sub-projects and they all use "src" to hold their
# source code in....}

# To determine the communi version check the #define value for IRC_VERSION_STR
# in 3rdparty/communi/include/IrcCore/ircglobal.h - at the time of creation of
# this file it was "3.5.0"

include(src/core/core.pri)
include(src/model/model.pri)
include(src/util/util.pri)
