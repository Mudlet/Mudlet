/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
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

#include "TEntityHandler.h"

bool TEntityHandler::handle(char& ch, std::string& localBuffer, size_t& localBufferPosition, size_t localBufferLength)
{
    if (ch == '&' || mIgnoreTag) {
        if ((localBufferPosition + 4 < localBufferLength) && (mSkip.empty())) {
            if (localBuffer.substr(localBufferPosition, 4) == "&gt;") {
                localBufferPosition += 3;
                ch = '>';
                mIgnoreTag = false;
            } else if (localBuffer.substr(localBufferPosition, 4) == "&lt;") {
                localBufferPosition += 3;
                ch = '<';
                mIgnoreTag = false;
            } else if (localBuffer.substr(localBufferPosition, 5) == "&amp;") {
                mIgnoreTag = false;
                localBufferPosition += 4;
                ch = '&';
            } else if (localBuffer.substr(localBufferPosition, 6) == "&quot;") {
                localBufferPosition += 5;
                mIgnoreTag = false;
                mSkip.clear();
                ch = '"';
            }
        } else if (mSkip == "&gt" && ch == ';') { // if the content is split across package borders
            mIgnoreTag = false;
            mSkip.clear();
            ch = '>';
        } else if (mSkip == "&lt" && ch == ';') {
            mIgnoreTag = false;
            mSkip.clear();
            ch = '<';
        } else if (mSkip == "&amp" && ch == ';') {
            mIgnoreTag = false;
            mSkip.clear();
            ch = '&';
        } else if (mSkip == "&quot" && ch == ';') {
            mIgnoreTag = false;
            mSkip.clear();
            ch = '"';
        } else {
            mIgnoreTag = true;
            mSkip += ch;
            // sanity check
            if (mSkip.size() > 7) {
                mIgnoreTag = false;
                mSkip.clear();
            }
            ++localBufferPosition;
            return true;
        }
    }

    return false;
}