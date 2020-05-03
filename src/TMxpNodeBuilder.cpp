/***************************************************************************
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
#include "TMxpNodeBuilder.h"
#include "TMxpTagParser.h"
#include "TStringUtils.h"

bool TMxpNodeBuilder::accept(char ch)
{
    if (mIsInsideTag) { // inside tag
        mCurrentText.clear();
        mIsText = false;

        if (!acceptTag(ch)) {
            return false;
        } else {
            mIsInsideTag = false;
            return true;
        }
    } else if (ch == '<') { // start tag
        if (mIsInsideText) {
            mIsInsideText = false;
            mIsText = true;
            mHasNode = true;
            return true;
        } else {              // second call
            mHasNode = false; //mIsText = false
            return acceptTag(ch);
        }
    } else if (!mOptionIgnoreText) { // text
        mIsInsideText = true;
        mCurrentText.append(ch);
        return false;
    }

    return false;
}
bool TMxpNodeBuilder::acceptTag(char ch)
{
    if (mHasNode) {
        mHasNode = false;
        return true;
    }

    if (mIsInsideAttr) {
        if (!acceptAttribute(ch)) {
            return false;
        } else {
            if (!mCurrentAttrName.isEmpty()) {
                processAttribute();
            }
            resetCurrentAttribute();
        }
    }

    if (QChar(ch).isSpace())
        return false;

    if (ch == '<') { // reset
        resetCurrentTag();
        mIsInsideTag = true;
        return false;
    }

    if (ch == '/') {
        mIsEndTag = mCurrentTagName.isEmpty();
        mIsEmptyTag = !mIsEndTag;
        return false;
    }

    if (ch == '>') {
        processAttribute();
        mHasNode = true;
        return false;
    }

    if (!acceptAttribute(ch))
        return false;

    return false;
}
void TMxpNodeBuilder::processAttribute()
{
    if (mCurrentTagName.isEmpty()) {
        mCurrentTagName = mCurrentAttrName;
    } else if (!mCurrentAttrName.isEmpty()) {
        mCurrentTagAttrs.append(MxpTagAttribute(mCurrentAttrName, mCurrentAttrValue));
    }
}
void TMxpNodeBuilder::resetCurrentTag()
{
    mHasNode = false;
    mIsEndTag = false;
    mIsEmptyTag = false;
    mIsInsideTag = false;
    mCurrentTagName.clear();
    mCurrentTagAttrs.clear();

    resetCurrentAttribute();
}
bool TMxpNodeBuilder::acceptAttribute(char ch)
{
    mIsInsideAttr = true;

    QString& buffer = mReadingAttrValue ? mCurrentAttrValue : mCurrentAttrName;
    if (!acceptSequence(ch, buffer)) {
        return false;
    }

    resetCurrentSequence();

    if (ch == '=') {
        mReadingAttrValue = true;
        return false;
    } else {
        return true;
    }
}
void TMxpNodeBuilder::resetCurrentAttribute()
{
    mCurrentAttrName.clear();
    mCurrentAttrValue.clear();

    mReadingAttrValue = false;

    mIsInsideAttr = false;
    resetCurrentSequence();
}
bool TMxpNodeBuilder::acceptSequence(char ch, QString& buffer)
{
    if (mHasSequence) {
        mHasSequence = false;
        return true;
    }

    if (TStringUtils::isQuote(ch)) {
        if (!mIsInsideSequence) {
            mIsInsideSequence = true;
            mIsQuotedSequence = true;
            mOpeningQuote = ch;
            return false;
        } else if (mIsQuotedSequence && ch == mOpeningQuote) {
            mHasSequence = true;
            return false;
        }
    }

    if (!mIsQuotedSequence) {
        if (QChar(ch).isSpace()) {
            mHasSequence = true;
            return false;
        } else if (ch == '>' || ch == '/' || ch == '=') {
            return true;
        }
    }

    if (mIsQuotedSequence && !mSequenceHasSpaces && ch == '=') {
        return true;
    }

    if (!mIsInsideSequence) {
        mIsInsideSequence = true;
    }

    mSequenceHasSpaces = mSequenceHasSpaces || QChar(ch).isSpace();

    buffer.append(ch);
    return false;
}
void TMxpNodeBuilder::resetCurrentSequence()
{
    mSequenceHasSpaces = false;
    mIsQuotedSequence = false;
    mIsInsideSequence = false;
    mHasSequence = false;
}
MxpTag* TMxpNodeBuilder::buildTag()
{
    MxpTag* result = mIsEndTag ? static_cast<MxpTag*>(new MxpEndTag(mCurrentTagName)) : static_cast<MxpTag*>(new MxpStartTag(mCurrentTagName, mCurrentTagAttrs, mIsEmptyTag));
    resetCurrentTag();

    return result;
}
MxpNode* TMxpNodeBuilder::buildNode()
{
    MxpNode* node = mIsText ? static_cast<MxpNode*>(new MxpTextNode(mCurrentText)) : static_cast<MxpNode*>(buildTag());
    mCurrentText.clear();
    mHasNode = false;
    return node;
}
void TMxpNodeBuilder::reset()
{
    resetCurrentTag();
    mCurrentText.clear();
}
