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
        // test (there is no user to click it). We just want to observe the
        // signal and its payload.
        disconnect(&host->mTelnet, &cTelnet::signal_promptTlsAvailable, mudlet::self(), nullptr);

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

        QVERIFY2(spy.wait(2s) || spy.count() == 1, "cTelnet did not emit signal_promptTlsAvailable when MSSP advertised a TLS port.");
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
