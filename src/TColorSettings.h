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

#ifndef MUDLET_COLORSETTINGS_H
#define MUDLET_COLORSETTINGS_H

#include "TColorScheme.h"

class TColorSettings : public TColorScheme
{
public:
    // These three replace three sets of three integers that were used to hold
    // colour components during the parsing of SGR sequences, they were called:
    // fgColor{R|G|B}, fgColorLight{R|G|B} and bgColor{R|G|B} apart from
    // anything else, the first and last sets had the same names as arguments
    // to several of the methods which meant the latter shadowed and masked
    // them off!
    QColor mFgColor;
    QColor mFgColorLight;
    QColor mBgColor;

    TColorSettings();
    TColorSettings(const TColorSettings& other);

    void updateColors(const QColor& fg, const QColor& bg);

    virtual void reset();
};

#endif //MUDLET_COLORSETTINGS_H
