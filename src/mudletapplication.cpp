/***************************************************************************
 *   Copyright (C) 2019-2013 by Vadim Peretokin - vperetokin@gmail.com     *
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


#include "mudletapplication.h"
#include "mudlet.h"
#include <QFileOpenEvent>

MudletApplication::MudletApplication(int &argc, char **argv) :
        QApplication(argc, argv)
{
}

bool MudletApplication::event(QEvent *event)
{
    if (event->type() == QEvent::FileOpen) {
        auto openEvent = static_cast<QFileOpenEvent*>(event);
        // macOS tends to send blank "" urls on launch
        if (openEvent->url().scheme() != QString()) {
            mudlet::self()->handleTelnetUri(openEvent->url());
        }
        return true;
    }
    return QApplication::event(event);
}
