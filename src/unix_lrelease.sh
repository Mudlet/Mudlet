#!/bin/sh

###########################################################################
#                                                                         #
#   Copyright (C) 2017 by Stephen Lyons - slysven@virginmedia.com         #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
#                                                                         #
###########################################################################

# Script for Unix/Linux systems (possible NOT Macs) to produce binary
# translation files accommodating need to mark unfinished translations
# specially.

# Intending to be called from the QMake project file it needs one command
# line argument which is available from that place as:
# "%{CurrentProject:QTINSTALL_BINS}" which is the "path to the currently
# selected Qt Kit's ex;ecutables - and is needed to find and use the right
# "lupdate" executable

DELIMITER="#"

DELIMITOPTION="-markuntranslated"

case $# in
    2) DELIMITER=${2}
        if [ X${DELIMITER} = X ]
        then DELIMITOPTION=""
        fi ;;
    1) ;;
    *) echo "Usage: unix_lrelease.sh /path/to/QtBin/lrelease  [marker]
    /path/to/QtBin/lrelease  a path to the particular Qt lrelease executable to
                             use, is a required argument.
    marker                   (optional) prefix to apply to all places where the
                             translation is marked as unfinished (and where the
                             source text is used instead with that prefix.
                             This defaults to \"#\" (as does Qt Linguist) use
                             \"\" to suppress this."
    exit 1 ;;
esac

if [ ! -x ${1}/lrelease ]
then echo "Error: ${1}/lrelease does not appear to be a path to a Qt lrelease executable"
    exit 2
fi

TRANSLATIONFILES=""

for TRANSLATIONFILE in $(ls ./mudlet_*.ts)
do
    LANGUAGECODE=${TRANSLATIONFILE##./mudlet_}
    TRANSLATIONFILES="${TRANSLATIONFILES} ${TRANSLATIONFILE}"
done

echo "Processing ..."
if [ -z ${DELIMITOPTION} ]
then ${1}/lrelease -compress -removeidentical ${TRANSLATIONFILES}
else ${1}/lrelease -compress -removeidentical ${DELIMITOPTION} "${DELIMITER}" ${TRANSLATIONFILES}
fi
echo "... done"
