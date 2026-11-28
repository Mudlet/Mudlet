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

// What a game is told the client is called. Desktop Mudlet says MUDLET in TTYPE
// and NEW-ENVIRON and Mudlet in MSDP, GMCP and ATCP; an app that embeds Mudlet
// may name itself instead (MudletApp::setClientName()), which then goes to the
// same protocols in the same case. MXP's <VERSION> reply is TMxpVersionTagTest's.
//
// The game's requests arrive by loopback; the answers are real socket writes,
// waited out with a DO TIMING_MARK, which Mudlet always answers.

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "RecordingTelnetServer.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetClientNameTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    RecordingTelnetServer* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-Client-Name");

    struct NamesSent
    {
        QByteArray terminalType;
        QByteArray newEnviron;
        QByteArray msdp;
        QByteArray gmcp;
        // The answer to WILL ATCP, then the one to Auth.Request
        QList<QByteArray> atcp;
    };

    void feed(const QByteArray& data)
    {
        QByteArray copy = data;
        mpHost->mTelnet.loopbackTest(copy);
    }

    void announce(const char command, const char option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(option);
        feed(data);
    }

    static QByteArray subnegotiation(const char option, const QByteArray& payload)
    {
        QByteArray data;
        data.append(TN_IAC).append(TN_SB).append(option).append(payload).append(TN_IAC).append(TN_SE);
        return data;
    }

    QByteArray sentBytes()
    {
        announce(TN_DO, OPT_TIMING_MARK);
        QByteArray marker;
        marker.append(TN_IAC).append(TN_WONT).append(OPT_TIMING_MARK);
        const bool arrived = QTest::qWaitFor(
                [this, &marker]() {
                    return mpServer->received().contains(marker);
                },
                10s);
        if (!arrived) {
            QTest::qFail("the telnet marker never came back, so the capture is worthless", __FILE__, __LINE__);
            return {};
        }
        const QByteArray stream = mpServer->received();
        mpServer->forgetReceived();
        const int markerAt = stream.indexOf(marker);
        return markerAt >= 0 ? stream.left(markerAt) : stream;
    }

    // The value after name and the separator in payload, up to the next of the terminators; empty when name is not
    // there, which the comparisons then show
    static QByteArray valueAfter(const QByteArray& payload, const QByteArray& name, const char separator, const QByteArray& terminators)
    {
        const QByteArray key = name + separator;
        const int at = payload.indexOf(key);
        if (at < 0) {
            return {};
        }
        QByteArray value = payload.mid(at + key.size());
        for (const char terminator : terminators) {
            const int end = value.indexOf(terminator);
            if (end >= 0) {
                value.truncate(end);
            }
        }
        return value;
    }

    // Asks for the client's name every way a game can, and reads each answer off the wire
    NamesSent namesSent()
    {
        mpHost->mVersionInTTYPE = false;
        mpHost->mEnableNEWENVIRON = true;
        mpHost->mEnableMNES = false;
        mpHost->mEnableMSDP = true;
        mpHost->mEnableGMCP = true;

        // A restart of the MTTS cycle, whose first answer is the name
        announce(TN_DONT, OPT_TERMINAL_TYPE);
        announce(TN_DO, OPT_TERMINAL_TYPE);
        feed(subnegotiation(OPT_TERMINAL_TYPE, QByteArray(1, TNSB_SEND)));
        announce(TN_WILL, OPT_NEW_ENVIRON);
        // Outside MNES, CLIENT_NAME is a user variable
        feed(subnegotiation(OPT_NEW_ENVIRON, QByteArray(1, NEW_ENVIRON_SEND) + NEW_ENVIRON_USERVAR + "CLIENT_NAME"));
        announce(TN_WILL, OPT_MSDP);
        announce(TN_WILL, static_cast<char>(OPT_GMCP));
        // ATCP is taken up only while GMCP is off
        mpHost->mEnableGMCP = false;
        announce(TN_WILL, static_cast<char>(OPT_ATCP));
        feed(subnegotiation(static_cast<char>(OPT_ATCP), "Auth.Request"));
        mpHost->mEnableGMCP = true;

        NamesSent names;
        for (const Subnegotiation& reply : subnegotiationsIn(sentBytes())) {
            const QByteArray& payload = reply.payload;
            if (reply.option == static_cast<unsigned char>(OPT_TERMINAL_TYPE) && payload.startsWith(TNSB_IS)) {
                names.terminalType = payload.mid(1);
            } else if (reply.option == static_cast<unsigned char>(OPT_NEW_ENVIRON) && payload.startsWith(NEW_ENVIRON_IS)) {
                names.newEnviron = valueAfter(payload, "CLIENT_NAME", NEW_ENVIRON_VAL, QByteArray(1, NEW_ENVIRON_VAR) + NEW_ENVIRON_USERVAR);
            } else if (reply.option == static_cast<unsigned char>(OPT_MSDP) && payload.contains("CLIENT_NAME")) {
                names.msdp = valueAfter(payload, "CLIENT_NAME", MSDP_VAL, QByteArray(1, MSDP_VAR));
            } else if (reply.option == static_cast<unsigned char>(OPT_GMCP) && payload.startsWith("Core.Hello ")) {
                names.gmcp = QJsonDocument::fromJson(payload.mid(11)).object().value(qsl("client")).toString().toUtf8();
            } else if (reply.option == static_cast<unsigned char>(OPT_ATCP) && payload.startsWith("hello ")) {
                names.atcp.append(valueAfter(payload, "hello", ' ', " \n"));
            }
        }
        return names;
    }

private slots:
    void initTestCase()
    {
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        mpServer = new RecordingTelnetServer(qApp);
        QVERIFY2(mpServer->start(), "RecordingTelnetServer failed to bind a loopback port");

        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, qsl("localhost"), QString::number(mpServer->serverPort()));
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (mpHost->mTelnet.getConnectionState() != QAbstractSocket::ConnectedState) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the recording server.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        if (mudlet::self()) {
            QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        mpServer->forgetReceived();
    }

    void cleanup() { MudletApp::setClientName(QString()); }

    void test_aGameIsToldDesktopMudletsNames()
    {
        const NamesSent names = namesSent();
        QCOMPARE(names.terminalType, QByteArray("MUDLET"));
        QCOMPARE(names.newEnviron, QByteArray("MUDLET"));
        QCOMPARE(names.msdp, QByteArray("Mudlet"));
        QCOMPARE(names.gmcp, QByteArray("Mudlet"));
        QCOMPARE(names.atcp, QList<QByteArray>({"Mudlet", "Mudlet"}));
    }

    void test_anAppsOwnNameGoesToEveryProtocolInDesktopsCase()
    {
        QVERIFY(MudletApp::setClientName(qsl("Mudlet-Test")));
        const NamesSent names = namesSent();
        QCOMPARE(names.terminalType, QByteArray("MUDLET-TEST"));
        QCOMPARE(names.newEnviron, QByteArray("MUDLET-TEST"));
        QCOMPARE(names.msdp, QByteArray("Mudlet-Test"));
        QCOMPARE(names.gmcp, QByteArray("Mudlet-Test"));
        QCOMPARE(names.atcp, QList<QByteArray>({"Mudlet-Test", "Mudlet-Test"}));
    }

    void test_anEmptyNameGivesDesktopMudletsNamesBack()
    {
        QVERIFY(MudletApp::setClientName(qsl("Mudlet-Test")));
        QVERIFY(MudletApp::setClientName(QString()));
        const NamesSent names = namesSent();
        QCOMPARE(names.terminalType, QByteArray("MUDLET"));
        QCOMPARE(names.gmcp, QByteArray("Mudlet"));
    }
};

MUDLET_GROUPED_TEST_MAIN(TelnetClientNameTest)

#include "TelnetClientNameTest.moc"
