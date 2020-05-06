/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2018 by Stephen Lyons - slysven@virginmedia.com    *
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

#include "TChar.h"
#include "Host.h"

// Default constructor:
TChar::TChar()
: mFgColor(Qt::white)
, mBgColor(Qt::black)
, mFlags(None)
, mIsSelected(false)
, mLinkIndex(0)
{
}

TChar::TChar(const QColor& fg, const QColor& bg, const TChar::AttributeFlags flags, const int linkIndex)
: mFgColor(fg)
, mBgColor(bg)
, mFlags(flags)
, mIsSelected(false)
, mLinkIndex(linkIndex)
{
}

TChar::TChar(Host* pH)
: mFlags(None)
, mIsSelected(false)
, mLinkIndex(0)
{
    if (pH) {
        mFgColor = pH->mFgColor;
        mBgColor = pH->mBgColor;
    } else {
        mFgColor = Qt::white;
        mBgColor = Qt::black;
    }
}

// Note: this operator compares ALL aspects of 'this' against 'other' which may
// not be wanted in every case:
bool TChar::operator==(const TChar& other)
{
    if (mIsSelected != other.mIsSelected) {
        return false;
    }
    if (mLinkIndex != other.mLinkIndex) {
        return false;
    }
    if (mFgColor != other.mFgColor) {
        return false;
    }
    if (mBgColor != other.mBgColor) {
        return false;
    }
    if (mFlags != other.mFlags) {
        return false;
    }
    return true;
}

// Copy constructor:
TChar::TChar(const TChar& copy)
: mFgColor(copy.mFgColor)
, mBgColor(copy.mBgColor)
, mFlags(copy.mFlags)
, mIsSelected(false)
, mLinkIndex(copy.mLinkIndex)
{
}