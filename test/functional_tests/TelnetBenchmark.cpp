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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <chrono>

#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetBenchmark : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Benchmark-Host";
    QString mPort; // assigned the stub's actual ephemeral port in init()
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
        mpHost = TestProfile::create(hostname, address, port);
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
        // already in use and never gets an enabled Connect button. Since #9712
        // the opt-in that makes setupConfig() adopt a directory is
        // $XDG_CONFIG_HOME/mudlet/profiles, not the mudlet directory alone.
        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

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
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }
};

#include "TelnetBenchmark.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetBenchmark)
