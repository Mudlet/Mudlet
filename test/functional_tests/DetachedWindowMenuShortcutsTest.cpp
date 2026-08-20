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
 * A detached profile window's menu shortcuts have to reach their actions. The
 * hazard is the window's own menubar: QMenuBar grabs a window shortcut for each
 * menu title's mnemonic, so a title written "&Window" claims Alt+W alongside the
 * Close profile action that already has it. QShortcutMap reports both as one
 * ambiguous match and Qt then alternates between them - one press only warns
 * ("QAction::event: Ambiguous shortcut overload") while the next opens the menu -
 * so the shortcut looks dead while the same menu entry still works when clicked.
 * Only detached windows have such mnemonics to collide with.
 *
 * Run with: ctest -R DetachedWindowMenuShortcutsTest -V
 */

#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QPointer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "ShortcutsManager.h"
#include "TDetachedWindow.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

// Set while a test is watching for Qt's ambiguity warning. Which of the two
// symptoms a given press produces depends on the order the grabs entered
// QShortcutMap, so this is corroboration - the assertion that cannot be dodged is
// that the profile actually closed
static bool sAmbiguityWarningSeen = false;
static QtMessageHandler sPreviousMessageHandler = nullptr;

static void ambiguityWatchingMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    if (message.contains(qsl("Ambiguous shortcut overload"))) {
        sAmbiguityWarningSeen = true;
    }
    if (sPreviousMessageHandler) {
        sPreviousMessageHandler(type, context, message);
    }
}

class DetachedWindowMenuShortcutsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QPointer<TDetachedWindow> mpDetachedWindow;
    QString mPort;
    const QString mLocalhost = qsl("localhost");
    const QString mFirstHostname = qsl("DetachedWindowMenuShortcuts-First");
    const QString mSecondHostname = qsl("DetachedWindowMenuShortcuts-Second");

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

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. The opt-in
        // that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

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

    // A window per test rather than one for the class, so a failure cannot hand
    // the next test a half-torn-down one. cleanup()'s reattach and
    // detachSecondProfile()'s leftover check are what hold that up
    void init() { mpDetachedWindow = detachSecondProfile(); }

    void cleanup()
    {
        if (mudlet::self()->getDetachedWindows().contains(mSecondHostname)) {
            mudlet::self()->slot_tabReattachRequested(mSecondHostname);
        }
    }

    // No mnemonic at all rather than no mnemonic that collides today: the
    // shortcuts are user-assignable, so a title whose letter is free in the
    // default set is still a shortcut the next rebind can land on
    void test_noMenuTitleCarriesAMnemonic()
    {
        QVERIFY(mpDetachedWindow);
        QMenuBar* pMenuBar = mpDetachedWindow->menuBar();
        QVERIFY(pMenuBar);

        // A menubar mnemonic is a shortcut of the whole window, so it stands on an
        // action of any menu rather than only the one it titles
        const auto menuBarActions = pMenuBar->actions();
        QList<const QAction*> allActions;
        for (const QAction* pMenuAction : menuBarActions) {
            allActions << shortcutBearingActions(pMenuAction->menu());
        }
        QVERIFY2(!allActions.isEmpty(), "no menu action carries a shortcut, so nothing here could tell a live collision from a latent one");

        QStringList offenders;
        for (const QAction* pMenuAction : menuBarActions) {
            if (!carriesMnemonic(pMenuAction->text())) {
                continue;
            }

            // What QMenuBarPrivate::updateGeometries() would grab for this title.
            // It enriches the message rather than deciding it, because macOS
            // compiles mnemonics out (qt_sequence_no_mnemonics) and returns an
            // empty sequence for every title - which would leave this whole test
            // asserting nothing on two of the three CI platforms
            const QKeySequence mnemonic = QKeySequence::mnemonic(pMenuAction->text());
            if (mnemonic.isEmpty()) {
                offenders << qsl("\"%1\"").arg(pMenuAction->text());
                continue;
            }

            QStringList standingOn;
            for (const QAction* pAction : allActions) {
                if (pAction->shortcut() == mnemonic) {
                    standingOn << qsl("\"%1\"").arg(pAction->text());
                }
            }
            offenders << (standingOn.isEmpty() ? qsl("\"%1\" grabs %2").arg(pMenuAction->text(), mnemonic.toString())
                                               : qsl("\"%1\" grabs %2, which %3 uses").arg(pMenuAction->text(), mnemonic.toString(), standingOn.join(qsl(" and "))));
        }

        QVERIFY2(offenders.isEmpty(),
                 qPrintable(qsl("a detached window's menu titles carry mnemonics, which QMenuBar turns into window shortcuts that Qt then "
                                "ignores presses of once an action shares one: %1")
                                    .arg(offenders.join(qsl("; ")))));
    }

    // The reported symptom, driven the way the user hit it. Last, since it leaves
    // the profile closed
    void test_theCloseProfileShortcutClosesTheProfile()
    {
        QVERIFY(mpDetachedWindow);

        QKeySequence* pSequence = mudlet::self()->shortcutsManager()->getSequence(qsl("Close profile"));
        QVERIFY2(pSequence && !pSequence->isEmpty(), "no Close profile shortcut is registered, so this test would prove nothing");
        const QKeyCombination combination = (*pSequence)[0];

        // detachTab() leaves Host::setFocusOnHostActiveCommandLine()'s zero-timer
        // queued behind it and that activates the main window, so keep asking
        // rather than claiming activation once and losing it to a late arrival.
        // The claim has to be read off the widget layer: the detached window is
        // parented to the main window, and QWindow::isActive() counts a window as
        // active whenever its parent is, so qWaitForWindowActive() says yes on
        // every platform while QShortcutMap is still resolving against the main
        // window
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             mpDetachedWindow->activateWindow();
                             return QApplication::activeWindow() == mpDetachedWindow.data();
                         },
                         2000ms),
                 "the detached window never became the active window, so its window shortcuts could not be reached");

        sAmbiguityWarningSeen = false;
        sPreviousMessageHandler = qInstallMessageHandler(ambiguityWatchingMessageHandler);
        QTest::keyClick(mpDetachedWindow.data(), combination.key(), combination.keyboardModifiers());
        const bool ambiguous = sAmbiguityWarningSeen;
        qInstallMessageHandler(sPreviousMessageHandler);
        sPreviousMessageHandler = nullptr;

        QVERIFY2(!ambiguous, qPrintable(qsl("Qt called %1 ambiguous, so the press went to the warning rather than to the profile").arg(pSequence->toString())));
        QVERIFY2(QTest::qWaitFor(
                         [this]() {
                             return !mudlet::self()->getDetachedWindows().contains(mSecondHostname);
                         },
                         2000ms),
                 qPrintable(qsl("%1 left the detached profile open").arg(pSequence->toString())));
    }

private:
    // Reads shortcut() rather than shortcuts(), so an action given alternates
    // would only be checked on its primary one
    QList<const QAction*> shortcutBearingActions(const QMenu* pMenu) const
    {
        QList<const QAction*> found;
        if (!pMenu) {
            return found;
        }
        const auto actions = pMenu->actions();
        for (const QAction* pAction : actions) {
            if (!pAction->shortcut().isEmpty()) {
                found << pAction;
            }
            if (pAction->menu()) {
                found << shortcutBearingActions(pAction->menu());
            }
        }
        return found;
    }

    // QKeySequence::mnemonic() cannot answer this: macOS compiles mnemonics out,
    // so it returns an empty sequence for a title that carries one. "&&" is an
    // escaped ampersand rather than a mnemonic
    static bool carriesMnemonic(const QString& title)
    {
        for (int i = 0; i < title.size() - 1; ++i) {
            if (title.at(i) != QLatin1Char('&')) {
                continue;
            }
            if (title.at(i + 1) == QLatin1Char('&')) {
                ++i;
                continue;
            }
            return true;
        }
        return false;
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
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

#include "DetachedWindowMenuShortcutsTest.moc"
MUDLET_GROUPED_TEST_MAIN(DetachedWindowMenuShortcutsTest)
