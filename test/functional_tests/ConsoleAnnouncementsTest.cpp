/***************************************************************************
 *   Copyright (C) 2026 by Mike Conley - mike.conley@stickmud.com          *
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

#include <QAccessible>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "TAccessibleTextEdit.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TConsoleAnnouncements.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
#include "TelnetServerStub.h"
#include "mudlet.h"

#include "GroupedTest.h"

class ConsoleAnnouncementsTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = qsl("Test-ConsoleAnnouncements");
    QString mpPort;
    const QString mpLocalhost = qsl("localhost");

    inline static QStringList smAnnouncements;
    inline static int smRemovals = 0;

    static void record(QAccessibleEvent* event)
    {
        if (event->type() == QAccessible::Announcement) {
            smAnnouncements.append(static_cast<QAccessibleAnnouncementEvent*>(event)->message());
        } else if (event->type() == QAccessible::TextRemoved) {
            ++smRemovals;
        }
    }

    // Qt only hands events to the handler while accessibility is active, and with no screen reader here only
    // setActive() makes it so
    static bool startRecording()
    {
        smAnnouncements.clear();
        smRemovals = 0;
        QAccessible::installUpdateHandler(record);
        QAccessible::setActive(true);
        return QAccessible::isActive();
    }

    static void stopRecording()
    {
        QAccessible::setActive(false);
        QAccessible::installUpdateHandler(nullptr);
    }

    static void print(TBuffer& buffer, const QString& text) { buffer.append(text, 0, text.size(), Qt::white, Qt::black); }

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
        // main() installs it in the app; a removal is raised on the pane's accessible text
        QAccessible::installFactory(TAccessibleTextEdit::textEditFactory);
    }

    void cleanupTestCase()
    {
        QAccessible::removeFactory(TAccessibleTextEdit::textEditFactory);
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        TestProfile::removeProfileDirectory(mpHostname);
    }

    void cleanup()
    {
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        TestProfile::removeProfileDirectory(mpHostname);
    }

    void test_newLinesAreAnnouncedJoinedAndTrimmed()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        print(buffer, qsl("a\nb\nc\n"));
        const bool active = startRecording();
        consoleAnnouncements::announceNewLines(pHost->mainConsoleView()->mUpperPane, buffer, 1, 3);
        stopRecording();
        if (!active) {
            QSKIP("QAccessible::setActive(true) does not make isActive() true without a screen reader before Qt 6.10");
        }
        QCOMPARE(smAnnouncements, QStringList{qsl("b\nc")});
        QCOMPARE(smRemovals, 0);
    }

    // Lines a clearWindow() or a trim took away are not new text to read out again
    void test_fewerLinesThanBeforeAnnounceNothingNew()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        TBuffer buffer(pHost);
        print(buffer, qsl("a\nb\nc\n"));
        const bool active = startRecording();
        consoleAnnouncements::announceNewLines(pHost->mainConsoleView()->mUpperPane, buffer, 3, 1);
        stopRecording();
        if (!active) {
            QSKIP("QAccessible::setActive(true) does not make isActive() true without a screen reader before Qt 6.10");
        }
        QCOMPARE(smRemovals, 1);
        QVERIFY(smAnnouncements.isEmpty());
    }

    void test_wantedFollowsTheProfileAndTheActiveHost()
    {
        Host* pHost = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        QVERIFY(pHost);
        QCOMPARE(mudlet::self()->getActiveHost(), pHost);
        const bool active = startRecording();
        pHost->mAnnounceIncomingText = false;
        const bool wantedWhenOff = consoleAnnouncements::wanted(*pHost, true);
        pHost->mAnnounceIncomingText = true;
        const bool wantedForMain = consoleAnnouncements::wanted(*pHost, true);
        const bool wantedForOther = consoleAnnouncements::wanted(*pHost, false);
        stopRecording();
        if (!active) {
            QSKIP("QAccessible::setActive(true) does not make isActive() true without a screen reader before Qt 6.10");
        }
        QVERIFY(!wantedWhenOff);
        QVERIFY(wantedForMain);
        QVERIFY(!wantedForOther);
        QVERIFY(!consoleAnnouncements::wanted(*pHost, true));
    }
};

#include "ConsoleAnnouncementsTest.moc"
MUDLET_GROUPED_TEST_MAIN(ConsoleAnnouncementsTest)
