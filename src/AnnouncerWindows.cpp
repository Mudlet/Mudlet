/***************************************************************************
*   Copyright (C) 2022 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "Announcer.h"

#include <QDebug>

Announcer::Announcer(QObject *parent)
: QObject{parent}
{

}

void Announcer::announce(const QString text)
{
    qDebug() << "announcing" << text;
    // check:
    //FireWinAccessibilityEvent(EVENT_OBJECT_LIVEREGIONCHANGED, node); // MSAA
    //FireUiaAccessibilityEvent(UIA_LiveRegionChangedEventId, node); // UIA
        // UiaRaiseAutomationEvent

    // also:
    // on Windows we need to fire IA2_EVENT_TEXT_INSERTED and IA2_EVENT_TEXT_REMOVED events individually on each affected node within the changed region, with additional attributes like “container-live:polite” to indicate that the affected node was part of a live region
}
