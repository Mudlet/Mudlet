/***************************************************************************
 *   Copyright (C) 2026 by Mudlet contributors                             *
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

#include <QtTest/QtTest>

#include <chrono>

#include <zlib.h>
#include <zstd.h>

#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForMccp4();

using namespace std::chrono_literals;

// End-to-end cover for MCCP4 (telnet option 88): the negotiation handshake and
// both decompression paths (zstd and deflate).
//
// The same-packet cases matter most. MCCP1/2 announce compression with a
// malformed sequence that processSocketData() spots with a look-ahead, so it
// can switch to the decompressor part-way through a buffer. MCCP4 instead uses
// an ordinary IAC SB ... IAC SE subnegotiation, and real servers write it into
// the same TCP segment as the first compressed bytes, so the handover has to
// work mid-buffer there too.
class TelnetMccp4Test : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-Telnet-MCCP4";
    QString mPort; // the stub's actual ephemeral port, assigned in init()
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResourcesForMccp4(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    // IAC WILL COMPRESS4 must be answered with IAC DO COMPRESS4 followed by the
    // list of encodings this client can actually decode.
    void test_negotiationOffersSupportedEncodings()
    {
        auto* host = connectedHost();
        QVERIFY2(host, "No active host available for the test.");

        mpServer->clearReceivedData();
        offerCompress4(host);

        QByteArray expectedDo;
        expectedDo.append(TN_IAC);
        expectedDo.append(TN_DO);
        expectedDo.append(OPT_COMPRESS4);

        QByteArray expectedOffer;
        expectedOffer.append(TN_IAC);
        expectedOffer.append(TN_SB);
        expectedOffer.append(OPT_COMPRESS4);
        expectedOffer.append(MCCP4_ACCEPT_ENCODING);
        expectedOffer.append("zstd,deflate");
        expectedOffer.append(TN_IAC);
        expectedOffer.append(TN_SE);

        QVERIFY2(waitForServerToReceive(expectedDo), "Client did not answer IAC WILL COMPRESS4 with IAC DO COMPRESS4.");
        QVERIFY2(waitForServerToReceive(expectedOffer), qPrintable(qsl("Client did not offer its supported MCCP4 encodings. It sent: %1").arg(QString(mpServer->receivedData().toHex(' ')))));
    }

    // BEGIN_ENCODING arriving on its own, with the compressed bytes in a later
    // read - the easy case, where processSocketData() is re-entered with
    // mNeedDecompression already set.
    void test_zstdInLaterPacketIsDecompressed()
    {
        auto* host = connectedHost();
        QVERIFY2(host, "No active host available for the test.");
        offerCompress4(host);

        QByteArray begin = beginEncoding("zstd");
        feed(host, begin);

        QByteArray payload = zstdCompress("MCCP4_ZSTD_LATER_PACKET\r\n");
        feed(host, payload);

        QVERIFY2(waitForBufferToContain("MCCP4_ZSTD_LATER_PACKET"), qPrintable(qsl("zstd payload sent after BEGIN_ENCODING was not decompressed. Buffer holds:\n%1").arg(bufferContents())));
    }

    // BEGIN_ENCODING and the first compressed bytes in a single read, which is
    // what the reference MCCP4 servers actually do.
    void test_zstdInSamePacketIsDecompressed()
    {
        auto* host = connectedHost();
        QVERIFY2(host, "No active host available for the test.");
        offerCompress4(host);

        QByteArray data = beginEncoding("zstd");
        data.append(zstdCompress("MCCP4_ZSTD_SAME_PACKET\r\n"));
        feed(host, data);

        QVERIFY2(waitForBufferToContain("MCCP4_ZSTD_SAME_PACKET"), qPrintable(qsl("zstd payload in the same packet as BEGIN_ENCODING was not decompressed. Buffer holds:\n%1").arg(bufferContents())));
    }

    void test_deflateInLaterPacketIsDecompressed()
    {
        auto* host = connectedHost();
        QVERIFY2(host, "No active host available for the test.");
        offerCompress4(host);

        QByteArray begin = beginEncoding("deflate");
        feed(host, begin);

        QByteArray payload = deflateCompress("MCCP4_DEFLATE_LATER_PACKET\r\n");
        feed(host, payload);

        QVERIFY2(waitForBufferToContain("MCCP4_DEFLATE_LATER_PACKET"), qPrintable(qsl("deflate payload sent after BEGIN_ENCODING was not decompressed. Buffer holds:\n%1").arg(bufferContents())));
    }

    void test_deflateInSamePacketIsDecompressed()
    {
        auto* host = connectedHost();
        QVERIFY2(host, "No active host available for the test.");
        offerCompress4(host);

        QByteArray data = beginEncoding("deflate");
        data.append(deflateCompress("MCCP4_DEFLATE_SAME_PACKET\r\n"));
        feed(host, data);

        QVERIFY2(waitForBufferToContain("MCCP4_DEFLATE_SAME_PACKET"),
                 qPrintable(qsl("deflate payload in the same packet as BEGIN_ENCODING was not decompressed. Buffer holds:\n%1").arg(bufferContents())));
    }

    // An encoding this client never offered must not silently leave the
    // connection stuck in a compressed state.
    void test_unsupportedEncodingIsRejected()
    {
        auto* host = connectedHost();
        QVERIFY2(host, "No active host available for the test.");
        offerCompress4(host);

        QByteArray data = beginEncoding("brotli");
        data.append("MCCP4_PLAIN_AFTER_REJECT\r\n");
        feed(host, data);

        QVERIFY2(waitForBufferToContain("MCCP4_PLAIN_AFTER_REJECT"), qPrintable(qsl("Plain text after an unsupported MCCP4 encoding was not displayed. Buffer holds:\n%1").arg(bufferContents())));
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

private:
    // IAC SB COMPRESS4 BEGIN_ENCODING <name> IAC SE
    QByteArray beginEncoding(const QByteArray& encoding) const
    {
        QByteArray data;
        data.append(TN_IAC);
        data.append(TN_SB);
        data.append(OPT_COMPRESS4);
        data.append(MCCP4_BEGIN_ENCODING);
        data.append(encoding);
        data.append(TN_IAC);
        data.append(TN_SE);
        return data;
    }

    QByteArray zstdCompress(const QByteArray& plain) const
    {
        const size_t bound = ZSTD_compressBound(static_cast<size_t>(plain.size()));
        QByteArray out(static_cast<qsizetype>(bound), '\0');
        const size_t written = ZSTD_compress(out.data(), bound, plain.constData(), static_cast<size_t>(plain.size()), 3);
        if (ZSTD_isError(written)) {
            return {};
        }
        out.resize(static_cast<qsizetype>(written));
        return out;
    }

    // zlib-wrapped deflate, which is what inflateInit()/initStreamDecompressor() expects
    QByteArray deflateCompress(const QByteArray& plain) const
    {
        uLongf bound = compressBound(static_cast<uLong>(plain.size()));
        QByteArray out(static_cast<qsizetype>(bound), '\0');
        const int result = compress2(reinterpret_cast<Bytef*>(out.data()), &bound, reinterpret_cast<const Bytef*>(plain.constData()), static_cast<uLong>(plain.size()), 6);
        if (result != Z_OK) {
            return {};
        }
        out.resize(static_cast<qsizetype>(bound));
        return out;
    }

    // processSocketData() writes a NUL at in_buffer[size + 1], so the backing
    // buffer needs a little slack past the data it is handed.
    void feed(Host* host, QByteArray& data)
    {
        data.reserve(data.size() + 16);
        host->mTelnet.loopbackTest(data);
    }

    void offerCompress4(Host* host)
    {
        QByteArray will;
        will.append(TN_IAC);
        will.append(TN_WILL);
        will.append(OPT_COMPRESS4);
        feed(host, will);
    }

    bool waitForServerToReceive(const QByteArray& needle, int timeoutMs = 5000) const
    {
        return QTest::qWaitFor(
                [&]() {
                    return mpServer->receivedData().contains(needle);
                },
                timeoutMs);
    }

    bool waitForBufferToContain(const QString& text, int timeoutMs = 5000) const
    {
        return QTest::qWaitFor(
                [&]() {
                    return bufferContents().contains(text);
                },
                timeoutMs);
    }

    QString bufferContents() const
    {
        auto* host = mudlet::self()->getActiveHost();
        if (!host || !host->mpConsole) {
            return {};
        }
        auto& buffer = host->mpConsole->buffer;
        QStringList lines;
        for (int i = 0; i <= buffer.getLastLineNumber(); ++i) {
            lines << buffer.line(i);
        }
        return lines.join(QChar::LineFeed);
    }

    // Starts a profile the way a user would through the GUI, and waits until it
    // is connected to the stub
    Host* connectedHost()
    {
        QTimer::singleShot(0, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy profileLoaded(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!profileLoaded.wait(10000)) {
            return nullptr;
        }
        auto* host = mudlet::self()->getActiveHost();
        if (!host) {
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(5000)) {
            return nullptr;
        }
        return host;
    }

    void deleteProfileDirectory(const QString& profileName) const
    {
        QDir dir(mudlet::getMudletPath(enums::profileHomePath, profileName));
        if (dir.exists()) {
            dir.removeRecursively();
        }
    }
};

void initializeQRCResourcesForMccp4()
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

#include "TelnetMccp4Test.moc"
QTEST_MAIN(TelnetMccp4Test)
