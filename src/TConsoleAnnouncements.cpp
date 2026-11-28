/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include "TConsoleAnnouncements.h"

#include "Host.h"
#include "TBuffer.h"
#include "mudlet.h"

#include <QAccessible>

namespace consoleAnnouncements {

bool wanted(const Host& host, const bool isMainConsole)
{
    return QAccessible::isActive() && isMainConsole && host.mAnnounceIncomingText && mudlet::self()->getActiveHost() == &host;
}

void announceNewLines(QObject* view, const TBuffer& buffer, const int previousLastLine, const int lastLine)
{
    QString newLines;

    // content has been deleted
    if (previousLastLine > lastLine) {
        QAccessibleTextInterface* ti = QAccessible::queryAccessibleInterface(view)->textInterface();
        if (!ti) {
            return;
        }
        auto totalCharacterCount = ti->characterCount();
        ti->setCursorPosition(totalCharacterCount);
        QAccessibleTextRemoveEvent event(view, totalCharacterCount, QString());
        QAccessible::updateAccessibility(&event);
        return;
    }

    // content has been added
    for (int i = previousLastLine; i < lastLine; i++) {
        newLines.append(buffer.spokenLine(i));
        newLines.append('\n');
    }

    newLines = newLines.trimmed();
    if (newLines.isEmpty()) {
        return;
    }

    mudlet::self()->announce(newLines, QString(), true);
}

} // namespace consoleAnnouncements
