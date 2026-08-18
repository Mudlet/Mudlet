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
 * Regression test for a dangling TCommandLine* left behind in
 * TMainConsole::mSubCommandLineMap when the widget that owns the command line
 * is deleted.
 *
 * A TCommandLine is always a child widget of some other widget (the miniconsole
 * it is embedded in, or the user window / scroll box it was created into), so
 * Qt's parent-child ownership frees it along with that parent. The map entry
 * registered in TConsole::setCmdVisible() / TMainConsole::createCommandLine()
 * survived, leaving a non-null pointer to freed memory that every later lookup
 * of that name dereferenced.
 *
 * The last test here is the one that needs no Lua at all: TConsole::setFont()
 * walks the whole map and calls console() on every entry, and that walk is
 * reached from Host::setDisplayFont(), i.e. from changing the display font in
 * Preferences.
 *
 * Bootstrap mirrors the other functional tests.
 */

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TCommandLine.h"
#include "TConsole.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class SubCommandLineLifetimeTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "SubCommandLine-Test-Host";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    // The free happens through deleteLater(), so it only lands once control
    // returns to the event loop - which is exactly what makes the stale entry
    // point at freed memory rather than at a doomed but still live widget.
    void runDeferredDeletes()
    {
        QTest::qWait(50ms);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

private slots:
    // Start mudlet and create a profile once for all tests.
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
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
            QDir(path).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
    }

    // Route 1: enableCommandLine() on a miniconsole, then deleteMiniConsole().
    // The command line is a child of the miniconsole, so the miniconsole's
    // destruction frees it.
    void test_miniConsoleCommandLineDeregistersWhenConsoleDeleted()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString name = qsl("doomedMiniConsole");

        TConsole* miniConsole = console->createMiniConsole(QString(), name, 0, 0, 300, 100);
        QVERIFY2(miniConsole, "could not create the miniconsole");
        miniConsole->setCmdVisible(true); // what Lua enableCommandLine(name) does
        QVERIFY2(console->mSubCommandLineMap.contains(name), "command line not registered after enabling it");

        auto [deleted, deleteMsg] = console->deleteMiniConsole(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));
        runDeferredDeletes();

        QVERIFY2(!console->mSubCommandLineMap.contains(name), "stale command line entry left behind after deleting the miniconsole that owned it");

        // The observable non-crashing symptom of the stale entry: the name still
        // looks taken, so a fresh command line of that name cannot be made.
        auto [created, createMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(createMsg));
        console->deleteCommandLine(name);
        runDeferredDeletes();
    }

    // Route 2: createCommandLine() into a scroll box, then deleteScrollBox().
    void test_scrollBoxCommandLineDeregistersWhenScrollBoxDeleted()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString scrollBoxName = qsl("doomedScrollBox");
        const QString cmdLineName = qsl("scrollBoxCmdLine");

        QVERIFY2(console->createScrollBox(QString(), scrollBoxName, 0, 0, 300, 200), "could not create the scroll box");
        auto [created, createMsg] = console->createCommandLine(scrollBoxName, cmdLineName, 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(createMsg));
        QVERIFY(console->mSubCommandLineMap.contains(cmdLineName));

        auto [deleted, deleteMsg] = console->deleteScrollBox(scrollBoxName);
        QVERIFY2(deleted, qPrintable(deleteMsg));
        runDeferredDeletes();

        QVERIFY2(!console->mSubCommandLineMap.contains(cmdLineName), "stale command line entry left behind after deleting the scroll box that owned it");

        // Observable consequence: the name is free again.
        auto [recreated, recreateMsg] = console->createCommandLine(QString(), cmdLineName, 0, 0, 100, 30);
        QVERIFY2(recreated, qPrintable(recreateMsg));
        console->deleteCommandLine(cmdLineName);
        runDeferredDeletes();
    }

    // Route 3: createCommandLine() into a user window, then deleteMiniConsole()
    // on that user window - the dock owns the command line's parent widget.
    void test_userWindowCommandLineDeregistersWhenWindowDeleted()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString windowName = qsl("doomedUserWindow");
        const QString cmdLineName = qsl("userWindowCmdLine");

        auto [opened, openMsg] = mpHost->openWindow(windowName, /*loadLayout=*/false, /*autoDock=*/true, qsl("l"));
        QVERIFY2(opened, qPrintable(openMsg));
        auto [created, createMsg] = console->createCommandLine(windowName, cmdLineName, 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(createMsg));
        QVERIFY(console->mSubCommandLineMap.contains(cmdLineName));

        auto [deleted, deleteMsg] = console->deleteMiniConsole(windowName);
        QVERIFY2(deleted, qPrintable(deleteMsg));
        runDeferredDeletes();

        QVERIFY2(!console->mSubCommandLineMap.contains(cmdLineName), "stale command line entry left behind after deleting the user window that owned it");

        auto [recreated, recreateMsg] = console->createCommandLine(QString(), cmdLineName, 0, 0, 100, 30);
        QVERIFY2(recreated, qPrintable(recreateMsg));
        console->deleteCommandLine(cmdLineName);
        runDeferredDeletes();
    }

    // deleteCommandLine() must not leave the entry behind either - it takes the
    // entry itself, so the destructor has to cope with the name already gone.
    void test_deleteCommandLineDeregisters()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString name = qsl("explicitlyDeletedCmdLine");

        auto [created, createMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(createMsg));
        auto [deleted, deleteMsg] = console->deleteCommandLine(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));
        runDeferredDeletes();

        QVERIFY2(!console->mSubCommandLineMap.contains(name), "command line entry left behind after deleteCommandLine()");

        // Recreating under the same name must work.
        auto [recreated, recreateMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(recreated, qPrintable(recreateMsg));
        console->deleteCommandLine(name);
        runDeferredDeletes();
    }

    // The no-Lua route: changing the display font in Preferences ends up in
    // Host::setDisplayFont() -> TConsole::setFont(), which walks the whole
    // mSubCommandLineMap and calls console() on every entry. With a stale entry
    // present that is a read of freed memory (a clean heap-use-after-free under
    // AddressSanitizer). Kept last so the cheaper assertions above report first.
    void test_changingDisplayFontAfterDeletedWindowIsSafe()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString name = qsl("fontWalkMiniConsole");

        TConsole* miniConsole = console->createMiniConsole(QString(), name, 0, 0, 300, 100);
        QVERIFY2(miniConsole, "could not create the miniconsole");
        miniConsole->setCmdVisible(true);
        QVERIFY(console->mSubCommandLineMap.contains(name));

        auto [deleted, deleteMsg] = console->deleteMiniConsole(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));
        runDeferredDeletes();

        QFont changedFont = mpHost->getDisplayFont();
        changedFont.setPointSize(changedFont.pointSize() == 12 ? 14 : 12);
        auto [fontSet, fontMsg] = mpHost->setDisplayFont(changedFont);
        QVERIFY2(fontSet, qPrintable(fontMsg));

        QVERIFY2(!console->mSubCommandLineMap.contains(name), "stale command line entry survived into the setFont() walk");

        auto [recreated, recreateMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(recreated, qPrintable(recreateMsg));
        console->deleteCommandLine(name);
        runDeferredDeletes();
    }

    // Erasure has to be by value, not by name: a replacement registered under the
    // same name before the old widget's deferred delete has run must survive it.
    void test_recreatingBeforeTheDeferredDeleteKeepsTheNewCommandLine()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString name = qsl("reusedCmdLineName");

        auto [created, createMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(createMsg));
        auto [deleted, deleteMsg] = console->deleteCommandLine(name);
        QVERIFY2(deleted, qPrintable(deleteMsg));

        // Deliberately no event loop turn here - the old widget is still alive.
        auto [recreated, recreateMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(recreated, qPrintable(recreateMsg));
        TCommandLine* replacement = console->mSubCommandLineMap.value(name);
        QVERIFY(replacement);

        runDeferredDeletes();

        QVERIFY2(console->mSubCommandLineMap.value(name) == replacement, "the old command line's deregistration took the replacement with it");
        console->deleteCommandLine(name);
        runDeferredDeletes();
    }

    // Kept last on purpose: it leaves a registered command line behind, so that
    // cleanupTestCase()'s teardown destroys the console with one still in its
    // widget tree. ~TMainConsole has to drop its destroyed() handler first - that
    // handler runs from ~QWidget, which is after the console's own members,
    // mSubCommandLineMap included, have already been destroyed.
    void test_destroyingTheConsoleWithALiveCommandLineIsSafe()
    {
        TMainConsole* console = mpHost->mpConsole;
        const QString name = qsl("outlivesTheConsole");

        auto [created, createMsg] = console->createCommandLine(QString(), name, 0, 0, 100, 30);
        QVERIFY2(created, qPrintable(createMsg));
        QVERIFY(console->mSubCommandLineMap.contains(name));
    }
};

#include "SubCommandLineLifetimeTest.moc"
MUDLET_GROUPED_TEST_MAIN(SubCommandLineLifetimeTest)
