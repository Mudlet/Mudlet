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

bool TMxpTagDetector::handle(char ch)
{
    // ignore < and > inside of parameter strings
    if (mOpenTagCount == 1) {
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
            ++mOpenTagCount;
            mCurrentToken += ch;
            mAssemblingToken = true;
            return true;
        }
    }

    if (ch == '>') {
        if (!mParsingVar) {
            ++mCloseTagCount;
        }

        // sanity check
        if (mCloseTagCount > mOpenTagCount) {

            reset();
            return true;
        }

        if ((mOpenTagCount > 0) && (mCloseTagCount == mOpenTagCount)) {

            mCurrentToken += ch;
            mAssemblingToken = false;
            mIsTokenAvailable = true;
            return false;
        }
    }

    if (mAssemblingToken) {
        if (ch == '\n') {
            mCloseTagCount = 0;
            mOpenTagCount = 0;
            mAssemblingToken = false;
            mCurrentToken.clear();
            mParsingVar = false;
        } else {
            mCurrentToken += ch;
            return true;
        }
    }

    return false;
}

void TMxpTagDetector::reset()
{
    mOpenTagCount = 0;
    mCloseTagCount = 0;
    mIsTokenAvailable = false;
    mAssemblingToken = false;
    mParsingVar = false;

    mCurrentToken.clear();
}

bool TMxpTagDetector::hasToken() const
{
    return mIsTokenAvailable;
}
std::string TMxpTagDetector::getToken()
{
    std::string result = mCurrentToken;
    reset();
    return result;
}
