/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Developers                               *
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
 * Benchmarks for telnet data processing path using real Mudlet components.
 *
 * This tests the actual cTelnet::processSocketData -> TBuffer::translateToPlainText
 * pipeline to establish baselines for zero-copy optimizations.
 *
 * Run with: ctest -R TelnetBenchmark -V
 * For detailed timing: ./TelnetBenchmark -tickcounter
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
void initializeQRCResourcesForBenchmark();

class TelnetBenchmark : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Benchmark-Host";
    const QString mPort = "4001";
    const QString mLocalhost = "localhost";
    Host* mpHost = nullptr;

    // Test data of varying sizes
    QByteArray mSmallData;  // ~1KB typical MUD line
    QByteArray mMediumData; // ~10KB room description
    QByteArray mLargeData;  // ~100KB batch update

    static QByteArray generateMudTraffic(int lines)
    {
        QByteArray result;
        result.reserve(lines * 120);

        for (int i = 0; i < lines; ++i) {
            // ANSI color start
            result.append("\x1b[1;32m");
            // Typical MUD text
            result.append("You are standing in a dark forest. The trees tower above you. ");
            // ANSI reset
            result.append("\x1b[0m");
            // Line ending
            result.append("\r\n");

            // Every 5th line add a prompt with telnet GA
            if (i % 5 == 0) {
                result.append("\x1b[1;37m> \x1b[0m");
                result.append("\xff\xf9"); // IAC GA
            }
        }
        return result;
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
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for benchmark.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }

private slots:
    void initTestCase()
    {
        initializeQRCResourcesForBenchmark();

        // Generate test data
        mSmallData = generateMudTraffic(10);    // ~1.2KB
        mMediumData = generateMudTraffic(100);  // ~12KB
        mLargeData = generateMudTraffic(1000); // ~120KB

        qInfo() << "Test data sizes - Small:" << mSmallData.size()
                << "Medium:" << mMediumData.size()
                << "Large:" << mLargeData.size();
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, mPort.toUShort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(
            std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);

        startProfile(mHostname, mLocalhost, mPort);
    }

    /*
     * Benchmark: Small data (~1KB) through telnet path
     * Represents typical single-line MUD responses
     */
    void benchSmallData()
    {
        QVERIFY(mpHost != nullptr);

        QBENCHMARK {
            mpHost->mTelnet.loopbackTest(mSmallData);
        }
    }

    /*
     * Benchmark: Medium data (~10KB) through telnet path
     * Represents room descriptions, inventory lists
     */
    void benchMediumData()
    {
        QVERIFY(mpHost != nullptr);

        QBENCHMARK {
            mpHost->mTelnet.loopbackTest(mMediumData);
        }
    }

    /*
     * Benchmark: Large data (~100KB) through telnet path
     * Represents batch updates, log dumps
     */
    void benchLargeData()
    {
        QVERIFY(mpHost != nullptr);

        QBENCHMARK {
            mpHost->mTelnet.loopbackTest(mLargeData);
        }
    }

    void cleanup()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    void cleanupTestCase()
    {
    }
};

void initializeQRCResourcesForBenchmark()
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

#include "TelnetBenchmark.moc"
QTEST_MAIN(TelnetBenchmark)
