#ifndef MUDLET_MUDLETMEDIA_H
#define MUDLET_MUDLETMEDIA_H

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

#include <QObject>

class QMediaDevices;

// The application-wide side of media playback: the two mute switches every
// profile's TMedia obeys, and the watch on the system's audio outputs. Owned by
// HostManager, so a profile reaches it through self() rather than through the
// main window, and has it with no main window at all.
class MudletMedia : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(MudletMedia)
    MudletMedia();
    ~MudletMedia() override;

    static MudletMedia* self() { return smpSelf; }

    // Sounds played by scripts, and sounds the game sends over GMCP or MSP
    bool apiMuted() const { return mApiMuted; }
    bool gameMuted() const { return mGameMuted; }
    bool allMuted() const { return mApiMuted && mGameMuted; }
    bool noneMuted() const { return !mApiMuted && !mGameMuted; }

    // Reaches the players already running in every profile too. Every call is
    // announced through signal_muteSet, but only a change raises
    // sysSettingChanged in each profile.
    void setApiMuted(bool);
    void setGameMuted(bool);
    // Unmutes both when both are muted, otherwise mutes whichever is not
    void toggleAllMuted();

    // Deferred until the first player is made: constructing a QMediaDevices
    // loads the multimedia backend, which can stall start-up for hundreds of
    // milliseconds probing hardware decoders.
    void watchAudioOutputDevices();

signals:
    void signal_muteSet(bool apiNotGame, bool muted);

private:
    void setMuted(bool apiNotGame, bool muted);
    void refreshAudioDevices();

    inline static MudletMedia* smpSelf = nullptr;

    QMediaDevices* mpMediaDevices = nullptr;
    bool mApiMuted = false;
    bool mGameMuted = false;
};

#endif // MUDLET_MUDLETMEDIA_H
