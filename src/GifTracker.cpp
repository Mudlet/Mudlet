/***************************************************************************
 *   Copyright (C) 2023 by Adam Robinson - seldon1951@hotmail.com          *
 *   Copyright (C) 2026 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "GifTracker.h"

#include "Host.h"

/* We need an explict constructor in this file as the Host class is forward
 * declared in the header file and it is problematic to define any deferencing
 * of it there:*/
GifTracker::GifTracker(Host* pHost)
: mpHost(pHost)
{
}

bool GifTracker::registerGif(QMovie* pT)
{
    if (!pT) {
        return false;
    }

    mMovieList.push_back(pT);
    return true;
}

void GifTracker::unregisterGif(QMovie* pT)
{
    if (!pT) {
        return;
    }

    mMovieList.remove(pT);
    return;
}

std::tuple<QString, int, int> GifTracker::assembleReport()
{
    int statsItemsTotal = 0;
    int statsActiveItems = 0;
    for (auto pItem : mMovieList) {
        ++statsItemsTotal;
        if (pItem->state() == QMovie::Running) {
            ++statsActiveItems;
        }
    }
    QStringList msg;
    msg << QLatin1String("Gifs current total: ") << QString::number(statsItemsTotal) << QLatin1String("\n") << QLatin1String("active Gifs: ") << QString::number(statsActiveItems)
        << QLatin1String("\n");
    return {msg.join(QString()), statsItemsTotal, statsActiveItems};
}
