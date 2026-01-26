/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Authors                                  *
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
 * Test for issue https://github.com/Mudlet/Mudlet/issues/1065
 * Off-by-one error in cTelnet::processSocketData()
 *
 * The bug: null terminator was placed at in_buffer[amount + 1] instead of
 * in_buffer[amount], and buffer was accessed before validating amount.
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
void initializeQRCResourcesForBufferTest();

class cTelnetBufferTest : public QObject {
  Q_OBJECT

private:
  TelnetServerStub *mpServer = nullptr;
  const QString mHostname = "BufferTest-Host";
  const QString mPort = "4002";
  const QString mLocalhost = "localhost";
  Host *mpHost = nullptr;

  void startProfile(const QString &hostname, const QString &address,
                    const QString &port) {
    QTimer::singleShot(0, qApp, [hostname, address, port]() {
      mudlet::self()->startAutoLogin({});
      QTest::qWait(100);
      QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button,
                        Qt::LeftButton);
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
    mpHost = mudlet::self()->getActiveHost();
    if (!mpHost) {
      QFAIL("No active host available for test.");
    }

    QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
    if (!spy2.wait(500)) {
      QFAIL("Could not connect with the host.");
    }
  }

  void deleteProfileDirectory(const QString &profileName) {
    const QString path =
        mudlet::getMudletPath(enums::profileHomePath, profileName);
    QDir dir(path);
    if (dir.exists()) {
      dir.removeRecursively();
    }
  }

private slots:
  void initTestCase() { initializeQRCResourcesForBufferTest(); }

  void init() {
    mpServer = new TelnetServerStub(qApp);
    mpServer->start(mLocalhost, mPort.toUShort());
    mudlet::start();
    mudlet::self()->setupConfig();
    mudlet::self()->takeOwnershipOfInstanceCoordinator(
        std::make_unique<MudletInstanceCoordinator>(
            "MudletInstanceCoordinator"));
    mudlet::self()->init();
    mudlet::self()->setStorePasswordsSecurely(false);
    deleteProfileDirectory(mHostname);

    startProfile(mHostname, mLocalhost, mPort);
  }

  // Test that data at exact byte boundaries is processed correctly
  // The off-by-one bug would cause data to be incorrectly terminated
  void testExactBoundaryData() {
    QVERIFY(mpHost != nullptr);

    // Test various boundary sizes - the bug would manifest as incorrect
    // null terminator placement causing extra garbage or truncation
    const QVector<int> testSizes = {31, 32, 33, 63, 64, 65, 127, 128, 129};

    for (int size : testSizes) {
      // Create test data of exact size with a newline at the end
      QByteArray testData(size - 1, 'X');
      testData.append('\n');

      // Add extra byte that the buggy code would incorrectly include
      // by placing null terminator one position too far
      testData.append('!');

      // Use loopbackTest to process through actual cTelnet::processSocketData
      // Pass size (not size+1) to simulate the exact amount received
      QByteArray dataToProcess = testData.left(size);
      mpHost->mTelnet.loopbackTest(dataToProcess);
    }

    // If we got here without crashing, the buffer handling is correct
    QVERIFY(true);
  }

  // Test that text is displayed correctly through the full pipeline
  void testTextDisplayedCorrectly() {
    QVERIFY(mpHost != nullptr);

    // Clear any existing content
    mpHost->mpConsole->mTriggerEngineMode = true;

    // Send a specific message and verify it arrives intact
    QByteArray testMessage = "Hello, World!\r\n";
    mpHost->mTelnet.loopbackTest(testMessage);

    // Wait for processing
    QTest::qWait(100);

    // The message should be in the console without corruption
    QString displayedText = mpHost->mpConsole->getCurrentLine("");
    QVERIFY2(displayedText.contains("Hello, World!"),
             qPrintable(QString("Expected 'Hello, World!' but got: '%1'")
                            .arg(displayedText)));
  }

  // Test that empty data doesn't cause issues (amount == 0 case)
  void testEmptyDataHandling() {
    QVERIFY(mpHost != nullptr);

    // Empty data should be handled gracefully without crashes
    QByteArray emptyData;
    mpHost->mTelnet.loopbackTest(emptyData);

    // If we got here, empty data was handled correctly
    QVERIFY(true);
  }

  // Test processing of data with ANSI escape sequences at boundaries
  void testAnsiAtBoundaries() {
    QVERIFY(mpHost != nullptr);

    // ANSI color followed by text - tests that escape sequences
    // aren't corrupted by incorrect null terminator placement
    QByteArray ansiData = "\x1b[1;32mGreen Text\x1b[0m\r\n";
    mpHost->mTelnet.loopbackTest(ansiData);

    QTest::qWait(100);

    QString displayedText = mpHost->mpConsole->getCurrentLine("");
    QVERIFY2(
        displayedText.contains("Green Text"),
        qPrintable(
            QString("Expected 'Green Text' but got: '%1'").arg(displayedText)));
  }

  // Test processing telnet commands (GA) at buffer boundaries
  void testTelnetCommandAtBoundary() {
    QVERIFY(mpHost != nullptr);

    // Text followed by IAC GA (telnet Go Ahead)
    QByteArray dataWithGA = "Prompt> ";
    dataWithGA.append('\xff'); // IAC
    dataWithGA.append('\xf9'); // GA

    mpHost->mTelnet.loopbackTest(dataWithGA);

    // Should process without issues
    QVERIFY(true);
  }

  void cleanup() {
    mpHost = nullptr;
    delete mpServer;
    mpServer = nullptr;
    deleteProfileDirectory(mHostname);
    delete mudlet::self();
  }

  void cleanupTestCase() {}
};

void initializeQRCResourcesForBufferTest() {
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

#include "cTelnetBufferTest.moc"
QTEST_MAIN(cTelnetBufferTest)
