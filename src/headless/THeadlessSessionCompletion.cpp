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

std::optional<QString> THeadlessSession::completeCommandLine(const bool forward)
{
    if (!usable()) {
        return std::nullopt;
    }
    if (mpHost->passwordEntryWanted()) {
        //: Error from a frontend with no window that was asked to complete the word on its input line while the game hides what the player types, as for a password
        mErrorString = tr("Nothing is completed while the game hides what is typed.");
        return std::nullopt;
    }
    auto* pView = dynamic_cast<TNullConsoleFrontend*>(mpHost->consoleFrontend());
    std::optional<QString> line = pView ? pView->tabCompleteMainCommandLine(forward) : std::nullopt;
    if (!line) {
        //: Error from a frontend with no window that was asked to complete the word on its input line, as Tab does, when no word the game sent recently or a script suggested completes it
        mErrorString = tr("No word completes what is typed.");
    }
    return line;
}
