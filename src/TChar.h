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

#ifndef MUDLET_TBUFFER_CPP_TCHAR_H
#define MUDLET_TBUFFER_CPP_TCHAR_H

#include "pre_guard.h"
#include <QDebug>
#include <QColor>
#include "post_guard.h"

class TColorSettings;

class TChar
{
    friend class TBuffer;

public:
    enum AttributeFlag {
        None = 0x0,
        // Replaces TCHAR_BOLD 2
        Bold = 0x1,
        // Replaces TCHAR_ITALICS 1
        Italic = 0x2,
        // Replaces TCHAR_UNDERLINE 4
        Underline = 0x4,
        // New, TCHAR_OVERLINE had not been previous done, is
        // ANSI CSI SGR Overline (53 on, 55 off)
        Overline = 0x8,
        // Replaces TCHAR_STRIKEOUT 32
        StrikeOut = 0x10,
        // NOT a replacement for TCHAR_INVERSE, that is now covered by the
        // separate isSelected bool but they must be EX-ORed at the point of
        // painting the Character
        Reverse = 0x20,
        // The attributes that are currently user settable and what should be
        // consider in HTML generation:
        TestMask = 0x3f,
        // Replaces TCHAR_ECHO 16
        Echo = 0x100
    };
    Q_DECLARE_FLAGS(AttributeFlags, AttributeFlag)

    TChar();
    TChar(const QColor& fg, const QColor& bg, TChar::AttributeFlags flags = TChar::None, const int linkIndex = 0);
    TChar(const TChar&);
    TChar(TColorSettings &colors, AttributeFlags attributeFlags = TChar::None);

    static TChar createTransparent(const QColor& bgColor, AttributeFlags attributeFlags);

    bool operator==(const TChar&);
    void setColors(const QColor& newForeGroundColor, const QColor& newBackGroundColor) {
        mFgColor = newForeGroundColor;
        mBgColor = newBackGroundColor;
    }
    // Only considers the following flags: Bold, Italic, Overline, Reverse,
    // Strikeout, Underline, does not consider Echo:
    void setAllDisplayAttributes(const AttributeFlags newDisplayAttributes) { mFlags = (mFlags & ~TestMask) | (newDisplayAttributes & TestMask); }
    void setForeground(const QColor& newColor) { mFgColor = newColor; }
    void setBackground(const QColor& newColor) { mBgColor = newColor; }
    void setTextFormat(const QColor& newFgColor, const QColor& newBgColor, const AttributeFlags newDisplayAttributes) {
        setColors(newFgColor, newBgColor);
        setAllDisplayAttributes(newDisplayAttributes);
    }

    const QColor& foreground() const { return mFgColor; }
    const QColor& background() const { return mBgColor; }
    AttributeFlags allDisplayAttributes() const { return mFlags & TestMask; }
    void select() { mIsSelected = true; }
    void deselect() { mIsSelected = false; }
    bool isSelected() const { return mIsSelected; }
    int linkIndex () const { return mLinkIndex; }

private:
    QColor mFgColor;
    QColor mBgColor;
    AttributeFlags mFlags;
    // Kept as a separate flag because it must often be handled separately
    bool mIsSelected;
    int mLinkIndex;

};
Q_DECLARE_OPERATORS_FOR_FLAGS(TChar::AttributeFlags)

#ifndef QT_NO_DEBUG_STREAM
// Dumper for the TChar::AttributeFlags - so that qDebug gives a detailed broken
// down results when presented with the value rather than just a hex value.
// Note "inline" is REQUIRED:
inline QDebug& operator<<(QDebug& debug, const TChar::AttributeFlags& attributes)
{
    QDebugStateSaver saver(debug);
    QString result = QLatin1String("TChar::AttributeFlags(");
    QStringList presentAttributes;
    if (attributes & TChar::Bold) {
        presentAttributes << QLatin1String("Bold (0x01)");
    }
    if (attributes & TChar::Italic) {
        presentAttributes << QLatin1String("Italic (0x02)");
    }
    if (attributes & TChar::Underline) {
        presentAttributes << QLatin1String("Underline (0x04)");
    }
    if (attributes & TChar::Overline) {
        presentAttributes << QLatin1String("Overline (0x08)");
    }
    if (attributes & TChar::StrikeOut) {
        presentAttributes << QLatin1String("StrikeOut (0x10)");
    }
    if (attributes & TChar::Reverse) {
        presentAttributes << QLatin1String("Reverse (0x20)");
    }
    if (attributes & TChar::Echo) {
        presentAttributes << QLatin1String("Echo (0x100)");
    }
    result.append(presentAttributes.join(", "));
    result.append(QLatin1String(")"));
    debug.nospace() << result;
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif //MUDLET_TBUFFER_CPP_TCHAR_H
