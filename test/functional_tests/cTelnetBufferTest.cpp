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
 *
 * To test this we must use raw char[] buffers - QByteArray always
 * null-terminates at position size() which masks the off-by-one.
 */

#include <QtTest/QtTest>
#include <cstring>

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

  // Uses a raw char[] buffer where position [amount] contains a garbage
  // byte. With the bug (null terminator at amount+1), the garbage byte
  // would be included in the processed output. With the fix (null
  // terminator at amount), it is correctly excluded.
  void testOffByOneNullTerminator() {
    QVERIFY(mpHost != nullptr);

    // Wait for any welcome data to be processed
    QTest::qWait(200);

    char buffer[64];
    std::memset(buffer, 0, sizeof(buffer));

    // Set up: 4 bytes of valid data, then a garbage byte at position 4
    // Bug: null terminator at buffer[5], so "Test!" (5 chars) is processed
    // Fix: null terminator at buffer[4], so "Test" (4 chars) is processed
    std::memcpy(buffer, "Test", 4);
    buffer[4] = '!';
    buffer[5] = '\0';

    mpHost->mTelnet.processSocketData(buffer, 4, true);
    QTest::qWait(100);

    QString displayed = mpHost->mpConsole->getCurrentLine("");
    QVERIFY2(!displayed.contains('!'),
             qPrintable(QString("Garbage '!' found in output - off-by-one bug "
                                "present. Got: '%1'")
                            .arg(displayed)));
    QVERIFY2(
        displayed.contains("Test"),
        qPrintable(
            QString("Expected 'Test' in output but got: '%1'").arg(displayed)));
  }

  // Same test at multiple sizes to catch edge cases at various boundaries
  void testOffByOneAtVariousSizes() {
    QVERIFY(mpHost != nullptr);

    QTest::qWait(200);

    const QVector<int> testSizes = {1, 2, 4, 8, 15, 16, 31, 32, 33, 63, 64};

    for (int size : testSizes) {
      char buffer[128];
      std::memset(buffer, 0, sizeof(buffer));

      // Fill with 'A's for the valid portion, put garbage right after
      std::memset(buffer, 'A', size);
      buffer[size] = '!';
      buffer[size + 1] = '\0';

      mpHost->mTelnet.processSocketData(buffer, size, true);
      QTest::qWait(50);

      QString displayed = mpHost->mpConsole->getCurrentLine("");
      QVERIFY2(
          !displayed.contains('!'),
          qPrintable(
              QString("Size %1: Garbage '!' found - off-by-one bug. Got: '%2'")
                  .arg(size)
                  .arg(displayed)));
    }
  }

  // Empty data should be handled gracefully (amount == 0)
  void testEmptyDataHandling() {
    QVERIFY(mpHost != nullptr);

    QTest::qWait(200);

    // Establish baseline
    QByteArray baselineData = "Baseline\r\n";
    mpHost->mTelnet.loopbackTest(baselineData);
    QTest::qWait(50);

    QString beforeEmpty = mpHost->mpConsole->getCurrentLine("");

    // Empty data should not corrupt console state
    QByteArray emptyData;
    mpHost->mTelnet.loopbackTest(emptyData);
    QTest::qWait(50);

    QString afterEmpty = mpHost->mpConsole->getCurrentLine("");
    QCOMPARE(afterEmpty, beforeEmpty);
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
