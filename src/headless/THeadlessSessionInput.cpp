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

// What the GUI's main window and console toolbar give the player and Lua cannot do the same way

#include "THeadlessSession.h"

#include "Host.h"
#include "HostManager.h"
#include "TEvent.h"

#include <QPointer>

bool THeadlessSession::setScriptsStopped(const bool stopped)
{
    if (!usable()) {
        return false;
    }
    // Again while stopped too, for what a script, resetProfile() or a package has turned on since
    if (stopped) {
        mpHost->stopAllTriggers();
    } else if (mpHost->mEmergencyStop) {
        mpHost->reenableAllTriggers();
    }
    return true;
}

bool THeadlessSession::scriptsStopped() const
{
    return mpHost && mpHost->mEmergencyStop;
}

bool THeadlessSession::setApplicationActive(const bool active)
{
    if (refused()) {
        return false;
    }
    if (active == mApplicationActive) {
        return true;
    }
    mApplicationActive = active;
    const unsigned int change = ++mApplicationActiveChanges;
    if (!mpPool) {
        return true;
    }
    TEvent event{};
    event.mArgumentList << QLatin1String("sysApplicationFocusChangeEvent") << (active ? QLatin1String("1") : QLatin1String("0"));
    event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_BOOLEAN;
    // A handler may open or close a profile, or delete the session
    QList<QPointer<Host>> profiles;
    for (const auto& pHost : mpPool->hostList()) {
        profiles.append(pHost.data());
    }
    const QPointer<THeadlessSession> self(this);
    for (const QPointer<Host>& pHost : std::as_const(profiles)) {
        // A handler that changed it again has told every profile already
        if (!self || mApplicationActiveChanges != change) {
            break;
        }
        if (pHost && pHost->mLoadedOk && !pHost->closeStarted()) {
            pHost->raiseEvent(event);
        }
    }
    return true;
}
