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

/*
 * The game's Discord button in a detached window is a QToolButton holding a
 * QAction, and everything the user sees of it - the icon, the game's name, and
 * the click doing anything - comes from that action being the button's *default*
 * action rather than merely one of its actions. Without that the button falls
 * back to its own bare "Discord" text with no icon and swallows clicks.
 *
 * The last two cases cover the other half: each detached window owns its own copy
 * of the Discord actions, so an update that only refreshes the main window's pair
 * leaves the detached button showing the name the game had at detach time.
 *
 * Run with: ctest -R DetachedWindowDiscordButtonTest -V
 */

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QDesktopServices>
#include <QPointer>
#include <QToolButton>
#include <QtTest/QtTest>

#include "MudletPaths.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TDetachedWindow.h"
#include "TLuaInterpreter.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

// Stands in for the web browser: openWebPage() ends in QDesktopServices::openUrl(),
// which would otherwise launch one on the test machine
class UrlCatcher : public QObject
{
    Q_OBJECT

public:
    QUrl mLastUrl;

public slots:
    void slot_openUrl(const QUrl& url) { mLastUrl = url; }
};

class DetachedWindowDiscordButtonTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    UrlCatcher mUrlCatcher;
    QPointer<TDetachedWindow> mpDetachedWindow;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowDiscord-First");
    const QString mSecondHostname = qsl("DetachedWindowDiscord-Second");
    // updateDiscordNamedIcon() elides the game name at 90px, so every name used
    // here has to render narrower than that or the equality asserts below fail
    const QString mGameName = qsl("Achaea");
    const QString mInviteUrl = qsl("https://discord.gg/mudlet-discord-button-test");

    // setupConfig() consults portable.txt before the XDG logic
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

        // A config root of this process's own, so that a second copy of this test
        // running at the same time does not share a profile list with it
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletPaths::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDesktopServices::setUrlHandler(qsl("https"), &mUrlCatcher, "slot_openUrl");

        deleteProfileDirectory(mFirstHostname);
        deleteProfileDirectory(mSecondHostname);

        // Two of them, because slot_tabDetachRequested() refuses index 0
        startProfile(mFirstHostname);
        if (QTest::currentTestFailed()) {
            return;
        }
        startProfile(mSecondHostname);
    }

    void cleanupTestCase()
    {
        QDesktopServices::unsetUrlHandler(qsl("https"));
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            deleteProfileDirectory(mFirstHostname);
            deleteProfileDirectory(mSecondHostname);
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // The game's Discord details are what the button reads, so they have to be in
    // place before the detach builds that window's toolbar
    void init()
    {
        Host* pHost = mudlet::self()->getHostManager().getHost(mSecondHostname);
        QVERIFY(pHost);
        pHost->setDiscordGameName(mGameName);
        pHost->setDiscordInviteURL(mInviteUrl);
        mUrlCatcher.mLastUrl.clear();

        mpDetachedWindow = detachSecondProfile();
    }

    void cleanup()
    {
        if (mudlet::self()->getDetachedWindows().contains(mSecondHostname)) {
            mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        }
    }

    void test_theDiscordButtonShowsTheGamesNameAndIcon()
    {
        QToolButton* pButton = discordButton();
        QVERIFY(pButton);

        QVERIFY2(pButton->defaultAction(), "the Discord button has no default action, so it shows neither the game's name nor an icon and ignores clicks");
        QCOMPARE(pButton->defaultAction()->objectName(), qsl("openDiscord"));
        QCOMPARE(pButton->text(), mGameName);
        QVERIFY2(!pButton->icon().isNull(), "the Discord button has no icon");

        QAction* pMudletChat = mudletChatAction();
        QVERIFY(pMudletChat);
        QVERIFY2(pMudletChat->isVisible(), "the game has its own invite, so the separate Mudlet chat button should be showing");
    }

    // The common case: most profiles have no invite of their own, and this branch
    // could never run before the button started accepting clicks at all
    void test_withNoInviteTheButtonOpensMudletsOwnDiscord()
    {
        QToolButton* pButton = discordButton();
        QVERIFY(pButton);

        Host* pHost = mudlet::self()->getHostManager().getHost(mSecondHostname);
        QVERIFY(pHost);
        pHost->setDiscordInviteURL(QString());
        mudlet::self()->updateDiscordNamedIcon();

        QAction* pMudletChat = mudletChatAction();
        QVERIFY(pMudletChat);
        QVERIFY2(!pMudletChat->isVisible(), "with no invite of its own there is nothing for the separate Mudlet chat button to add");

        pButton->click();

        // mudlet::mMudletDiscordInvite, which is private
        QCOMPARE(mUrlCatcher.mLastUrl, QUrl(qsl("https://www.mudlet.org/chat")));
    }

    void test_clickingTheDiscordButtonOpensTheGamesInvite()
    {
        QToolButton* pButton = discordButton();
        QVERIFY(pButton);

        pButton->click();

        QCOMPARE(mUrlCatcher.mLastUrl, QUrl(mInviteUrl));
    }

    // Driven through the real GMCP entry point rather than a stand-in for it, so
    // that cutting the wiring anywhere along that path fails here
    void test_theButtonFollowsALaterGameNameChange()
    {
        QToolButton* pButton = discordButton();
        QVERIFY(pButton);

        Host* pHost = mudlet::self()->getHostManager().getHost(mSecondHostname);
        QVERIFY(pHost);
        pHost->processDiscordGMCP(qsl("External.Discord.Status"), qsl(R"({"game":"Avalon"})"));

        QCOMPARE(pHost->getDiscordGameName(), qsl("Avalon"));
        QCOMPARE(pButton->text(), qsl("Avalon"));
    }

    // The same refresh reached from Lua, which a detached profile's own scripts use
    void test_theButtonFollowsSetDiscordGameUrl()
    {
        QToolButton* pButton = discordButton();
        QVERIFY(pButton);

        Host* pHost = mudlet::self()->getHostManager().getHost(mSecondHostname);
        QVERIFY(pHost);

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setDiscordGameUrl('https://discord.gg/lusternia', 'Lusternia')")));

        QCOMPARE(pHost->getDiscordGameName(), qsl("Lusternia"));
        QCOMPARE(pButton->text(), qsl("Lusternia"));

        // Detached windows are parentless, so this can only reach the main one
        QToolButton* pMainButton = mudlet::self()->findChild<QToolButton*>(qsl("discord"));
        QVERIFY(pMainButton);
        QVERIFY2(pMainButton->text() != qsl("Lusternia"), "a detached profile's game name was painted onto the main window's button");

        QVERIFY(pHost->getLuaInterpreter()->compileAndExecuteScript(qsl("setDiscordGameUrl()")));

        QCOMPARE(pButton->text(), qsl("Discord"));
        QVERIFY(!mudletChatAction()->isVisible());
    }

private:
    // The action rather than the toolbar widget it produced: QToolBarLayout only
    // syncs that widget's visibility while laying out, which the offscreen
    // platform does not do, so the widget reads hidden even when the action is not
    QAction* mudletChatAction() const
    {
        if (!mpDetachedWindow) {
            QTest::qFail("no detached window to look for a Mudlet chat action in", __FILE__, __LINE__);
            return nullptr;
        }
        QAction* pAction = mpDetachedWindow->findChild<QAction*>(qsl("mudlet_discord"));
        if (!pAction) {
            QTest::qFail("the detached window's toolbar has no Mudlet chat action", __FILE__, __LINE__);
        }
        return pAction;
    }

    QToolButton* discordButton() const
    {
        if (!mpDetachedWindow) {
            QTest::qFail("no detached window to look for a Discord button in", __FILE__, __LINE__);
            return nullptr;
        }
        QToolButton* pButton = mpDetachedWindow->findChild<QToolButton*>(qsl("discord"));
        if (!pButton) {
            QTest::qFail("the detached window's toolbar has no Discord button", __FILE__, __LINE__);
        }
        return pButton;
    }

    TDetachedWindow* detachSecondProfile()
    {
        if (!mudlet::self()->getDetachedWindows().isEmpty()) {
            QTest::qFail("a detached window was left over from an earlier test", __FILE__, __LINE__);
            return nullptr;
        }

        mudlet::self()->slot_tabDetachRequested(1, QPoint(200, 200));

        TDetachedWindow* pDetachedWindow = mudlet::self()->getDetachedWindows().value(mSecondHostname);
        if (!pDetachedWindow) {
            // Whichever profile sits at tab 1 is the one that detaches, so say
            // which one was expected rather than only that nothing appeared
            QTest::qFail(qPrintable(qsl("detaching tab 1 produced no window for '%1' - the tab order is not what this test assumes").arg(mSecondHostname)), __FILE__, __LINE__);
        }
        return pDetachedWindow;
    }

    void startProfile(const QString& hostname)
    {
        auto host = TestProfile::create(hostname, mLocalhost, mPort);
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy connectionSpy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connectionSpy.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(MudletPaths::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowDiscordButtonTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowDiscordButtonTest)
