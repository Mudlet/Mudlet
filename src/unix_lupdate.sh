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

# Script for Unix/Linux systems (possible NOT Macs) to update translation
# files accomodating need to treat en_US one specially.

# Intending to be called from the QMake project file it needs one command
# line argument which is available from that place as:
# "%{CurrentProject:QTINSTALL_BINS}" which is the "path to the currently
# selected Qt Kit's ex;ecutables - and is needed to find and use the right
# "lupdate" executable

# NOTE: THIS SCRIPT DOES NOT HANDLE MAKING NEW TRANSLATIONS, I.E. WILL NOT
# WORK ON AN EMPTY FILE EVEN THOUGH IT HAS THE CORRECT NAME; INSTEAD TO
# CREATE A NEW TRANSLATION FOR xx (LANGUAGE ONLY, NO COUNTRY SPECIFIER) or
# xx_YY (LANGUAGE AND CONTRY) INVOKE ANY (YOU CAN USE YOUR SYSTEM'S DEFAULT)
# lupdate IN THE ./src/ DIRECTORY, IN THE FOLLOWING WAY:
#
#     lupdate . -ts ./mudlet_xx.ts -recursive
# or
#     lupdate . -ts ./mudlet_xx_YY.ts -recursive
#
# THE GENERATED FILE WILL THAN HAVE THE REQUIRED SKELATAL CONTENTS TO WORK
# WITH THIS SCRIPT

if [ $# -ne 1 ]
then echo "Usage: unix_lupdate.sh /path/to/QtBin/lupdate
    /path/to/QtBin/lupdate  a path to the particular Qt lupdate executable to
                            use, is the (only) argument required/allowed."
    exit 1
else if [ ! -x ${1}/lupdate ]
     then echo "Error: ${1}/ does not appear to be a path to a Qt lupdate executable"
         exit 2
     fi
fi

TRANSLATIONFILES=$(ls ./mudlet_*.ts)

for TRANSLATIONFILE in ${TRANSLATIONFILES}
do
    LANGUAGECODE=${TRANSLATIONFILE##./mudlet_}
    LANGUAGECODE=${LANGUAGECODE%%.ts}
    echo "Processing ${LANGUAGECODE} ..."
    if [ ${LANGUAGECODE} = "en_US" ]
    then ${1}/lupdate . -pluralonly -ts ${TRANSLATIONFILE} -recursive
        echo "... done (plurals only) ${LANGUAGECODE}"
    else ${1}/lupdate . -ts ${TRANSLATIONFILE} -recursive
        echo "... done ${LANGUAGECODE}"
    fi
done
