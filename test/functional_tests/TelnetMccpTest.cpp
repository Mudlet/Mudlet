/***************************************************************************
 *   Copyright (C) 2025-2026 by Mudlet Developers                          *
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
 * Regression tests for MCCP (Mud Client Compression Protocol) packet boundary handling.
 *
 * These tests verify fixes for issue #6624 where MCCP compression failed when
 * the start sequence (IAC SB COMPRESS2 IAC SE) was split across packet boundaries.
 *
 * Run with: ctest -R TelnetMccpTest -V
 */

#include <QtTest/QtTest>
#include <zlib.h>

#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForMccpTest();

class TelnetMccpTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "MCCP-Test";
    const QString mPort = "4002";
    const QString mLocalhost = "localhost";
    Host* mpHost = nullptr;

    // Compress data using zlib deflate (matching cTelnet's expectations)
    static QByteArray compressData(const QByteArray& input)
    {
        QByteArray output;
        output.resize(compressBound(input.size()) + 100);

        z_stream stream;
        stream.zalloc = Z_NULL;
        stream.zfree = Z_NULL;
        stream.opaque = Z_NULL;

        // Use default compression, matching typical MUD server behavior
        if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK) {
            return {};
        }

        stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));
        stream.avail_in = input.size();
        stream.next_out = reinterpret_cast<Bytef*>(output.data());
        stream.avail_out = output.size();

        // Use Z_SYNC_FLUSH to ensure all data is flushed
        int ret = deflate(&stream, Z_SYNC_FLUSH);
        if (ret != Z_OK) {
            deflateEnd(&stream);
            return {};
        }

        output.resize(stream.total_out);
        deflateEnd(&stream);
        return output;
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
            QFAIL("No active host available for test.");
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
        initializeQRCResourcesForMccpTest();
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
     * Test: MCCP v2 start sequence split across packet boundaries
     *
     * This is a regression test for issue #6624. The old code used lookahead
     * to detect the MCCP start sequence, which failed when the sequence was
     * split across TCP packets:
     *   Packet 1: IAC SB COMPRESS2
     *   Packet 2: IAC SE + compressed_data
     *
     * The fix uses a flag set in processTelnetCommand() after the complete
     * subnegotiation is received via the state machine.
     */
    void test_MccpV2StartSequenceAcrossPackets()
    {
        QVERIFY(mpHost != nullptr);

        const QString testMessage = "Hello from MCCP compressed stream";

        // Step 1: Server offers MCCP v2 (IAC WILL COMPRESS2)
        // This causes client to set mMCCP_version_2 = true and respond with DO
        QByteArray willCompress2;
        willCompress2.append(TN_IAC);
        willCompress2.append(TN_WILL);
        willCompress2.append(OPT_COMPRESS2);
        mpHost->mTelnet.loopbackTest(willCompress2);

        // Brief wait for negotiation to process
        QTest::qWait(50);

        // Step 2: First packet - partial MCCP start sequence (IAC SB COMPRESS2)
        // This is where the old code would fail - it looked ahead for IAC SE
        // but couldn't find it in the same buffer
        QByteArray packet1;
        packet1.append(TN_IAC);
        packet1.append(TN_SB);
        packet1.append(OPT_COMPRESS2);
        mpHost->mTelnet.loopbackTest(packet1);

        // Brief wait to simulate packet boundary timing
        QTest::qWait(10);

        // Step 3: Second packet - rest of start sequence + compressed data
        // (IAC SE + zlib_compressed_data)
        QByteArray uncompressed = testMessage.toUtf8() + "\r\n";
        QByteArray compressed = compressData(uncompressed);
        QVERIFY2(!compressed.isEmpty(), "Failed to compress test data");

        QByteArray packet2;
        packet2.append(TN_IAC);
        packet2.append(TN_SE);
        packet2.append(compressed);
        mpHost->mTelnet.loopbackTest(packet2);

        // Wait for processing and display
        QTest::qWait(200);

        // Verify the decompressed text appears in the console
        QString consoleLine = mpHost->mpConsole->getCurrentLine("");
        QVERIFY2(consoleLine.contains(testMessage),
                 qPrintable(QString("Expected '%1' in console, got '%2'")
                            .arg(testMessage, consoleLine)));
    }

    /*
     * Test: MCCP v2 with complete sequence in single packet (baseline)
     *
     * This verifies that MCCP works correctly when the start sequence
     * is NOT split across packets (the normal case).
     */
    void test_MccpV2CompleteSequenceInSinglePacket()
    {
        QVERIFY(mpHost != nullptr);

        const QString testMessage = "Complete sequence test message";

        // Step 1: Server offers MCCP v2
        QByteArray willCompress2;
        willCompress2.append(TN_IAC);
        willCompress2.append(TN_WILL);
        willCompress2.append(OPT_COMPRESS2);
        mpHost->mTelnet.loopbackTest(willCompress2);

        QTest::qWait(50);

        // Step 2: Complete MCCP start sequence + compressed data in one packet
        QByteArray uncompressed = testMessage.toUtf8() + "\r\n";
        QByteArray compressed = compressData(uncompressed);
        QVERIFY2(!compressed.isEmpty(), "Failed to compress test data");

        QByteArray packet;
        packet.append(TN_IAC);
        packet.append(TN_SB);
        packet.append(OPT_COMPRESS2);
        packet.append(TN_IAC);
        packet.append(TN_SE);
        packet.append(compressed);
        mpHost->mTelnet.loopbackTest(packet);

        QTest::qWait(200);

        QString consoleLine = mpHost->mpConsole->getCurrentLine("");
        QVERIFY2(consoleLine.contains(testMessage),
                 qPrintable(QString("Expected '%1' in console, got '%2'")
                            .arg(testMessage, consoleLine)));
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

void initializeQRCResourcesForMccpTest()
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

#include "TelnetMccpTest.moc"
QTEST_MAIN(TelnetMccpTest)
