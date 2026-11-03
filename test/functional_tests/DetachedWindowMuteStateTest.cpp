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

#include <QAction>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "MudletMedia.h"
#include "ProfileTestHelper.h"
#include "TDetachedWindow.h"
#include "TTabBar.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Muting is application-wide, so a detached window's mute items have to show
// what is held, whichever window or script changed it, or clicking one does
// the opposite of what its checkmark says.
class DetachedWindowMuteStateTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowMuteState-First");
    const QString mSecondHostname = qsl("DetachedWindowMuteState-Second");

    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        mudlet::self()->show();

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);
        // Two of them: slot_tabDetachRequested() refuses index 0
        startProfile(mFirstHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondHostname);
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(MudletMedia::self());
        MudletMedia::self()->setApiMuted(false);
        MudletMedia::self()->setGameMuted(false);
        QVERIFY2(mudlet::self()->getDetachedWindows().isEmpty(), "a detached window was left over from an earlier case");
    }

    void cleanup()
    {
        const QStringList detachedProfiles = mudlet::self()->getDetachedWindows().keys();
        for (const QString& profileName : detachedProfiles) {
            mudlet::self()->slot_tabReattachRequested(profileName);
        }
    }

    void test_aWindowDetachedWhileMutedShowsItMuted()
    {
        MudletMedia::self()->setApiMuted(true);

        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);

        QVERIFY(muteAction(pWindow, qsl("menuMuteAPI"))->isChecked());
        QVERIFY(muteAction(pWindow, qsl("muteAPI"))->isChecked());
        QVERIFY(!muteAction(pWindow, qsl("menuMuteGame"))->isChecked());
        QVERIFY(!muteAction(pWindow, qsl("menuMuteMedia"))->isChecked());
    }

    void test_mutingElsewhereReachesAnOpenDetachedWindow()
    {
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        const QStringList names{qsl("menuMuteMedia"), qsl("menuMuteAPI"), qsl("menuMuteGame"), qsl("muteMedia"), qsl("muteAPI"), qsl("muteGame")};
        for (const QString& name : names) {
            QVERIFY2(muteAction(pWindow, name) && !muteAction(pWindow, name)->isChecked(), qPrintable(qsl("%1 started out checked or missing").arg(name)));
        }

        MudletMedia::self()->toggleAllMuted();

        for (const QString& name : names) {
            QVERIFY2(muteAction(pWindow, name)->isChecked(), qPrintable(qsl("%1 is unchecked while everything is muted").arg(name)));
        }

        MudletMedia::self()->setGameMuted(false);

        QVERIFY(muteAction(pWindow, qsl("menuMuteAPI"))->isChecked());
        QVERIFY(!muteAction(pWindow, qsl("menuMuteGame"))->isChecked());
        QVERIFY(!muteAction(pWindow, qsl("muteGame"))->isChecked());
        QVERIFY2(!muteAction(pWindow, qsl("menuMuteMedia"))->isChecked(), "Mute all media is checked while the game is not muted");
        QVERIFY(!muteAction(pWindow, qsl("muteMedia"))->isChecked());
    }

    void test_clickingAMuteItemDoesWhatItsCheckmarkSays()
    {
        TDetachedWindow* pWindow = detachProfile(mSecondHostname);
        QVERIFY(pWindow);
        MudletMedia::self()->setApiMuted(true);
        QAction* pMuteAPI = muteAction(pWindow, qsl("menuMuteAPI"));
        QVERIFY(pMuteAPI);

        pMuteAPI->trigger();

        QVERIFY2(!MudletMedia::self()->apiMuted(), "clicking the checked item did not unmute");
        QVERIFY(!pMuteAPI->isChecked());
        QVERIFY(!muteAction(pWindow, qsl("muteAPI"))->isChecked());
    }

private:
    QAction* muteAction(TDetachedWindow* pWindow, const QString& name) const { return pWindow ? pWindow->findChild<QAction*>(name) : nullptr; }

    TDetachedWindow* detachProfile(const QString& profileName)
    {
        const int tabIndex = mudlet::self()->mpTabBar->tabIndex(profileName);
        if (tabIndex < 1) {
            QTest::qFail(qPrintable(qsl("'%1' is at tab %2, which slot_tabDetachRequested() will not detach").arg(profileName).arg(tabIndex)), __FILE__, __LINE__);
            return nullptr;
        }
        mudlet::self()->slot_tabDetachRequested(tabIndex, QPoint(200, 200));
        TDetachedWindow* pWindow = mudlet::self()->getDetachedWindows().value(profileName);
        if (!pWindow) {
            QTest::qFail(qPrintable(qsl("detaching tab %1 produced no window for '%2'").arg(tabIndex).arg(profileName)), __FILE__, __LINE__);
        }
        return pWindow;
    }

    void startProfile(const QString& hostname)
    {
        auto host = TestProfile::create(hostname, mLocalhost, mPort);
        if (!host) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletApp::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowMuteStateTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowMuteStateTest)
