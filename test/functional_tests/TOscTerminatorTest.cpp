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

/*
 * Tests for OSC (Operating System Command) escape sequence terminator handling.
 * Verifies fix for issue #3912: BEL (0x07) terminator support.
 *
 * Run with: ctest -R TOscTerminatorTest -V
 */

#include <QtTest/QtTest>

#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForOscTest();

class TOscTerminatorTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "OSC-Test-Host";
    const QString mPort = "4002";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForOscTest();
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // Test that BEL-terminated OSC 2 (window title) doesn't swallow subsequent text
    void test_BelTerminatedOsc2_DoesNotSwallowText()
    {
        // OSC 2 (set window title) with BEL terminator, followed by regular text
        // ESC ] 2 ; Window Title BEL Hello World
        QString messageFromMud = QString("\x1b]2;Window Title\x07Hello World");
        QString expectedText = "Hello World";

        mpServer->setWelcomeMessage(messageFromMud);
        startProfile(mHostname, mLocalhost, mPort);
        QSignalSpy(mudlet::self()->getActiveHost()->mpConsole, &TMainConsole::signal_newDataAlert).wait(200);

        QString actualText = mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
        QCOMPARE(actualText, expectedText);
    }

    // Test that ST-terminated OSC sequences still work (regression test)
    void test_StTerminatedOsc_StillWorks()
    {
        // OSC with ST terminator (ESC \), followed by regular text
        // ESC ] P 0 F F 0 0 0 0 ESC \ Hello
        // Note: OSC P redefines colors, but we're just checking text after isn't swallowed
        QString messageFromMud = QString("\x1b]P0FF0000\x1b\\Hello");
        QString expectedText = "Hello";

        mpServer->setWelcomeMessage(messageFromMud);
        startProfile(mHostname, mLocalhost, mPort);
        QSignalSpy(mudlet::self()->getActiveHost()->mpConsole, &TMainConsole::signal_newDataAlert).wait(200);

        QString actualText = mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
        QCOMPARE(actualText, expectedText);
    }

    // Test that text after BEL-terminated OSC 8 (hyperlink) is displayed
    void test_BelTerminatedOsc8_TextDisplayed()
    {
        // OSC 8 hyperlink with BEL, some text, close OSC 8, more text
        // ESC ] 8 ; ; http://example.com BEL Link Text ESC ] 8 ; ; BEL After Link
        QString messageFromMud = QString("\x1b]8;;http://example.com\x07Link Text\x1b]8;;\x07 After Link");
        // The hyperlink text and subsequent text should both appear
        QString expectedContains = "After Link";

        mpServer->setWelcomeMessage(messageFromMud);
        startProfile(mHostname, mLocalhost, mPort);
        QSignalSpy(mudlet::self()->getActiveHost()->mpConsole, &TMainConsole::signal_newDataAlert).wait(200);

        QString actualText = mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
        QVERIFY2(actualText.contains(expectedContains),
                 qPrintable(QString("Expected text to contain '%1' but got '%2'").arg(expectedContains, actualText)));
    }

    // Test that BEL-terminated OSC P (color redefinition) works
    void test_BelTerminatedOscP_TextDisplayed()
    {
        // OSC P redefines color palette, with BEL terminator, followed by text
        // ESC ] P 0 F F 0 0 0 0 BEL Hello
        QString messageFromMud = QString("\x1b]P0FF0000\x07Hello");
        QString expectedText = "Hello";

        mpServer->setWelcomeMessage(messageFromMud);
        startProfile(mHostname, mLocalhost, mPort);
        QSignalSpy(mudlet::self()->getActiveHost()->mpConsole, &TMainConsole::signal_newDataAlert).wait(200);

        QString actualText = mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
        QCOMPARE(actualText, expectedText);
    }

    // Test that empty OSC sequence doesn't crash (edge case)
    void test_EmptyOscSequence_DoesNotCrash()
    {
        // OSC with immediate BEL terminator (empty content)
        // ESC ] BEL Normal text
        QString messageFromMud = QString("\x1b]\x07Normal text");
        QString expectedText = "Normal text";

        mpServer->setWelcomeMessage(messageFromMud);
        startProfile(mHostname, mLocalhost, mPort);
        QSignalSpy(mudlet::self()->getActiveHost()->mpConsole, &TMainConsole::signal_newDataAlert).wait(200);

        QString actualText = mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
        QCOMPARE(actualText, expectedText);
    }

    // Test that OSC sequence exceeding length limit doesn't hang and recovers gracefully
    void test_OscExceedsLengthLimit_DoesNotHang()
    {
        // Create an OSC sequence longer than 4096 bytes, followed by a BEL and normal text
        QString longContent = QString(5000, 'A');
        // ESC ] 2 ; <5000 A's> BEL Normal text
        QString messageFromMud = QString("\x1b]2;") + longContent + QString("\x07Normal text");
        QString expectedText = "Normal text";

        mpServer->setWelcomeMessage(messageFromMud);
        startProfile(mHostname, mLocalhost, mPort);
        QSignalSpy(mudlet::self()->getActiveHost()->mpConsole, &TMainConsole::signal_newDataAlert).wait(200);

        QString actualText = mudlet::self()->getActiveHost()->mpConsole->getCurrentLine("");
        // When length limit is exceeded, the parser scans forward for a terminator and
        // skips the malformed sequence. Text after the terminator should display normally.
        QCOMPARE(actualText, expectedText);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResourcesForOscTest()
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

#include "TOscTerminatorTest.moc"
QTEST_MAIN(TOscTerminatorTest)
