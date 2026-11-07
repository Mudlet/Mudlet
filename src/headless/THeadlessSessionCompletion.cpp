/***************************************************************************
 *   Copyright (C) 2026 by Mudlet Developers                               *
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

// What the GUI's command line completes as the player types

#include "THeadlessSession.h"

#include "Host.h"
#include "TNullConsoleFrontend.h"

#include <QSet>

namespace {
const TNullConsoleFrontend* nullView(const QPointer<Host>& pHost)
{
    return pHost ? dynamic_cast<const TNullConsoleFrontend*>(pHost->consoleFrontend()) : nullptr;
}

QStringList sorted(const QSet<QString>& words)
{
    QStringList list(words.cbegin(), words.cend());
    list.sort();
    return list;
}
} // namespace

QStringList THeadlessSession::commandLineSuggestions() const
{
    const TNullConsoleFrontend* pView = nullView(mpHost);
    return pView ? sorted(pView->mainCommandLineSuggestions()) : QStringList();
}

QStringList THeadlessSession::commandLineBlacklist() const
{
    const TNullConsoleFrontend* pView = nullView(mpHost);
    return pView ? sorted(pView->mainCommandLineBlacklist()) : QStringList();
}
