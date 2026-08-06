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
 * The windows a Host hands out - the notepad, the IRC client and the toolbars
 * its actions put on the main window - are all created without a Host parent,
 * so the Qt parent-child system does not dispose of them along with the
 * profile. Only Host::closeChildren() did, and a Host that is destroyed
 * without it (its shared pointer being dropped from the host pool is all it
 * takes) orphaned the lot.
 *
 * Two more windows are in the same family and deliberately not covered here:
 * the trigger editor, which PR #9700 "fix: trigger editor and deleted item
 * subtrees leaking memory" is fixing in the same place, and the user windows
 * TMainConsole hands out.
 *
 * The tests below take the three orderings a Host goes away in - destroyed on
 * its own, destroyed after closeChildren(), and destroyed by the main window
 * going - and assert the same thing of each: after the Host is gone, so are its
 * windows. The QPointers are what makes that provable in any build, a leak
 * leaves them still set, while an AddressSanitizer build additionally turns the
 * double delete that a naive "delete it in both places" fix would introduce
 * into a failure.
 *
 * Note that the toolbars are parented to the main window, so the third ordering
 * reclaims them either way; there it is the notepad and the IRC client that are
 * orphaned.
 *
 * Run with: ctest -R HostChildTeardownTest -V
 */

#include <QtTest/QtTest>
#include <chrono>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>

#include "ActionUnit.h"
#include "Host.h"
#include "HostManager.h"
#include "MudletInstanceCoordinator.h"
#include "TAction.h"
#include "TMainConsole.h"
#include "TToolBar.h"
#include "TelnetServerStub.h"
#include "dlgConnectionProfiles.h"
#include "dlgIRC.h"
#include "dlgNotepad.h"
#include "mudlet.h"

using namespace std::chrono_literals;

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForHostChildTeardownTest();

class HostChildTeardownTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    QString mPort; // the stub's actual ephemeral port, so concurrent test runs cannot collide
    const QString mLocalhost = qsl("localhost");

    void deleteProfileDirectory(const QString& profileName)
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

    Host* startProfile(const QString& profileName)
    {
        deleteProfileDirectory(profileName);

        const QString address = mLocalhost;
        const QString port = mPort;
        QTimer::singleShot(0ms, qApp, [profileName, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), profileName);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5000)) {
            return nullptr;
        }
        return mudlet::self()->getActiveHost();
    }

    // A root action set to be a floating toolbar is what puts a TToolBar on the
    // main window; the toolbar is only built when the unit regenerates them.
    void createToolBarAction(Host* pHost, const QString& name)
    {
        auto* pAction = new TAction(name, pHost);
        pAction->setCommandButtonUp(QString());
        pAction->setCommandButtonDown(QString());
        pAction->setIsPushDownButton(false);
        pAction->setIsFolder(true);
        pAction->mLocation = 4; // floating/dockable toolbar
        pAction->mOrientation = 1;
        pAction->setScript(QString());
        pAction->setIsActive(true);
        pAction->registerAction();
    }

    // Every test here wants a profile with all of those windows open.
    struct OpenWindows
    {
        QPointer<dlgNotepad> notePad;
        QPointer<dlgIRC> dlgIrc;
        // two of them, so that the loop in ~Host() is made to iterate
        QList<QPointer<TToolBar>> toolBars;
    };

    OpenWindows openEveryChildWindow(Host* pHost)
    {
        OpenWindows windows;

        mudlet::self()->slot_notes();
        windows.notePad = pHost->mpNotePad;
        if (windows.notePad) {
            // gives the destructor's save() something to write, and this test
            // something to read back
            if (auto* note = qobject_cast<QPlainTextEdit*>(windows.notePad->tabWidget->widget(0))) {
                note->setPlainText(csmNoteText);
            }
        }

        // built directly rather than through the Lua openIrc(): that connects to
        // the network, which a test has no business doing
        pHost->mpDlgIRC = new dlgIRC(pHost);
        pHost->mpDlgIRC->show();
        windows.dlgIrc = pHost->mpDlgIRC;

        createToolBarAction(pHost, qsl("HostChildTeardown toolbar"));
        createToolBarAction(pHost, qsl("HostChildTeardown second toolbar"));
        pHost->getActionUnit()->updateAllToolbars();
        for (const auto& pToolBar : pHost->getActionUnit()->getToolBarList()) {
            windows.toolBars.append(pToolBar);
        }
        return windows;
    }

    // Reports every one of them rather than stopping at the first, so that a
    // failure says which of the windows were left behind.
    static QStringList windowsLeftBehind(const OpenWindows& windows)
    {
        QStringList leftBehind;
        if (windows.notePad) {
            leftBehind << qsl("the notepad");
        }
        if (windows.dlgIrc) {
            leftBehind << qsl("the IRC client");
        }
        for (const auto& pToolBar : windows.toolBars) {
            if (pToolBar) {
                leftBehind << qsl("a toolbar");
            }
        }
        return leftBehind;
    }

    static bool everyWindowWasOpened(const OpenWindows& windows) { return windows.notePad && windows.dlgIrc && windows.toolBars.size() == 2 && windows.toolBars.at(0) && windows.toolBars.at(1); }

    // What the notepad is made to hold, so that the notes the destructor writes
    // out can be told apart from an empty note
    static inline const QString csmNoteText = qsl("HostChildTeardown note text");

    QString noteContentOnDisk(const QString& profileName) const
    {
        QFile file(mudlet::getMudletPath(enums::profileDataItemPath, profileName, qsl("notes.json")));
        if (!file.open(QIODevice::ReadOnly)) {
            return QString();
        }
        const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
        const QJsonArray tabs = root.value(qsl("tabs")).toArray();
        if (tabs.isEmpty()) {
            return QString();
        }
        return tabs.at(0).toObject().value(qsl("content")).toString();
    }

    // The profile whose windows cleanupTestCase() checks on, left open on purpose
    OpenWindows mWindowsLeftOpenAtTheEnd;
    bool mLeftOpenProfileWasSetUp = false;
    const QString mProfileLeftOpenAtTheEnd = qsl("HostChildTeardown-LeftOpen");

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForHostChildTeardownTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        // a stub that did not bind only warns, and every test would then fail
        // five seconds later saying the profile was slow to load
        QVERIFY2(mpServer->serverPort() != 0, "The telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    // A test that stops at a failed assertion leaves its Host in the pool, and
    // the next one would then load its profile alongside it and could take the
    // wrong one as active - one failure reporting as three. The profile the last
    // test leaves open on purpose is not named here.
    void cleanup()
    {
        for (const QString& profileName : {qsl("HostChildTeardown-NoCloseChildren"), qsl("HostChildTeardown-CloseChildren")}) {
            if (mudlet::self()->getHostManager().getHost(profileName)) {
                mudlet::self()->getHostManager().deleteHost(profileName);
            }
            deleteProfileDirectory(profileName);
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;

        // getMudletPath() reads the main window, so the path has to be taken
        // while there still is one
        const QString leftOpenProfilePath = mudlet::getMudletPath(enums::profileHomePath, mProfileLeftOpenAtTheEnd);

        // The third case: a profile that is still loaded when the main window
        // goes. ~mudlet destroys the host pool as one of its members, so the
        // Host is destroyed with no close of any kind having been asked for -
        // and this is what most of the functional tests do to their profiles.
        QVERIFY2(mLeftOpenProfileWasSetUp, "The profile this checks on was never opened, so the check below would pass on three null pointers");
        delete mudlet::self();
        // Only the notepad and the IRC client carry weight here: the toolbars
        // are children of the main window, so ~QWidget frees them either way.
        const QStringList leftBehind = windowsLeftBehind(mWindowsLeftOpenAtTheEnd);
        QVERIFY2(leftBehind.isEmpty(), qPrintable(qsl("Destroying the main window left %1 of the profile that was still loaded behind").arg(leftBehind.join(qsl(" and ")))));

        QDir(leftOpenProfilePath).removeRecursively();
    }

    // The reported leak: nothing called closeChildren(), the host pool simply
    // let go of the Host. Mudlet itself gets here whenever the profile's main
    // console has already gone, because Host::requestClose() then answers "we
    // must already be dying" and returns before it reaches closeChildren().
    void test_destroyingTheHostTakesItsWindowsWithIt()
    {
        const QString profileName = qsl("HostChildTeardown-NoCloseChildren");
        Host* pHost = startProfile(profileName);
        QVERIFY2(pHost, "Profile took too long to load");

        const OpenWindows windows = openEveryChildWindow(pHost);
        QVERIFY2(everyWindowWasOpened(windows), "Not all of the profile's windows were opened");

        // forceClose() stops TMainConsole::closeEvent() asking whether to save,
        // which would block on a modal dialog here; the console carries
        // WA_DeleteOnClose, so closing it is what takes it away
        pHost->forceClose();
        pHost->mpConsole->close();
        QTRY_VERIFY2(pHost->mpConsole.isNull(), "The main console did not go away"); // Qt 6 disposes of a WA_DeleteOnClose widget by deleteLater()
        QVERIFY2(pHost->requestClose(), "Closing the profile was refused");
        QVERIFY2(pHost->mpNotePad, "requestClose() reached closeChildren() after all - this no longer tests a Host that skips it");

        const QPointer<Host> hostGuard(pHost);
        pHost = nullptr;
        // dropping the pool's shared pointer is all that mudlet::closeHost()
        // does to the Host itself, and it is where ~Host() runs
        mudlet::self()->getHostManager().deleteHost(profileName);
        QVERIFY2(hostGuard.isNull(), "The Host outlived deleteHost(), so ~Host() never ran");

        const QStringList leftBehind = windowsLeftBehind(windows);
        QVERIFY2(leftBehind.isEmpty(), qPrintable(qsl("Destroying the Host left %1 behind").arg(leftBehind.join(qsl(" and ")))));
        // ~Host() closes the notepad rather than only deleting it, which is what
        // gets the notes and the window state written out
        QCOMPARE(noteContentOnDisk(profileName), csmNoteText);

        deleteProfileDirectory(profileName);
    }

    // The path every ordinary profile close takes. closeChildren() hands the
    // toolbars and the IRC client to deleteLater() and closes the notepad, which
    // Qt 6 also defers - and mudlet::closeEvent() destroys the Host in the same
    // call stack, so ~Host() meets windows whose deferred delete has not run
    // yet. Deleting one of those a second time would end this run under
    // AddressSanitizer.
    void test_closeChildrenFollowedByDestructionIsSafe()
    {
        const QString profileName = qsl("HostChildTeardown-CloseChildren");
        Host* pHost = startProfile(profileName);
        QVERIFY2(pHost, "Profile took too long to load");

        const OpenWindows windows = openEveryChildWindow(pHost);
        QVERIFY2(everyWindowWasOpened(windows), "Not all of the profile's windows were opened");

        pHost->forceClose();
        QVERIFY2(pHost->requestClose(), "Closing the profile was refused");
        QVERIFY2(windows.notePad, "The notepad was disposed of before this could test what happens when it has not been");
        // deliberately no event loop turn before this: mudlet::closeEvent() does
        // not give one either, which is what leaves the deferred deletes pending
        const QPointer<Host> hostGuard(pHost);
        pHost = nullptr;
        mudlet::self()->getHostManager().deleteHost(profileName);
        QVERIFY2(hostGuard.isNull(), "The Host outlived deleteHost(), so ~Host() never ran");

        // Checked before the event loop gets a turn, so that the deferred
        // deletes closeChildren() queued cannot be what satisfies it: ~Host()
        // deletes the toolbars outright, and that is the half of this the rest
        // of the run would otherwise not notice going missing
        for (const auto& pToolBar : windows.toolBars) {
            QVERIFY2(pToolBar.isNull(), "Destroying the Host did not delete a toolbar closeChildren() had only queued");
        }

        // and now let what closeChildren() deferred run, which is where a second
        // delete of anything ~Host() already took would land
        QTRY_VERIFY2(windowsLeftBehind(windows).isEmpty(), "Closing and then destroying the Host left one of its windows behind");
        deleteProfileDirectory(profileName);
    }

    // Leaves the profile loaded for cleanupTestCase() to destroy the main window
    // on top of.
    void test_leaveAProfileOpenForTheMainWindowToTakeDown()
    {
        Host* pHost = startProfile(mProfileLeftOpenAtTheEnd);
        QVERIFY2(pHost, "Profile took too long to load");

        mWindowsLeftOpenAtTheEnd = openEveryChildWindow(pHost);
        QVERIFY2(everyWindowWasOpened(mWindowsLeftOpenAtTheEnd), "Not all of the profile's windows were opened");
        mLeftOpenProfileWasSetUp = true;
    }
};

void initializeQRCResourcesForHostChildTeardownTest()
{
#ifdef INCLUDE_VARIABLE_SPLASH_SCREEN
    qInitResources_additional_splash_screens();
#endif
#ifdef INCLUDE_FONTS
    qInitResources_mudlet_fonts_common();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    qInitResources_mudlet_fonts_posix();
#endif
#endif
    qInitResources_mudlet();
    qInitResources_qm();
}

#include "HostChildTeardownTest.moc"
QTEST_MAIN(HostChildTeardownTest)
