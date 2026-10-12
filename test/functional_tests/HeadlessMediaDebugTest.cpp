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

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

#include "Host.h"
#include "HostManager.h"
#include "MediaBackendTestHelper.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "TAppFrontend.h"
#include "TConsoleModel.h"
#include "TDebug.h"
#include "TLuaInterpreter.h"

#include "GroupedTest.h"

// A profile made without ever starting the main window, so it only has the null console view.
// Debug mode reports a media error in the main console, which such a profile has as well. In
// the media group for its QT_MEDIA_BACKEND pin, so this runs on the backend Mudlet ships with.
class HeadlessMediaDebugTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    // The main window owns the profile pool in the app; here nothing else would.
    std::unique_ptr<HostManager> mpHostManager;
    QString mUndecodableFile;
    // Set when the backend does not report an undecodable file as an error.
    QString mNoLoadErrorReason;

    static QString luaGlobalString(Host* host, const char* name)
    {
        lua_State* L = host->getLuaInterpreter()->getLuaGlobalState();
        lua_getglobal(L, name);
        const QString value = lua_isstring(L, -1) ? QString::fromUtf8(lua_tostring(L, -1)) : QString();
        lua_pop(L, 1);
        return value;
    }

    static bool mainBufferHolds(Host* host, const QString& text) { return host->mainConsoleModel().buffer.lineBuffer.join(QChar::LineFeed).contains(text); }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));

        mUndecodableFile = qsl("%1/headless-not-a-sound.wav").arg(mConfigDir.path());
        QFile file(mUndecodableFile);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArrayLiteral("not a sound at all, whatever its name says"));
        file.close();
        mNoLoadErrorReason = undecodableFileUnreportedReason(mUndecodableFile);

        QVERIFY2(!HostManager::self(), "A profile pool already exists, so this run is not headless.");
        mpHostManager = std::make_unique<HostManager>();
    }

    void cleanupTestCase()
    {
        mpHostManager.reset();
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void test_mediaErrorIsReportedInDebugModeWithNoMainWindow()
    {
        SKIP_OR_FAIL_WITHOUT(mNoLoadErrorReason);
        QVERIFY2(!TAppFrontend::hasView(), "A main window exists, so this run is not headless.");

        const QString hostname = qsl("Test-Headless-Media-Debug");
        QVERIFY2(HostManager::self()->addHost(hostname, QString(), QString(), QString()), "Could not create a profile with no main window.");
        Host* host = HostManager::self()->getHost(hostname);
        QVERIFY2(host, "The profile is not in the pool.");

        const bool debugMode = TDebug::smDebugMode;
        const auto restoreDebugMode = qScopeGuard([debugMode] {
            TDebug::smDebugMode = debugMode;
        });
        TDebug::smDebugMode = true;
        const QString script = qsl(R"lua(
local ok, err = pcall(function()
  assert(playSoundFile({name = [[%1]], key = "headlessNotASound"}), "playSoundFile refused")
end)
headlessMediaError = ok and 'ok' or tostring(err)
)lua")
                                       .arg(mUndecodableFile);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "The Lua chunk did not run.");
        QCOMPARE(luaGlobalString(host, "headlessMediaError"), qsl("ok"));
        QTRY_VERIFY2_WITH_TIMEOUT(mainBufferHolds(host, qsl("Media error: ")), "The media error was not reported in the main console.", 10000);
    }
};

#include "HeadlessMediaDebugTest.moc"
MUDLET_GROUPED_TEST_MAIN(HeadlessMediaDebugTest)
