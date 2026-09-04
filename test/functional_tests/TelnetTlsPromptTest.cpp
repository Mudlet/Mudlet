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

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <chrono>
#include <memory>

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include <QAbstractButton>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPointer>
#include <QProgressDialog>
#include <QPushButton>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QRegularExpression>

#include "GroupedTest.h"

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
    enum class ProfileDuringPrompt { Kept, DestroyedBeforeAnswering };

    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-Telnet-Tls-Prompt";
    const QString mLocalhost = "localhost";
    QString mPort;
    QTimer* mpPromptAnswerer = nullptr;
    QElapsedTimer mPromptAnswerClock;
    bool mTlsPromptAnswered = false;
    QString mTlsPromptInformativeText;
    QTcpServer* mpPackageServer = nullptr;
    QPointer<QTcpSocket> mpPackageClient;
    QTimer* mpPackageBodyDrip = nullptr;
    bool mPackageRequestAnswered = false;
    qint64 mPackageBodySent = 0;

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Bind an ephemeral OS-assigned port so parallel test runs (e.g. across
        // git worktrees) do not collide on a shared fixed port.
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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
        host->mTelnet.loopbackTest(advertise);
        if (spy.isEmpty()) {
            QVERIFY2(spy.wait(2s), "cTelnet did not emit signal_promptTlsAvailable for the first advertisement.");
        }
        QCOMPARE(spy.count(), 1);

        // Nobody has answered, so the prompt is still in flight: a repeated
        // advertisement (as a hostile server could spam) must be swallowed.
        QByteArray advertiseAgain = msspTlsPayload("48000");
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

    // The same late answer, but with a connection back up by the time it arrives - so the "is the
    // socket still there" check passes and the answer looks current. The advertised port belongs
    // to the connection that has gone, and moving the profile onto it now would be moving it onto
    // a port this game never mentioned.
    void test_tlsUpgradeAnswerAfterReconnectingIsDiscarded()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, nullptr, nullptr);

        QByteArray advertise = msspTlsPayload("48000");
        host->mTelnet.loopbackTest(advertise);
        QCOMPARE(host->mMSSPTlsPort, 48000);

        const int originalPort = host->getPort();
        const bool originalSsl = host->mSslTsl;

        QSignalSpy disconnectedSpy(&host->mTelnet, &cTelnet::signal_disconnected);
        QSignalSpy connectedSpy(&host->mTelnet, &cTelnet::signal_connected);
        host->mTelnet.disconnectIt();
        if (disconnectedSpy.isEmpty()) {
            QVERIFY2(disconnectedSpy.wait(5s), "The connection did not drop.");
        }
        host->mTelnet.reconnect();
        if (connectedSpy.isEmpty()) {
            QVERIFY2(connectedSpy.wait(5s), "The connection did not come back, so the answer below is not the stale-but-connected case.");
        }

        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("slot_tlsUpgradeResponse.*discarding the user's answer"));
        host->mTelnet.slot_tlsUpgradeResponse(true);

        QCOMPARE(host->getPort(), originalPort);
        QCOMPARE(host->mSslTsl, originalSsl);
#endif
    }

    // Every case above proves one half of the TLS offer in isolation, each by
    // putting the test where the frontend belongs. These run it whole: the
    // connect made in mudlet::addConsoleForNewHost() is left in place, so the
    // modal QMessageBox is really raised, really answered, and the answer really
    // travels back through cTelnet::slot_tlsUpgradeResponse(). Sever that
    // connect and there is no box to press, so these are the cases that notice.
    void test_tlsPromptAcceptedThroughTheFrontendDialog()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        // A plain-TCP stub stands in for the secure server: the encrypted
        // connect completes the TCP accept before the (doomed) handshake, and
        // SSL errors are reported by postMessage rather than another modal.
        TelnetServerStub secureStub;
        secureStub.start(mLocalhost, 0);
        const quint16 securePort = secureStub.serverPort();

        armTlsPromptAnswer(QMessageBox::Yes);
        QByteArray advertise = msspTlsPayload(QByteArray::number(securePort));
        host->mTelnet.loopbackTest(advertise);

        QTRY_VERIFY2_WITH_TIMEOUT(mTlsPromptAnswered, "The frontend never put the TLS upgrade question up for the user to answer.", 5000);
        // cTelnet emits the informative text already translated for the frontend
        // to show; a frontend that dropped it would ask about no port at all.
        QVERIFY2(mTlsPromptInformativeText.contains(QString::number(securePort)), qPrintable(qsl("The question the user was asked did not name the secure port: %1").arg(mTlsPromptInformativeText)));
        QTRY_COMPARE_WITH_TIMEOUT(host->getPort(), static_cast<int>(securePort), 5000);
        QVERIFY2(host->mSslTsl, "Answering Yes did not enable ssl_tsl on the profile.");

        // Stop talking to the local stub before it is destroyed at scope exit.
        host->mTelnet.disconnectIt();
#endif
    }

    void test_tlsPromptDeclinedThroughTheFrontendDialog()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        const int originalPort = host->getPort();
        const bool originalSsl = host->mSslTsl;

        // Declining disconnects and reconnects; that has to have settled before
        // the fixture pulls the server out from under it.
        QSignalSpy connectedSpy(&host->mTelnet, &cTelnet::signal_connected);
        armTlsPromptAnswer(QMessageBox::No);
        QByteArray advertise = msspTlsPayload("48000");
        host->mTelnet.loopbackTest(advertise);

        QTRY_VERIFY2_WITH_TIMEOUT(mTlsPromptAnswered, "The frontend never put the TLS upgrade question up for the user to answer.", 5000);
        // Declining is otherwise indistinguishable from nothing happening, so
        // the don't-ask-again flag is what has to carry the assertion.
        QTRY_VERIFY2_WITH_TIMEOUT(!host->mAskTlsAvailable, "Answering No never reached cTelnet: it is still willing to ask again.", 5000);
        QCOMPARE(host->getPort(), originalPort);
        QCOMPARE(host->mSslTsl, originalSsl);
        if (connectedSpy.isEmpty()) {
            QVERIFY2(connectedSpy.wait(5s), "Declining the TLS upgrade did not reconnect to the server.");
        }
#endif
    }

    // The question is handed over queued and then blocks in exec(), so the
    // profile can be torn down while the box is still up - a Lua closeProfile()
    // is enough. The QPointer<Host> the frontend's lambda captures is what has
    // to catch that; without it the answer is delivered to a destroyed Host.
    void test_tlsPromptAnsweredAfterTheProfileWentAwayIsDiscarded()
    {
#if defined(QT_NO_SSL)
        QSKIP("Built without SSL support - the TLS upgrade prompt does not exist.");
#else
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        armTlsPromptAnswer(QMessageBox::Yes, ProfileDuringPrompt::DestroyedBeforeAnswering);
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression("mudlet:.*discarding the user's answer"));
        QByteArray advertise = msspTlsPayload("48000");
        host->mTelnet.loopbackTest(advertise);

        // The flag is set inside the modal loop, so the QTRY cannot see it until
        // exec() has unwound and the frontend's lambda has run to its guard -
        // the warning claimed above is therefore already in by the time this
        // returns.
        QTRY_VERIFY2_WITH_TIMEOUT(mTlsPromptAnswered, "The frontend never put the TLS upgrade question up for the user to answer.", 5000);
        QVERIFY2(!mudlet::self()->getHostManager().getHost(mHostname), "The profile survived the teardown, so this is not the case being tested.");
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
        host->mTelnet.loopbackTest(oneBell);
        QCOMPARE(bellSpy.count(), 1);

        QByteArray twoBells("\a\a");
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

    // The download's progress and its end are two more connects made in
    // mudlet::addConsoleForNewHost(), and nothing asserted either. Cut the
    // progress one and a multi-megabyte GUI download shows a dialog frozen on
    // its placeholder range; cut the finish one and the dialog never goes away.
    void test_downloadProgressAndFinishDriveTheDialog()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        auto console = host->mpConsole;

        const QString url = startPackageServer();
        QVERIFY2(!url.isEmpty(), "Could not start the stand-in package server.");

        host->mTelnet.downloadAndInstallGUIPackage(qsl("game-ui"), qsl("game-ui.mpackage"), url);
        QVERIFY2(host->mTelnet.mpPackageDownloadReply, "The GUI download did not start a network reply.");
        QPointer<QProgressDialog> dialog = console->findChild<QProgressDialog*>();
        QVERIFY2(dialog, "The GUI download did not raise a progress dialog.");

        // showPackageDownloadProgress() opens on a placeholder range, so a
        // maximum matching the announced Content-Length can only have come from
        // the download's own progress reaching the dialog. The dialog carries
        // WA_DeleteOnClose and a failed download would take it away mid-wait, so
        // the condition has to stay safe for a null one.
        QTRY_VERIFY2_WITH_TIMEOUT(!dialog || dialog->maximum() == static_cast<int>(csmAnnouncedPackageLength), "The download's progress never reached the frontend's progress dialog.", 5000);
        QVERIFY2(dialog, "The progress dialog went away before the download's progress reached it.");
        QVERIFY2(dialog->value() > 0, "The download's progress did not move the dialog's value.");

        // Aborting is the quickest way to finish a download, and the wire that
        // takes the dialog down afterwards is the one a completed one uses too.
        host->mTelnet.slot_cancelPackageDownload();
        QTRY_COMPARE_WITH_TIMEOUT(console->findChildren<QProgressDialog*>().count(), 0, 5000);
    }

    // The user's Cancel has to reach cTelnet::slot_cancelPackageDownload().
    // QProgressDialog's cancel() slot is what canceled() calls, not what emits
    // it, so driving cancel() would prove nothing; the button's clicked() is
    // what reaches the connection TMainConsole makes. (close() emits it too,
    // which is why showPackageDownloadProgress() detaches before closing.)
    void test_downloadDialogCancelStopsTheDownload()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");
        QVERIFY2(host->mpConsole, "The active host has no main console.");
        auto console = host->mpConsole;

        const QString url = startPackageServer();
        QVERIFY2(!url.isEmpty(), "Could not start the stand-in package server.");

        host->mTelnet.downloadAndInstallGUIPackage(qsl("game-ui"), qsl("game-ui.mpackage"), url);
        QPointer<QNetworkReply> reply = host->mTelnet.mpPackageDownloadReply;
        QVERIFY2(reply, "The GUI download did not start a network reply.");
        // The reply is deleteLater()ed the moment it finishes, so how it ended
        // has to be recorded as it happens rather than read back afterwards.
        QSignalSpy errorSpy(reply.data(), &QNetworkReply::errorOccurred);
        QVERIFY(errorSpy.isValid());
        // Read off the reply rather than off the dialog, so that a severed
        // progress wire fails the case above and only the case above.
        QSignalSpy progressSpy(reply.data(), &QNetworkReply::downloadProgress);
        QVERIFY(progressSpy.isValid());

        QPointer<QProgressDialog> dialog = console->findChild<QProgressDialog*>();
        QVERIFY2(dialog, "The GUI download did not raise a progress dialog.");
        QTRY_VERIFY2_WITH_TIMEOUT(!progressSpy.isEmpty(), "The download never got under way, so there was nothing to cancel.", 5000);
        QVERIFY2(dialog, "The progress dialog went away before the download got under way.");

        auto cancelButton = dialog->findChild<QPushButton*>();
        QVERIFY2(cancelButton, "The download progress dialog has no Cancel button to press.");
        cancelButton->click();

        // The rest of the package arrives right behind the click. With the
        // Cancel wire cut the download simply runs to completion and is saved
        // for installation, which is what the file check below catches.
        finishPackageBody();

        QTRY_VERIFY2_WITH_TIMEOUT(!errorSpy.isEmpty(), "Pressing Cancel did not abort the package download.", 5000);
        QCOMPARE(errorSpy.takeFirst().at(0).value<QNetworkReply::NetworkError>(), QNetworkReply::OperationCanceledError);
        QVERIFY2(!QFileInfo::exists(host->mTelnet.mServerPackage), "The cancelled download was saved for installation anyway.");
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
        delete mpPromptAnswerer;
        mpPromptAnswerer = nullptr;
        delete mpServer;
        mpServer = nullptr;
        delete mpPackageBodyDrip;
        mpPackageBodyDrip = nullptr;
        delete mpPackageServer;
        mpPackageServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // Utility function to manually start a profile like a user would do via the
    // GUI
    void startProfile(const QString& hostname, const QString& address, const QString& port)
    {
        auto host = TestProfile::create(hostname, address, port);
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

private:
    // Deliberately not the placeholder range showPackageDownloadProgress()
    // opens on, so a dialog carrying the announced length cannot be mistaken for
    // one that has never been told anything.
    static constexpr qint64 csmAnnouncedPackageLength = 1000000;
    static constexpr qint64 csmPackageBodyChunk = 4096;
    static constexpr std::chrono::milliseconds csmPackageBodyDripInterval = 30ms;
    // 20 chunks at 30ms is 600ms of dripping against a 100ms throttle, so the
    // dialog is fed several times over even if the first emissions are dropped.
    static constexpr qint64 csmDrippedPackageLength = 20 * csmPackageBodyChunk;
    // How long the answerer waits for a pressable prompt before giving up. It
    // has to give up: the modal loop runs inside the QTRY's own wait, so a box
    // it cannot press would hang the case past the ctest timeout instead of
    // failing it with a message.
    static constexpr std::chrono::milliseconds csmPromptAnswerDeadline = 8s;

    // The prompt's own window. activeModalWidget() is maintained by Qt itself
    // rather than by the platform plugin, so it works offscreen, but the
    // top-level sweep keeps this from depending on that.
    static QMessageBox* visibleMessageBox()
    {
        if (auto* modal = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            return modal;
        }
        const auto topLevels = QApplication::topLevelWidgets();
        for (QWidget* widget : topLevels) {
            if (auto* box = qobject_cast<QMessageBox*>(widget); box && box->isVisible()) {
                return box;
            }
        }
        return nullptr;
    }

    // The frontend delivers the TLS offer queued and then blocks in
    // QMessageBox::exec(), so there is no moment at which the box is known to be
    // up: a one-shot armed after the advertisement would be guessing. A repeating
    // poll armed beforehand keeps looking until the box is there, from inside the
    // modal loop. cleanup() disposes of it, so a case that stops at a failed
    // assertion cannot leave it clicking through the next one's dialogs.
    void armTlsPromptAnswer(const QMessageBox::StandardButton answer, const ProfileDuringPrompt profileHandling = ProfileDuringPrompt::Kept)
    {
        mTlsPromptAnswered = false;
        mTlsPromptInformativeText.clear();
        delete mpPromptAnswerer;
        mpPromptAnswerer = new QTimer(this);
        mpPromptAnswerer->setInterval(20ms);
        mPromptAnswerClock.start();
        connect(mpPromptAnswerer, &QTimer::timeout, this, [this, answer, profileHandling]() {
            QMessageBox* box = visibleMessageBox();
            QAbstractButton* button = box ? box->button(answer) : nullptr;
            if (!button) {
                if (mPromptAnswerClock.durationElapsed() < csmPromptAnswerDeadline) {
                    return;
                }
                // Nothing pressable turned up. Take down whatever modal is
                // holding the loop, so the waiting case can report its own
                // failure rather than being killed for running too long.
                mpPromptAnswerer->stop();
                if (QWidget* modal = QApplication::activeModalWidget()) {
                    modal->close();
                }
                return;
            }
            mpPromptAnswerer->stop();
            mTlsPromptInformativeText = box->informativeText();
            if (profileHandling == ProfileDuringPrompt::DestroyedBeforeAnswering) {
                // deleteHost() drops the last shared pointer to the Host, so
                // ~Host() runs here and now rather than being posted - which is
                // what leaves the frontend's QPointer null when exec() returns.
                // forceClose() first, or the teardown asks whether to save.
                if (Host* pHost = mudlet::self()->getHostManager().getHost(mHostname)) {
                    pHost->forceClose();
                }
                mudlet::self()->getHostManager().deleteHost(mHostname);
            }
            mTlsPromptAnswered = true;
            button->click();
        });
        mpPromptAnswerer->start();
    }

    // A stand-in package host. It announces a Content-Length it does not
    // satisfy, so the reply stays in flight with real progress behind it, and it
    // dribbles the first part of the body out rather than writing it in one go:
    // Qt's HTTP reply throttles downloadProgress to one emission per 100ms and
    // drops the rest, so a single burst is swallowed whole and never reaches the
    // dialog. Returns the URL to download from, empty if it could not listen.
    QString startPackageServer()
    {
        delete mpPackageBodyDrip;
        mpPackageBodyDrip = nullptr;
        mPackageBodySent = 0;
        mPackageRequestAnswered = false;
        delete mpPackageServer;
        mpPackageServer = new QTcpServer(this);
        if (!mpPackageServer->listen(QHostAddress::LocalHost, 0)) {
            return QString();
        }
        connect(mpPackageServer, &QTcpServer::newConnection, this, [this]() {
            QTcpSocket* client = mpPackageServer->nextPendingConnection();
            mpPackageClient = client;
            auto request = std::make_shared<QByteArray>();
            connect(client, &QTcpSocket::readyRead, client, [this, client, request]() {
                request->append(client->readAll());
                if (mPackageRequestAnswered || !request->contains("\r\n\r\n")) {
                    return;
                }
                mPackageRequestAnswered = true;
                client->write(qsl("HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %1\r\n\r\n").arg(csmAnnouncedPackageLength).toLatin1());
                mpPackageBodyDrip = new QTimer(this);
                mpPackageBodyDrip->setInterval(csmPackageBodyDripInterval);
                connect(mpPackageBodyDrip, &QTimer::timeout, this, [this]() {
                    if (mPackageBodySent >= csmDrippedPackageLength) {
                        mpPackageBodyDrip->stop();
                        return;
                    }
                    sendPackageBody(csmPackageBodyChunk);
                });
                mpPackageBodyDrip->start();
                sendPackageBody(csmPackageBodyChunk);
            });
        });
        // The literal address, not localhost: the server binds IPv4 only, and a
        // name that also resolves to ::1 has the client open a second connection.
        return qsl("http://127.0.0.1:%1/game-ui.mpackage").arg(mpPackageServer->serverPort());
    }

    void sendPackageBody(const qint64 byteCount)
    {
        if (!mpPackageClient || mpPackageClient->state() != QAbstractSocket::ConnectedState) {
            return;
        }
        const qint64 sending = std::min(byteCount, csmAnnouncedPackageLength - mPackageBodySent);
        if (sending <= 0) {
            return;
        }
        mPackageBodySent += sending;
        mpPackageClient->write(QByteArray(static_cast<qsizetype>(sending), 'M'));
        mpPackageClient->flush();
    }

    // Delivers everything the server still owes, so a download that was not
    // actually cancelled runs to completion rather than merely hanging.
    void finishPackageBody()
    {
        delete mpPackageBodyDrip;
        mpPackageBodyDrip = nullptr;
        sendPackageBody(csmAnnouncedPackageLength - mPackageBodySent);
    }
};

#include "TelnetTlsPromptTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetTlsPromptTest)
