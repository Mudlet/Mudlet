
#ifndef MUDLET_MXPSUPPORT_CPP_TMXPTAGPARSER_H
#define MUDLET_MXPSUPPORT_CPP_TMXPTAGPARSER_H

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


#include "MxpTag.h"

#include "pre_guard.h"
#include <QString>
#include <QList>
#include <QSharedPointer>
#include "post_guard.h"

class TMxpTagParser
{
    static int readTextBlock(const QStringRef& str, int start, int end, QChar terminatingChar);

public:
    static QStringList parseToList(const QStringRef& tagText);
    static QStringList parseToList(const QString& tagText);

    QList<QSharedPointer<MxpNode>> parseToMxpNodeList(const QString& tagText, bool ignoreText = false) const;

    MxpTag* parseTag(const QString& tagText) const;
    MxpStartTag* parseStartTag(const QString& tagText) const;
    MxpEndTag* parseEndTag(const QString& tagText) const;

    MxpTagAttribute parseAttribute(const QString& attr) const;
};

#endif //MUDLET_MXPSUPPORT_CPP_TMXPTAGPARSER_H
