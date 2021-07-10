/***************************************************************************
 *   Copyright (C) 2020 by Gustavo Sousa - gustavocms@gmail.com            *
 *   Copyright (C) 2020 by Stephen Lyons - slysven@virginmedia.com         *
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

#include "TMxpTagParser.h"
#include "TMxpNodeBuilder.h"
#include "TStringUtils.h"

#include "pre_guard.h"
#include <QDebug>
#include "post_guard.h"

QList<QSharedPointer<MxpNode>> TMxpTagParser::parseToMxpNodeList(const QString& tagText, bool ignoreText)
{
    TMxpNodeBuilder nodeBuilder(ignoreText);
    std::string tagStdStr = tagText.toStdString();

    QList<QSharedPointer<MxpNode>> result;
    for (auto itr = tagStdStr.begin(); itr != tagStdStr.end(); itr++) {
        if (nodeBuilder.accept(*itr)) {
            result.append(QSharedPointer<MxpNode>(nodeBuilder.buildNode()));
            --itr;
        }
    }

    if (nodeBuilder.hasNode()) {
        result.append(QSharedPointer<MxpNode>(nodeBuilder.buildNode()));
    }

    return result;
}