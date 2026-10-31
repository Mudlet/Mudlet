#ifndef MUDLET_HOSTMANAGER_H
#define MUDLET_HOSTMANAGER_H

/***************************************************************************
 *   Copyright (C) 2008-2011 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2016, 2018, 2025 by Stephen Lyons                       *
 *                                               - slysven@virginmedia.com *
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


#include <QMap>
#include <QString>
#include <QStringList>
#include <QSharedPointer>

#include <chrono>
#include <functional>
#include <memory>
#include <utility>

class Discord;
class FontManager;
class Host;
class MudletMedia;
class QSettings;
class TEvent;
class TSpeechBridge;
typedef QMap<QString, QSharedPointer<Host>> HostMap;

class HostManager
{
    class Iter
    {
    public:
        Iter(HostManager* mgr, bool top);
        bool operator!=(const Iter& other) const;
        bool operator==(const Iter& other) const;
        Iter& operator++();
        QSharedPointer<Host> operator*();

    private:
        HostMap::iterator it;
    };


public:
    // Every profile reads Mudlet.ini as it is made, so the config root has to be settled first. The main
    // window settles it itself, after it is built, and a pool made with no main window as it is made.
    enum class ConfigRoot { settledHere, leftToMainWindow };

    Q_DISABLE_COPY(HostManager)
    explicit HostManager(ConfigRoot = ConfigRoot::settledHere);
    ~HostManager();

    // A value member of the application object, so valid from that object's constructor body until its
    // members are torn down; null outside that.
    static HostManager* self() { return smpSelf; }

    MudletMedia& media() { return *mpMedia; }
    FontManager& fonts() { return *mpFonts; }

    Host* getHost(const QString& hostname);
    // False for an empty name, one already open, or while the pool is being destroyed
    bool addHost(const QString& name, const QString& port, const QString& login, const QString& pass);
    // Loads and starts a profile with no window, as the GUI's load does, a new profile getting its game's
    // default packages. Null when that profile is already open, or the pool is going.
    // beforeLoad is handed the profile before its save loads, so a frontend can watch it from the first line;
    // afterLoad once its save has loaded and before its scripts start, where the connection dialog puts what
    // is typed into it over what the load set.
    Host* openProfile(const QString& name, bool connect, const std::function<void(Host*)>& beforeLoad = nullptr, const std::function<void(Host*)>& afterLoad = nullptr);
    // Handed every profile openProfile() opens, a script's loadProfile() ones too, before beforeLoad. A call
    // replaces the hook set before. The pool may outlive whoever set it, so the hook must capture no raw
    // pointer to its setter.
    void setProfileOpenedHook(std::function<void(Host*)> hook) { mProfileOpenedHook = std::move(hook); }
    static QStringList defaultPackages(const QString& gameUrl, const QString& profileName, bool serverGuiAccepted);
    // A profile tab's close button, with no window: sysExitEvent and the save now, out of the pool once
    // no script of its own is running. False if no such profile is open.
    bool requestProfileClose(const QString& name);
    // closeMudlet() with no main window, less the quit: closes and saves every profile, then empties the
    // pool, leaving one still running a script or a map operation until that ends. Not during a load.
    void closeAllProfiles();
    int getHostCount();
    void deleteHost(const QString&);
    // For a frontend closing every profile as it goes, ahead of the pool's own destruction
    void refuseNewProfiles() { mGoingDown = true; }
    void postIrcMessage(const QString&, const QString&, const QString&);
    void postInterHostEvent(const Host*, const TEvent&, const bool = false);
    void changeAllHostColour(const Host*);
    Iter begin() { return Iter(this, true); }
    Iter end() { return Iter(this, false); }
    bool hostLoaded(const QString& hostname) const;
    // A copy to walk while doing anything that may open or close a profile
    QList<QSharedPointer<Host>> hostList() const { return mHostPool.values(); }
    // A load runs the event loop part-way through, so closeMudlet() waits for it, as the GUI's does
    bool loadingProfile() const { return mProfileLoadsInProgress > 0; }
    // Whether a profile taken out of its pool is still held until no script of its own runs, which
    // closeMudlet() waits for
    static bool releasingProfile();

private:
    // The GUI's call, from mudlet::readLateSettings()
    friend class mudlet;

    // The Mudlet.ini values that profiles act on but that no profile reads for itself. Once per
    // process, before any profile, as reading them again would undo what has changed since.
    void readSettings(const QSettings&);
    void deleteHostOnceUnwound(Host*, std::chrono::milliseconds delay);

    inline static HostManager* smpSelf = nullptr;

    // Every profile obeys these switches, so whatever owns the profiles owns them too, and with no main
    // window a profile still has real ones. Before mHostPool, so they outlive the profiles that reach them.
    std::unique_ptr<MudletMedia> mpMedia;
    // Before mHostPool too, as the profiles reach it through FontManager::self()
    std::unique_ptr<FontManager> mpFonts;
    // Before mHostPool too, as the profiles reach it through Discord::self(); a second pool's goes unused
    std::unique_ptr<Discord> mpDiscord;
    // Before mHostPool too, as the profiles reach it through TSpeechBridge::instance(); a second pool's goes unused
    std::unique_ptr<TSpeechBridge> mpSpeechBridge;
    HostMap mHostPool;
    // From refuseNewProfiles() or the destructor on: a profile opened then would never get its sysExitEvent,
    // nor have its windows closed before it
    bool mGoingDown = false;
    int mProfileLoadsInProgress = 0;
    std::function<void(Host*)> mProfileOpenedHook;
    // Expires with the pool, for timers tied to a profile that may outlive it
    const std::shared_ptr<bool> mAlive = std::make_shared<bool>(true);
};

#endif // MUDLET_HOSTMANAGER_H
