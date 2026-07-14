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

#include "MudletInstanceCoordinator.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
void initializeQRCResourcesForSubnegotiation();

// Exercises recovery from a hostile/broken telnet subnegotiation: an IAC SB
// whose payload runs past the size cap without ever sending IAC SE. The rest of
// that subnegotiation must be dropped (not buffered without bound, and not
// leaked into the displayed stream) until the closing IAC SE, after which
// normal processing resumes.
class TelnetSubnegotiationTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = "Test-Telnet-Subnegotiation";
    const QString mPort = "4002";
    const QString mLocalhost = "localhost";

private slots:
    void initTestCase() { initializeQRCResourcesForSubnegotiation(); }

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

    void test_oversizedSubnegotiationIsDroppedUntilSE()
    {
        startProfile(mHostname, mLocalhost, mPort);
        auto host = mudlet::self()->getActiveHost();
        QVERIFY2(host, "No active host available for the test.");

        // MAX_TELNET_SUBNEGOTIATION_LENGTH in ctelnet.cpp is 1 MiB; send more
        // than that inside the subnegotiation, with no IAC SE, then a marker
        // that (only if recovery is broken) would leak into the display, then
        // the real IAC SE and a line of ordinary text.
        constexpr int overCapPadding = 1024 * 1024 + 1024;
        QByteArray data;
        data.append(TN_IAC);
        data.append(TN_SB);
        data.append(static_cast<char>(0x2d)); // an unused option; its value is irrelevant here
        data.append(QByteArray(overCapPadding, 'A'));
        data.append("SUBNEG_LEAK_MARKER");
        data.append(TN_IAC);
        data.append(TN_SE);
        data.append("SUBNEG_RECOVERED\r\n");
        // processSocketData() writes a NUL at in_buffer[size + 1], so give the
        // backing buffer a little slack before handing it its data pointer.
        data.reserve(data.size() + 16);

        host->mTelnet.loopbackTest(data);

        QVERIFY2(waitForBufferToContain("SUBNEG_RECOVERED"), "Ordinary text after an oversized subnegotiation was not displayed - recovery failed.");
        QVERIFY2(!bufferContains("SUBNEG_LEAK_MARKER"), "Subnegotiation payload past the size cap leaked into the display instead of being dropped until IAC SE.");
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
        if (!spy.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        auto host = mudlet::self()->getActiveHost();
        if (!host) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(2000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // True if any line in the main console buffer contains the given substring
    bool bufferContains(const QString& text)
    {
        auto console = mudlet::self()->getActiveHost()->mpConsole;
        for (int i = 0; i <= console->buffer.getLastLineNumber(); ++i) {
            if (console->buffer.line(i).contains(text)) {
                return true;
            }
        }
        return false;
    }

    // Polls the console buffer until the expected substring appears, with a timeout
    bool waitForBufferToContain(const QString& text, int timeoutMs = 5000)
    {
        return QTest::qWaitFor(
                [&]() {
                    return bufferContains(text);
                },
                timeoutMs);
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

void initializeQRCResourcesForSubnegotiation()
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

#include "TelnetSubnegotiationTest.moc"
QTEST_MAIN(TelnetSubnegotiationTest)
