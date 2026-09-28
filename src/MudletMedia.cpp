/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

#include "MudletMedia.h"

#include "Host.h"
#include "HostManager.h"
#include "TMedia.h"

#include <QDebug>
#include <QMediaDevices>

MudletMedia::MudletMedia()
{
    if (smpSelf) {
        qWarning() << "MudletMedia::MudletMedia() WARNING - a MudletMedia already exists, so self() keeps pointing at that one.";
        return;
    }
    smpSelf = this;
}

MudletMedia::~MudletMedia()
{
    if (smpSelf == this) {
        smpSelf = nullptr;
    }
}

void MudletMedia::setApiMuted(const bool muted)
{
    setMuted(true, muted);
}

void MudletMedia::setGameMuted(const bool muted)
{
    setMuted(false, muted);
}

void MudletMedia::toggleAllMuted()
{
    if (allMuted()) {
        setApiMuted(false);
        setGameMuted(false);
        return;
    }

    if (!mApiMuted) {
        setApiMuted(true);
    }

    if (!mGameMuted) {
        setGameMuted(true);
    }
}

void MudletMedia::setMuted(const bool apiNotGame, const bool muted)
{
    bool& held = apiNotGame ? mApiMuted : mGameMuted;
    const bool changed = held != muted;

    if (auto* hostManager = HostManager::self()) {
        for (const auto& pHost : *hostManager) {
            if (muted) {
                if (apiNotGame) {
                    pHost->mpMedia->muteMedia(TMediaData::MediaProtocolAPI);
                } else {
                    pHost->mpMedia->muteMedia(TMediaData::MediaProtocolGMCP);
                    pHost->mpMedia->muteMedia(TMediaData::MediaProtocolMSP);
                }
            } else {
                if (apiNotGame) {
                    pHost->mpMedia->unmuteMedia(TMediaData::MediaProtocolAPI);
                } else {
                    pHost->mpMedia->unmuteMedia(TMediaData::MediaProtocolGMCP);
                    pHost->mpMedia->unmuteMedia(TMediaData::MediaProtocolMSP);
                }
            }
        }
    }

    held = muted;
    // Sent unchanged too, so a view can put a control the user toggled back
    // in step with what is held
    emit signal_muteSet(apiNotGame, muted);

    if (!changed || !HostManager::self()) {
        return;
    }

    // Muting is application-wide rather than per profile, so every open
    // profile is told about it. The handlers run Lua synchronously and may
    // open a profile, which inserts into the live host map, so this walks a
    // copy the way HostManager's own broadcasts do:
    const QString settingName = apiNotGame ? qsl("muteMediaAPI") : qsl("muteMediaGame");
    const QList<QSharedPointer<Host>> hosts = HostManager::self()->hostList();
    for (const auto& pHost : hosts) {
        if (held != muted) {
            // A handler in an earlier profile wrote the opposite value back;
            // that nested call already told every profile, so the rest of this
            // loop would report a value nothing holds any more
            break;
        }
        pHost->raiseSettingChangedEvent(settingName, muted);
    }
}

void MudletMedia::watchAudioOutputDevices()
{
    if (mpMediaDevices) {
        return;
    }
    mpMediaDevices = new QMediaDevices(this);
    connect(mpMediaDevices, &QMediaDevices::audioOutputsChanged, this, &MudletMedia::refreshAudioDevices);
}

void MudletMedia::refreshAudioDevices()
{
    auto* hostManager = HostManager::self();
    if (!hostManager) {
        return;
    }
    for (const auto& pHost : *hostManager) {
        if (pHost && pHost->mpMedia) {
            pHost->mpMedia->refreshAudioDevices();
        }
    }
}
