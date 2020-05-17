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

#ifndef MUDLET_TBUFFERSTYLE_H
#define MUDLET_TBUFFERSTYLE_H

// #define DEBUG_SGR_PROCESSING
// Define this to get qDebug() messages about the decoding of ANSI OSC sequences:

#include "TChar.h"
#include "TColorSettings.h"
class TBufferStyle : public TColorSettings
{
public:
    TBufferStyle();
    explicit TBufferStyle(const TColorSettings& colorSettings);

    bool mBold;
    bool mItalics;
    bool mOverline;
    bool mReverse;
    bool mStrikeOut;
    bool mUnderline;
    bool mItalicBeforeBlink;

    bool mIsDefaultColor;

    TChar::AttributeFlags getTCharFlags() const;
    TChar createChar() const;
    TChar createTransparent() const;

    void updateColorSettings(const TColorSettings &colors);
    void decodeSGR38(const QStringList& parameters, bool isColonSeparated);
    void decodeSGR48(const QStringList& parameters, bool isColonSeparated);
    void decodeSGR(const QString& sequence, bool haveColorSpaceId, const TColorSettings& hostColorSettings);

    static QColor from6x6x6(int tag);
};


#endif //MUDLET_TBUFFERSTYLE_H
