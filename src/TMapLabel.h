#ifndef MUDLET_TMAPLABEL_H
#define MUDLET_TMAPLABEL_H

/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2014-2016, 2018-2022 by Stephen Lyons                   *
 *                                               - slysven@virginmedia.com *
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


#include "pre_guard.h"
#include <QtGlobal>
#include <QColor>
#include <QPixmap>
#include <QSizeF>
#include <QVector3D>
#include "post_guard.h"


class TMapLabel
{
public:
    // Try and deduce whether this label has data:
    bool isNull() const {return pos.isNull() && !size.isValid() && text.isEmpty() && pix.isNull(); }
    QByteArray base64EncodePixmap() const;

    QVector3D pos;
    // This gets initialised to (-1.0f, -1.0f) which is NOT QSizeF::isNull()
    // {That is (0.0f, 0.0f)} but is NOT QSizeF::isValid():
    QSizeF size;
    QSizeF clickSize;
    QString text;
    QColor fgColor  = QColorConstants::Black;
    QColor bgColor  = QColorConstants::Black;
    QPixmap pix;
    bool highlight = false;
    bool showOnTop = false;
    bool noScaling = false;
    bool temporary = false;
};

#endif // MUDLET_TMAPLABEL_H
