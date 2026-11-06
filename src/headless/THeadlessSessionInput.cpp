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

bool THeadlessSession::setReplayRecording(const bool record)
{
    if (!usable()) {
        return false;
    }
    cTelnet& telnet = mpHost->mTelnet;
    if (record == telnet.recordingReplay()) {
        if (record) {
            //: Error from a frontend with no window that was asked to record a replay while it was recording one already
            mErrorString = tr("A replay is being recorded already.");
        } else {
            //: Error from a frontend with no window that was asked to stop recording a replay while it was recording none
            mErrorString = tr("No replay is being recorded.");
        }
        return false;
    }
    // localText handlers run as the messages are printed, and may delete the session
    const QPointer<THeadlessSession> self(this);
    const bool done = telnet.toggleReplayRecording();
    if (!self || done) {
        return done;
    }
    if (record) {
        //: Error from a frontend with no window whose replay recording file could not be opened. %1 is the reason
        mErrorString = tr("The replay recording file could not be opened: %1").arg(telnet.replayRecordingErrorString());
    } else {
        //: Error from a frontend with no window whose replay recording stopped but could not be saved. %1 is the reason
        mErrorString = tr("The replay recording has stopped, but could not be saved: %1").arg(telnet.replayRecordingErrorString());
    }
    return false;
}

bool THeadlessSession::replayRecording() const
{
    return mpHost && mpHost->mTelnet.recordingReplay();
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
