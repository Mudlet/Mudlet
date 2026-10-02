#ifndef MUDLET_MUDLETREPLAY_H
#define MUDLET_MUDLETREPLAY_H

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
#include <QPointer>
#include <QTime>

class Host;

// The one replay the application plays at a time, whichever profile plays it:
// who holds it, how fast it runs and how much recorded time it has covered.
// cTelnet reads the file and feeds it through; this is the state around that
// which the main window shows on its replay toolbar. A member of the
// application object like HostManager, so a profile reaches it through self()
// rather than through the main window.
class MudletReplay : public QObject
{
    Q_OBJECT

public:
    Q_DISABLE_COPY_MOVE(MudletReplay)
    MudletReplay();
    ~MudletReplay() override;

    static MudletReplay* self() { return smpSelf; }

    // A relative file name is looked for in the profile's log directory. A
    // non-null pErrMsg marks a Lua caller: problems are described there rather
    // than on the profile's console.
    bool load(Host*, const QString& fileName, QString* pErrMsg = nullptr);

    // Claims the replay for pHost, at normal speed and from the start; false
    // while another replay holds it
    bool start(Host*);
    // Releases the replay, however it ended
    void over();
    bool running() const { return mRunning; }
    // The profile playing the replay, which need not be the one in front
    Host* host() const { return mpHost; }

    int speed() const { return mSpeed; }
    // Doubles or halves the speed, between 1 and 1024 times; nothing while no
    // replay runs
    void speedUp();
    void speedDown();

    // How much of the recording has been played, by its own timestamps
    const QTime& elapsed() const { return mElapsed; }
    // Called per chunk with the gap the recording has before it: counts the
    // gap as played and returns how long to wait at the current speed
    int advance(int gapMsec)
    {
        mElapsed = mElapsed.addMSecs(gapMsec);
        return gapMsec / mSpeed;
    }

signals:
    void signal_replayStarted();
    void signal_replayOver();
    void signal_replaySpeedChanged(int speed);

private:
    inline static MudletReplay* smpSelf = nullptr;

    QPointer<Host> mpHost;
    QTime mElapsed;
    int mSpeed = 1;
    bool mRunning = false;
};

#endif // MUDLET_MUDLETREPLAY_H
