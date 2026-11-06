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
#include "MudletMedia.h"
#include "TEvent.h"

#include <QFileInfo>
#include <QMimeData>
#include <QPoint>
#include <QPointer>
#include <QUrl>

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

bool THeadlessSession::dropOnMainConsole(const QMimeData& data, const QPoint& position)
{
    if (!usable()) {
        return false;
    }
    const bool hasText = data.hasText();
    if (!data.hasUrls() && !hasText) {
        //: Error from a frontend with no window that was given a drop with neither files, links nor text in it
        mErrorString = tr("The drop holds neither files nor text, which the main console does not take.");
        return false;
    }
    const QList<QUrl> urls = data.urls();
    const QString text = hasText ? data.text() : QString();
    // A handler may close the profile or delete the session
    const QPointer<THeadlessSession> self(this);
    const QPointer<Host> pHost(mpHost);
    const auto raise = [&self, &pHost, &position](const QString& name, const QString& item, const QString& kind) {
        if (!self || !pHost) {
            return;
        }
        TEvent event{};
        event.mArgumentList << name << item << kind << QString::number(position.x()) << QString::number(position.y()) << qsl("main");
        event.mArgumentTypeList << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_STRING << ARGUMENT_TYPE_NUMBER << ARGUMENT_TYPE_NUMBER << ARGUMENT_TYPE_STRING;
        pHost->raiseEvent(event);
    };
    for (const QUrl& url : urls) {
        const QString fileName = url.toLocalFile();
        if (const QFileInfo info(fileName); info.exists()) {
            raise(qsl("sysDropEvent"), fileName, info.suffix().trimmed());
        }
    }
    if (hasText) {
        if (const QUrl url(text); url.isValid()) {
            raise(qsl("sysDropUrlEvent"), url.toString(), url.scheme());
        }
    }
    return true;
}

void THeadlessSession::connectInputRelays(const unsigned int run)
{
    // Queued, as it is sent in the middle of telling every profile; the profiles its scripts opened outlive
    // the session's own, which leaves mRun as it was
    if (MudletMedia* pMedia = MudletMedia::self()) {
        connect(
                pMedia,
                &MudletMedia::signal_muteSet,
                this,
                [this, run](const bool apiNotGame, const bool muted) {
                    if (run == mRun && mpHost) {
                        emit soundsMutedChanged(apiNotGame ? SoundSource::scripts : SoundSource::game, muted);
                    }
                },
                Qt::QueuedConnection);
    }
}

bool THeadlessSession::setSoundsMuted(const SoundSource source, const bool muted)
{
    if (!usable()) {
        return false;
    }
    MudletMedia* pMedia = MudletMedia::self();
    if (!pMedia) {
        //: Error from a frontend with no window that was asked to mute or unmute sounds while nothing could play them
        mErrorString = tr("No sounds can be played, so none can be muted.");
        return false;
    }
    if (source == SoundSource::scripts) {
        pMedia->setApiMuted(muted);
    } else {
        pMedia->setGameMuted(muted);
    }
    return true;
}

bool THeadlessSession::toggleAllSoundsMuted()
{
    if (!usable()) {
        return false;
    }
    MudletMedia* pMedia = MudletMedia::self();
    if (!pMedia) {
        //: Error from a frontend with no window that was asked to mute or unmute sounds while nothing could play them
        mErrorString = tr("No sounds can be played, so none can be muted.");
        return false;
    }
    pMedia->toggleAllMuted();
    return true;
}

bool THeadlessSession::soundsMuted(const SoundSource source) const
{
    const MudletMedia* pMedia = mpHost ? MudletMedia::self() : nullptr;
    return pMedia && (source == SoundSource::scripts ? pMedia->apiMuted() : pMedia->gameMuted());
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
