#ifndef MUDLET_PROFILESESSIONTESTHELPER_H
#define MUDLET_PROFILESESSIONTESTHELPER_H

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

#include <chrono>
#include <memory>

#include <QAbstractSocket>
#include <QByteArray>
#include <QDir>
#include <QSignalSpy>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"
#include "utils.h"

namespace TestProfile {

// No step uses QVERIFY or QFAIL: both expand to a bare return, which is
// ill-formed in a function returning bool and leaves only the helper from a void
// one. A step returns false with why() set, and the caller QVERIFY2s that.
//
// stop() deletes Mudlet before it removes the profile directories, because
// Windows will not delete the files a Host holds open - see
// removeProfileDirectory(). It puts the XDG variables back so nothing later in
// the process resolves a path into a temporary directory that is gone.
class Session
{
public:
    Session() = default;
    ~Session() { stop(); }
    Q_DISABLE_COPY_MOVE(Session)

    bool start(const QString& profileName) { return startMudlet() && openProfile(profileName); }

    // Folders named here exist before init() reads the profiles directory, which is
    // what decides whether this is a first launch
    bool startMudlet(const QStringList& profilesPresentBeforeInit = {})
    {
        if (!mConfigDir.isValid() || !mCacheDir.isValid()) {
            return fail(qsl("no temporary directory for the config and cache roots: %1 %2").arg(mConfigDir.errorString(), mCacheDir.errorString()));
        }
        // setupConfig() adopts $XDG_CONFIG_HOME/mudlet only once it holds a profiles directory (#9712)
        const QString profilesDir = qsl("%1/mudlet/profiles").arg(mConfigDir.path());
        if (!QDir().mkpath(profilesDir)) {
            return fail(qsl("could not create %1").arg(profilesDir));
        }
        for (const QString& name : profilesPresentBeforeInit) {
            if (!QDir().mkpath(qsl("%1/%2").arg(profilesDir, name))) {
                return fail(qsl("could not create %1/%2").arg(profilesDir, name));
            }
        }
        if (!mEnvironmentRedirected) {
            mSavedXdgConfig = qgetenv("XDG_CONFIG_HOME");
            mSavedXdgCache = qgetenv("XDG_CACHE_HOME");
            mEnvironmentRedirected = true;
        }
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        // The glyphs stylesheets point at are written into the cache, and a run writes nowhere the machine keeps
        qputenv("XDG_CACHE_HOME", mCacheDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(address(), 0);
        if (!mpServer->isListening()) {
            return fail(qsl("TelnetServerStub failed to start: %1").arg(mpServer->errorString()));
        }
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        const QString configRoot = MudletApp::getMudletPath(enums::mainPath);
        if (configRoot != qsl("%1/mudlet").arg(mConfigDir.path())) {
            return fail(qsl("setupConfig() settled on %1 rather than %2/mudlet").arg(configRoot, mConfigDir.path()));
        }
        mConfigRootVerified = true;
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        return true;
    }

    bool openProfile(const QString& profileName)
    {
        // Nothing is created or removed under a config root that might be the user's own
        if (!mConfigRootVerified) {
            return fail(qsl("openProfile(%1) without a startMudlet() that succeeded").arg(profileName));
        }
        if (!removeProfileDirectory(profileName)) {
            return fail(qsl("could not remove what an earlier run left of the profile at %1").arg(MudletApp::getMudletPath(enums::profileHomePath, profileName)));
        }
        // Before create(), since one that times out part way can still leave a directory behind
        mOpenedProfiles << profileName;
        mProfileName = profileName;
        mpHost = create(profileName, address(), mPort);
        if (!mpHost) {
            return fail(qsl("TestProfile::create() did not open %1 - its warning names the step that timed out").arg(profileName));
        }
        // QSignalSpy::wait() only returns for a signal sent while it waits, and the
        // connection can already be up by the time create() returns
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState && !connected.wait(scmConnectTimeout)) {
            return fail(qsl("%1 never connected to the stub server on %2:%3").arg(profileName, address(), mPort));
        }
        return true;
    }

    void stop()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
        for (const QString& profileName : std::as_const(mOpenedProfiles)) {
            removeProfileDirectory(profileName);
        }
        mOpenedProfiles.clear();
        mConfigRootVerified = false;
        if (mEnvironmentRedirected) {
            mSavedXdgConfig.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdgConfig);
            mSavedXdgCache.isNull() ? qunsetenv("XDG_CACHE_HOME") : qputenv("XDG_CACHE_HOME", mSavedXdgCache);
            mEnvironmentRedirected = false;
        }
    }

    static bool configCannotBeRedirected() { return portableMarkerPresent(); }

    static const char* configCannotBeRedirectedReason() { return "portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected"; }

    Host* host() const { return mpHost; }
    TelnetServerStub* server() const { return mpServer; }
    QString port() const { return mPort; }
    QString address() const { return qsl("localhost"); }
    const QString& profileName() const { return mProfileName; }
    const QString& why() const { return mWhy; }

private:
    bool fail(const QString& why)
    {
        mWhy = why;
        return false;
    }

    static constexpr std::chrono::seconds scmConnectTimeout{8};

    QTemporaryDir mConfigDir;
    QTemporaryDir mCacheDir;
    QByteArray mSavedXdgConfig;
    QByteArray mSavedXdgCache;
    bool mEnvironmentRedirected = false;
    bool mConfigRootVerified = false;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    Host* mpHost = nullptr;
    QString mProfileName;
    QStringList mOpenedProfiles;
    QString mWhy;
};

} // namespace TestProfile

#endif // MUDLET_PROFILESESSIONTESTHELPER_H
