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

#include <QtTest/QtTest>

#include <chrono>

#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include <QHostAddress>
#include <QNetworkReply>
#include <QPointer>
#include <QProgressDialog>
#include <QTcpServer>
#include <QRegularExpression>

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForTlsPrompt();

using namespace std::chrono_literals;

// Exercises the widget-free seam introduced when cTelnet was de-widgeted: when
// MSSP advertises a secure (TLS) port on an unencrypted connection, cTelnet must
// no longer construct a QMessageBox itself; instead it emits
// signal_promptTlsAvailable() carrying the (already translated) strings, and the
// frontend owns the modal dialog. This test verifies the signal is emitted with
// the expected payload.
class TelnetTlsPromptTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-Telnet-Tls-Prompt";
    const QString mLocalhost = "localhost";
    QString mPort;

private slots:
    void initTestCase() { initializeQRCResourcesForTlsPrompt(); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Bind an ephemeral OS-assigned port so parallel test runs (e.g. across
        // git worktrees) do not collide on a shared fixed port.
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void test_msspTlsPortEmitsPromptSignal()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        // Detach the frontend's modal handler: mudlet::addConsoleForNewHost
        // connects a lambda that would call QMessageBox::exec() and block the
        // test (there is no user to click it). Disconnect by signal alone
        // (nullptr receiver/slot) rather than by receiver: if the frontend
        // wiring ever moves off mudlet::self() this still detaches every
        // handler, so the test cannot hang forever inside exec().
        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, nullptr, nullptr);

        QSignalSpy spy(&host->mTelnet, &cTelnet::signal_promptTlsAvailable);
        QVERIFY(spy.isValid());

        // Build an MSSP subnegotiation advertising a secure port:
        //   IAC SB MSSP  MSSP_VAR "TLS" MSSP_VAL "48000"  IAC SE
        const QByteArray tlsPort = "48000";
        QByteArray data;
        data.append(TN_IAC);
        data.append(TN_SB);
        data.append(OPT_MSSP);
        data.append(MSSP_VAR);
        data.append("TLS");
        data.append(MSSP_VAL);
        data.append(tlsPort);
        data.append(TN_IAC);
        data.append(TN_SE);
        // processSocketData() writes a NUL at in_buffer[size + 1], so give the
        // backing buffer a little slack before handing it its data pointer.
        data.reserve(data.size() + 16);

        host->mTelnet.loopbackTest(data);

        // The signal is emitted synchronously inside loopbackTest(), so it has
        // almost certainly already arrived; only wait if it somehow has not,
        // otherwise an unconditional spy.wait() would burn its full timeout.
        if (spy.isEmpty()) {
            QVERIFY2(spy.wait(2s), "cTelnet did not emit signal_promptTlsAvailable when MSSP advertised a TLS port.");
        }
        QCOMPARE(spy.count(), 1);

        // The informative text carries the advertised port, keeping the string
        // (and its translation context) inside cTelnet.
        const QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.size(), 2);
        const QString informativeText = arguments.at(1).toString();
        QVERIFY2(informativeText.contains(QString::fromLatin1(tlsPort)), qPrintable(QString("informativeText did not mention the TLS port: %1").arg(informativeText)));

        // cTelnet recorded the advertised port as well.
        QCOMPARE(host->mMSSPTlsPort, tlsPort.toInt());
#endif
    }

    // No path: declining the TLS upgrade must leave the port and ssl_tsl
    // untouched, stop the client asking again this session, and still bring the
    // connection back (slot_tlsUpgradeResponse() does disconnectIt() +
    // reconnect() against the stub, which accepts repeat connections).
    void test_tlsUpgradeDeclinedKeepsPortAndStopsAsking()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        // Detach the frontend modal handler so no dialog blocks the test.
        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, nullptr, nullptr);

        const int originalPort = host->getPort();
        const bool originalSsl = host->mSslTsl;

        // Advertise a secure port so mMSSPTlsPort is populated (the handler is
        // detached, so nothing pops up).
        QByteArray advertise = msspTlsPayload("48000");
        advertise.reserve(advertise.size() + 16);
        host->mTelnet.loopbackTest(advertise);

        // The user answers No; the reconnect that follows must complete.
        QSignalSpy connectedSpy(&host->mTelnet, &cTelnet::signal_connected);
        host->mTelnet.slot_tlsUpgradeResponse(false);
        QVERIFY2(connectedSpy.wait(5s), "Declining the TLS upgrade did not reconnect to the server.");

        QVERIFY2(!host->mAskTlsAvailable, "Declining the TLS upgrade did not stop the client asking again.");
        QCOMPARE(host->getPort(), originalPort);
        QCOMPARE(host->mSslTsl, originalSsl);

        // Re-advertising the same secure port must NOT prompt again now that
        // the user has declined (don't-ask-again is sticky for the session).
        QSignalSpy promptSpy(&host->mTelnet, &cTelnet::signal_promptTlsAvailable);
        QByteArray advertiseAgain = msspTlsPayload("48000");
        advertiseAgain.reserve(advertiseAgain.size() + 16);
        host->mTelnet.loopbackTest(advertiseAgain);
        QCOMPARE(promptSpy.count(), 0);
#endif
    }

    // Yes path: accepting switches the profile to the advertised secure port
    // and starts a fresh (encrypted) connection to it. A second plain-TCP stub
    // stands in for the secure server: connectToHostEncrypted() completes the
    // TCP accept before the (doomed) TLS handshake, and SSL errors are reported
    // via postMessage rather than a modal dialog, so nothing blocks.
    void test_tlsUpgradeAcceptedSwitchesPortAndConnects()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, nullptr, nullptr);

        TelnetServerStub secureStub;
        secureStub.start(mLocalhost, 0);
        const quint16 securePort = secureStub.serverPort();

        // Advertise the second stub's port as the secure port.
        QByteArray advertise = msspTlsPayload(QByteArray::number(securePort));
        advertise.reserve(advertise.size() + 16);
        host->mTelnet.loopbackTest(advertise);
        QCOMPARE(host->mMSSPTlsPort, static_cast<int>(securePort));

        QSignalSpy acceptSpy(&secureStub, &QTcpServer::newConnection);
        host->mTelnet.slot_tlsUpgradeResponse(true);

        // The port switch and ssl_tsl flag are set synchronously.
        QCOMPARE(host->getPort(), static_cast<int>(securePort));
        QVERIFY2(host->mSslTsl, "Accepting the TLS upgrade did not enable ssl_tsl on the profile.");
        // The encrypted connect reaches the secure stub's TCP accept.
        QVERIFY2(acceptSpy.wait(5s), "Accepting the TLS upgrade did not open a TCP connection to the secure port.");

        // Stop talking to the local stub before it is destroyed at scope exit.
        host->mTelnet.disconnectIt();
#endif
    }

    // F3 (modal stacking): while a TLS-upgrade prompt is pending, a server
    // re-advertising its secure MSSP port must NOT emit a second prompt. The
    // in-flight latch collapses repeats to a single emission until the user
    // answers (which clears it), so a hostile server cannot stack modals.
    void test_secondTlsAdvertisementWhilePendingDoesNotReprompt()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        // Detach the frontend modal handler so nothing pops up; the latch is set
        // by the emit itself, independent of who is listening.
        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, nullptr, nullptr);

        QSignalSpy spy(&host->mTelnet, &cTelnet::signal_promptTlsAvailable);
        QVERIFY(spy.isValid());

        QByteArray advertise = msspTlsPayload("48000");
        advertise.reserve(advertise.size() + 16);
        host->mTelnet.loopbackTest(advertise);
        if (spy.isEmpty()) {
            QVERIFY2(spy.wait(2s), "cTelnet did not emit signal_promptTlsAvailable for the first advertisement.");
        }
        QCOMPARE(spy.count(), 1);

        // Nobody has answered, so the prompt is still in flight: a repeated
        // advertisement (as a hostile server could spam) must be swallowed.
        QByteArray advertiseAgain = msspTlsPayload("48000");
        advertiseAgain.reserve(advertiseAgain.size() + 16);
        host->mTelnet.loopbackTest(advertiseAgain);
        QCOMPARE(spy.count(), 1);
#endif
    }

    // With the dialog delivered via a queued connection the answer can arrive
    // after the connection has dropped. slot_tlsUpgradeResponse() must discard
    // such a stale answer with a warning rather than act on a gone socket or
    // crash.
    void test_tlsUpgradeAnswerAfterDisconnectIsDiscarded()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, nullptr, nullptr);

        // Advertise so the port is recorded and the latch is set, mirroring a
        // real pending prompt.
        QByteArray advertise = msspTlsPayload("48000");
        advertise.reserve(advertise.size() + 16);
        host->mTelnet.loopbackTest(advertise);

        const int originalPort = host->getPort();
        const bool originalSsl = host->mSslTsl;

        // Drop the connection out from under the pending prompt; mpSocket is
        // reset to null once the disconnect completes. disconnectFromHost() can
        // emit disconnected() synchronously (empty write buffer), so check for an
        // already-recorded emission before waiting or wait() would miss it.
        QSignalSpy disconnectedSpy(&host->mTelnet, &cTelnet::signal_disconnected);
        host->mTelnet.disconnectIt();
        if (disconnectedSpy.isEmpty()) {
            QVERIFY2(disconnectedSpy.wait(5s), "The connection did not drop.");
        }

        // The late answer must be discarded with a warning and leave the profile
        // untouched (no port switch, no ssl_tsl flip, no crash).
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("slot_tlsUpgradeResponse.*discarding the user's answer"));
        host->mTelnet.slot_tlsUpgradeResponse(true);

        QCOMPARE(host->getPort(), originalPort);
        QCOMPARE(host->mSslTsl, originalSsl);
#endif
    }

    // A received telnet BELL (0x07) must emit signal_bell() exactly once per
    // bell byte so the frontend can flash/beep without re-alerting on redraws.
    void test_bellEmitsSignalPerBell()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        QSignalSpy bellSpy(&host->mTelnet, &cTelnet::signal_bell);

        QByteArray oneBell("\a");
        oneBell.reserve(oneBell.size() + 16);
        host->mTelnet.loopbackTest(oneBell);
        QCOMPARE(bellSpy.count(), 1);

        QByteArray twoBells("\a\a");
        twoBells.reserve(twoBells.size() + 16);
        host->mTelnet.loopbackTest(twoBells);
        QCOMPARE(bellSpy.count(), 3);
    }

    // I2: a second package-download prompt must replace (not stack on top of)
    // the first dialog, and cancelling with no active download is a no-op.
    void test_packageDownloadProgressDialogReplaced()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");

        auto console = host->mpConsole;
        console->showPackageDownloadProgress("Downloading package 1", "Cancel");
        console->showPackageDownloadProgress("Downloading package 2", "Cancel");

        // The first dialog closes with WA_DeleteOnClose, so let its queued
        // deleteLater() run before counting the live dialogs.
        QTest::qWait(50ms);
        QCOMPARE(console->findChildren<QProgressDialog*>().count(), 1);

        // Cancelling when no download is in flight must be a harmless no-op.
        host->mTelnet.slot_cancelPackageDownload();
        QCOMPARE(console->findChildren<QProgressDialog*>().count(), 1);
    }

    // When a second server-initiated GUI download supersedes one still in
    // flight (a reconnect re-sends Client.GUI), swapping the progress dialog
    // must not cancel the freshly started download. The superseded dialog's
    // close() emits canceled(), which used to abort the just-assigned new reply.
    // The test above missed this because it swapped dialogs with no reply live.
    void test_replacingDownloadDialogKeepsNewDownloadAlive()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        auto console = host->mpConsole;

        // A TCP server that accepts connections but never answers keeps the
        // package-download reply in flight (Running, NoError) for the whole
        // test, so an unwanted abort() is the only thing that can finish it.
        QTcpServer hangingServer;
        QVERIFY2(hangingServer.listen(QHostAddress::LocalHost, 0), "Could not start the stand-in download server.");
        const QString url = qsl("http://localhost:%1/game-ui.mpackage").arg(hangingServer.serverPort());

        // First server-initiated download: starts reply #1 and progress dialog #1.
        host->mTelnet.downloadAndInstallGUIPackage(qsl("game-ui"), qsl("game-ui.mpackage"), url);
        QVERIFY2(host->mTelnet.mpPackageDownloadReply, "The first GUI download did not start a network reply.");
        QCOMPARE(console->findChildren<QProgressDialog*>().count(), 1);

        // Second download supersedes the first; reply #2 must take over and stay
        // live rather than being cancelled the instant its dialog replaces #1.
        host->mTelnet.downloadAndInstallGUIPackage(qsl("game-ui"), qsl("game-ui.mpackage"), url);

        QPointer<QNetworkReply> newReply = host->mTelnet.mpPackageDownloadReply;
        QVERIFY2(newReply, "The superseding GUI download left no active network reply.");
        QVERIFY2(!newReply->isFinished(), "The superseding GUI download was cancelled at birth by the dialog swap.");
        QCOMPARE(newReply->error(), QNetworkReply::NoError);

        // Let dialog #1's WA_DeleteOnClose deleteLater() run: exactly one dialog
        // survives the swap, and the new download is still alive.
        QTest::qWait(50ms);
        QCOMPARE(console->findChildren<QProgressDialog*>().count(), 1);
        QVERIFY2(newReply && newReply->error() == QNetworkReply::NoError, "The superseding GUI download did not survive the dialog swap.");

        // The user's Cancel must still abort the live download.
        host->mTelnet.slot_cancelPackageDownload();
        QTest::qWait(50ms);
    }

    // Builds an MSSP subnegotiation advertising a secure TLS port:
    //   IAC SB MSSP  MSSP_VAR "TLS" MSSP_VAL <port>  IAC SE
    QByteArray msspTlsPayload(const QByteArray& port)
    {
        QByteArray data;
        data.append(TN_IAC);
        data.append(TN_SB);
        data.append(OPT_MSSP);
        data.append(MSSP_VAR);
        data.append("TLS");
        data.append(MSSP_VAL);
        data.append(port);
        data.append(TN_IAC);
        data.append(TN_SE);
        return data;
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // Utility function to manually start a profile like a user would do via the
    // GUI
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        QTimer::singleShot(0, qApp, [hostname, address, port]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100ms);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), hostname);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), address);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100ms);
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100ms);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(5s)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2s)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Utility function
    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);

        if (!dir.exists()) {
            qInfo() << "Profile directory does not exist:" << path;
            return;
        }
        dir.removeRecursively();
    }
};

void initializeQRCResourcesForTlsPrompt()
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

#include "TelnetTlsPromptTest.moc"
QTEST_MAIN(TelnetTlsPromptTest)
