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
#ifndef MUDLET_TCOLORSCHEME_H
#define MUDLET_TCOLORSCHEME_H

#include "pre_guard.h"
#include <QColor>
#include "post_guard.h"


class TColorScheme
{
public:
    TColorScheme();

    QColor mBlack;
    QColor mLightBlack;
    QColor mRed;
    QColor mLightRed;
    QColor mLightGreen;
    QColor mGreen;
    QColor mLightBlue;
    QColor mBlue;
    QColor mLightYellow;
    QColor mYellow;
    QColor mLightCyan;
    QColor mCyan;
    QColor mLightMagenta;
    QColor mMagenta;
    QColor mLightWhite;
    QColor mWhite;

    const QColor& getColorFromAnsi(int ansiColor) const;

    bool setColor(quint8 colorNumber, const QColor& Color);
    virtual void reset();
};


#endif //MUDLET_TCOLORSCHEME_H
