/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2021 by Florian Scheel - keneanung@gmail.com            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

// Text Formatting 'Counters', version, style by Michael Weller, michael.weller@t-online.de


#include "TMxpStubClient.h"


// Handle 'stacks' of attribute settings:
void TMxpStubClient::setBold(bool bold)
{
    if (bold) {
        boldCtr++;
    } else if (boldCtr > 0){
        boldCtr--;
    }
}

void TMxpStubClient::setItalic(bool italic)
{
    if (italic) {
        italicCtr++;
    } else if (italicCtr > 0){
        italicCtr--;
    }
}

void TMxpStubClient::setUnderline(bool underline)
{
    if (underline) {
        underlineCtr++;
    } else if (underlineCtr > 0){
        underlineCtr--;
    }
}

void TMxpStubClient::setStrikeOut(bool strikeOut)
{
    if (strikeOut) {
        strikeOutCtr++;
    } else if (strikeOutCtr > 0){
        strikeOutCtr--;
    }
}
