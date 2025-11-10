#ifndef MUDLET_GIFUNIT_H
#define MUDLET_GIFUNIT_H

/***************************************************************************
 *   Copyright (C) 2023-2023 by Adam Robinson - seldon1951@hotmail.com     *
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


#include <QMovie>
#include <QPointer>
#include <QString>

#include <list>

class Host;
class TLabel;


class GifTracker
{

public:
    GifTracker() = default;
    explicit GifTracker(Host* pHost)
    : mpHost(pHost)
    {}

    bool registerGif(QMovie* pT);
    void unregisterGif(QMovie* pT);
    std::tuple<QString, int, int> assembleReport();


private:
    //GifTracker() = default;
    QPointer<Host> mpHost;
    std::list<QMovie*> mMovieList;
};

#endif // MUDLET_GIFUNIT_H
