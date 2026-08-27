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
 * The notepad, the IRC client and the toolbars an action puts on the main
 * window are created without a Host parent, so nothing disposes of them along
 * with the profile unless the teardown does it by hand. Each test takes one of
 * the three orderings a Host goes away in and asserts the same thing: once the
 * Host is gone, so are its windows. The QPointers make a leak provable in any
 * build; an AddressSanitizer build additionally catches a double delete.
 *
 * Run with: ctest -R HostChildTeardownTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPlainTextEdit>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
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

#include "GroupedTest.h"

using namespace std::chrono_literals;

class HostChildTeardownTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    QString mPort;
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
        return TestProfile::create(profileName, address, port);
    }

    // A root action set to be a floating toolbar is what puts a TToolBar on the
    // main window, once the unit is asked to regenerate its toolbars.
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
            if (auto* note = qobject_cast<QPlainTextEdit*>(windows.notePad->tabWidget->widget(0))) {
                note->setPlainText(csmNoteText);
            }
        }

        // built directly rather than through openIrc(), which would connect to
        // the network
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

    OpenWindows mWindowsLeftOpenAtTheEnd;
    bool mLeftOpenProfileWasSetUp = false;
    const QString mProfileLeftOpenAtTheEnd = qsl("HostChildTeardown-LeftOpen");

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        // A config root of this process's own. Sharing the developer's
        // ~/.config/mudlet means sharing a profile list, so a second copy of
        // this test running at the same time is told the name it types is
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        // a stub that failed to bind only warns, and every test would then
        // report the profile as slow to load instead
        QVERIFY2(mpServer->serverPort() != 0, "The telnet stub did not start listening");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>(qsl("MudletInstanceCoordinator")));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
    }

    // A test that stops at a failed assertion leaves its Host in the pool, and
    // getActiveHost() could then hand the next test the wrong one. The profile
    // the last test leaves open on purpose is deliberately not named here.
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

        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            // getMudletPath() reads the main window, so the path has to be taken
            // while there still is one
            const QString leftOpenProfilePath = mudlet::getMudletPath(enums::profileHomePath, mProfileLeftOpenAtTheEnd);

            // The third ordering: a profile still loaded when the main window goes,
            // so the Host is destroyed with no close of any kind asked for.
            QVERIFY2(mLeftOpenProfileWasSetUp, "The profile this checks on was never opened, so the check below would pass on three null pointers");
            delete mudlet::self();
            // Only the notepad and the IRC client carry weight here: the toolbars
            // are children of the main window, so ~QWidget frees them either way.
            const QStringList leftBehind = windowsLeftBehind(mWindowsLeftOpenAtTheEnd);
            QVERIFY2(leftBehind.isEmpty(), qPrintable(qsl("Destroying the main window left %1 of the profile that was still loaded behind").arg(leftBehind.join(qsl(" and ")))));

            QDir(leftOpenProfilePath).removeRecursively();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Nothing calls closeChildren(): the host pool simply lets go of the Host.
    // Mudlet reaches this whenever the profile's main console has already gone,
    // as Host::requestClose() then returns before it gets to closeChildren().
    void test_destroyingTheHostTakesItsWindowsWithIt()
    {
        const QString profileName = qsl("HostChildTeardown-NoCloseChildren");
        Host* pHost = startProfile(profileName);
        QVERIFY2(pHost, "Profile took too long to load");

        const OpenWindows windows = openEveryChildWindow(pHost);
        QVERIFY2(everyWindowWasOpened(windows), "Not all of the profile's windows were opened");

        // forceClose() stops TMainConsole::closeEvent() asking whether to save,
        // which would block on a modal dialog here
        pHost->forceClose();
        pHost->mpConsole->close();
        QTRY_VERIFY2(pHost->mpConsole.isNull(), "The main console did not go away"); // Qt 6 disposes of a WA_DeleteOnClose widget by deleteLater()
        QVERIFY2(pHost->requestClose(), "Closing the profile was refused");
        QVERIFY2(pHost->mpNotePad, "requestClose() reached closeChildren() after all - this no longer tests a Host that skips it");

        const QPointer<Host> hostGuard(pHost);
        pHost = nullptr;
        mudlet::self()->getHostManager().deleteHost(profileName);
        QVERIFY2(hostGuard.isNull(), "The Host outlived deleteHost(), so ~Host() never ran");

        const QStringList leftBehind = windowsLeftBehind(windows);
        QVERIFY2(leftBehind.isEmpty(), qPrintable(qsl("Destroying the Host left %1 behind").arg(leftBehind.join(qsl(" and ")))));
        // only reaches the disk if ~Host() closed the notepad rather than just
        // deleting it
        QCOMPARE(noteContentOnDisk(profileName), csmNoteText);

        deleteProfileDirectory(profileName);
    }

    // closeChildren() disposes of these windows through deleteLater(), and
    // mudlet::closeEvent() destroys the Host in the same call stack, so ~Host()
    // meets windows whose deferred delete has not run yet.
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

        // checked before the event loop gets a turn, so that the deletes
        // closeChildren() deferred cannot be what satisfies it
        for (const auto& pToolBar : windows.toolBars) {
            QVERIFY2(pToolBar.isNull(), "Destroying the Host did not delete a toolbar closeChildren() had only queued");
        }

        // letting what closeChildren() deferred run is where a second delete of
        // anything ~Host() already took would land
        QTRY_VERIFY2(windowsLeftBehind(windows).isEmpty(), "Closing and then destroying the Host left one of its windows behind");
        deleteProfileDirectory(profileName);
    }

    // cleanupTestCase() is what destroys the main window on top of it.
    void test_leaveAProfileOpenForTheMainWindowToTakeDown()
    {
        Host* pHost = startProfile(mProfileLeftOpenAtTheEnd);
        QVERIFY2(pHost, "Profile took too long to load");

        mWindowsLeftOpenAtTheEnd = openEveryChildWindow(pHost);
        QVERIFY2(everyWindowWasOpened(mWindowsLeftOpenAtTheEnd), "Not all of the profile's windows were opened");
        mLeftOpenProfileWasSetUp = true;
    }
};

#include "HostChildTeardownTest.moc"
MUDLET_GROUPED_TEST_MAIN(HostChildTeardownTest)
