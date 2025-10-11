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
#ifndef MUDLET_TMXPNODEBUILDER_H
#define MUDLET_TMXPNODEBUILDER_H

#include "MxpTag.h"
#include "TStringUtils.h"

class TMxpNodeBuilder
{
    bool mOptionIgnoreText;

    // current tag attrs
    std::string mCurrentTagName;
    QList<MxpTagAttribute> mCurrentTagAttrs;
    bool mIsEndTag = false;
    bool mIsEmptyTag = false;
    // parsing tag state
    bool mIsInsideTag = false;

    // current attr
    std::string mCurrentAttrName;
    std::string mCurrentAttrValue;
    // parsing attr state
    bool mIsInsideAttr = false;
    bool mReadingAttrValue = false;

    // text sequence state
    bool mIsInsideSequence = false;
    bool mIsQuotedSequence = false;
    char mOpeningQuote = '\0';
    bool mSequenceHasSpaces = false;
    bool mHasSequence = false;

    // current text node
    std::string mCurrentText;

    // text node parsing state
    bool mIsInsideText = false;

    // overall processing state
    bool mHasNode = false; // a node is ready to be consumed
    bool mIsText = false;  // the current node is a text node

    bool acceptTag(char ch);
    void resetCurrentTag();

    bool acceptAttribute(char ch);
    void resetCurrentAttribute();

    bool acceptSequence(char ch, std::string& buffer);
    void resetCurrentSequence();
    void processAttribute();

public:
    explicit TMxpNodeBuilder(bool ignoreText = false);

    // returns true when a node (text/tag start/tag end) is available
    // the same char has to be input again when a match is found as it may be a boundary
    // and the class keeps no buffer of it
    bool accept(char ch);

    MxpNode* buildNode();
    MxpTag* buildTag();

    void reset();

    inline bool hasTag() const { return isTag() && hasNode(); }

    inline bool hasNode() const { return mHasNode; }

    inline bool isInsideTag() const { return mIsInsideTag; }

    inline bool isTag() const { return !mIsText; }

    inline bool isText() const { return mIsText; }
};
#endif //MUDLET_TMXPNODEBUILDER_H
