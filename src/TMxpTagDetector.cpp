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
#include "TMxpTagDetector.h"

bool TMxpTagDetector::handle(char& ch, size_t& localBufferPosition)
{
    // ignore < and > inside of parameter strings
    if (openT == 1) {
        if (ch == '\'' || ch == '\"') {
            if (!mParsingVar) {
                mOpenMainQuote = ch;
                mParsingVar = true;
            } else {
                if (ch == mOpenMainQuote) {
                    mParsingVar = false;
                }
            }
        }
    }

    if (ch == '<') {
        if (!mParsingVar) {
            ++openT;
            if (!currentToken.empty()) {
                currentToken += ch;
            }
            mAssemblingToken = true;
            ++localBufferPosition;
            return true;
        }
    }

    if (ch == '>') {
        if (!mParsingVar) {
            ++closeT;
        }

        // sanity check
        if (closeT > openT) {
            localBufferPosition++;

            reset();
            return true;
        }

        if ((openT > 0) && (closeT == openT)) {
            localBufferPosition++;

            mAssemblingToken = false;
            mIsTokenAvailable = true;
            return false;
        }
    }

    if (mAssemblingToken) {
        if (ch == '\n') {
            closeT = 0;
            openT = 0;
            mAssemblingToken = false;
            currentToken.clear();
            mParsingVar = false;
        } else {
            currentToken += ch;
            ++localBufferPosition;
            return true;
        }
    }

    return false;
}

void TMxpTagDetector::reset()
{
    openT = 0;
    closeT = 0;
    mIsTokenAvailable = false;
    mAssemblingToken = false;
    mParsingVar = false;

    currentToken.clear();
}

bool TMxpTagDetector::hasToken() const
{
    return mIsTokenAvailable;
}
std::string TMxpTagDetector::getToken()
{
    std::string result = currentToken;
    reset();
    return result;
}
