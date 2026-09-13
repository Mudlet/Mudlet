/***************************************************************************
 *   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    *
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

// cTelnet::decodeOption() names every telnet option Mudlet can meet, and the
// "Telnet Options:" block of the statistics report is its only caller outside
// the DEBUG_TELNET builds - so what a player is shown when they ask what was
// negotiated is exactly this table, and an option constant renumbered on one
// side only is reported under the wrong name with nothing to say so.
//
// The report lists an option that either side announced. The sweep below leans
// on the server half, heAnnouncedState, which processTelnetCommand() sets for
// every option a server announces - including ones Mudlet goes on to refuse - so
// announcing the whole 0-255 range in one pass reaches every arm of the switch,
// the unofficial numbers and the UNKNOWN fallback included.

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <chrono>

#include "MudletInstanceCoordinator.h"
#include "ProfileTestHelper.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"
#include "utils.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

class TelnetOptionsReportTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Test-Telnet-Options-Report");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // setupConfig() consults portable.txt before the XDG logic
    static bool portableMarkerPresent()
    {
        return QFileInfo::exists(qsl("%1/portable.txt").arg(QCoreApplication::applicationDirPath())) || QFileInfo::exists(qsl("%1/.config/mudlet/portable.txt").arg(QDir::homePath()));
    }

    // The names decodeOption() is contracted to produce. Held here rather than
    // read back off the function under test, so a renumbered constant or a
    // mistyped name is a failure instead of a report that agrees with itself.
    static QHash<int, QString> expectedOptionNames()
    {
        return {{0, qsl("BINARY (0)")},
                {1, qsl("ECHO (1)")},
                {2, qsl("RECONNECTION (2)")},
                {3, qsl("SUPPRESS_GO_AHEAD (3)")},
                {4, qsl("APPROX_MSG_SIZE (4)")},
                {5, qsl("STATUS (5)")},
                {6, qsl("TIMING_MARK (6)")},
                {7, qsl("REMOTE_CTRL_TRANS_AND_ECHO (7)")},
                {8, qsl("OUTPUT_L_WIDTH (8)")},
                {9, qsl("OUTPUT_P_SIZE (9)")},
                {10, qsl("OUTPUT_CR_DISPOSITION (10)")},
                {11, qsl("OUTPUT_HTAB_STOPS (11)")},
                {12, qsl("OUTPUT_HTAB_DISPOSITION (12)")},
                {13, qsl("OUTPUT_FF_DISPOSITION (13)")},
                {14, qsl("OUTPUT_VTAB_STOPS (14)")},
                {15, qsl("OUTPUT_VTAB_DISPOSITION (15)")},
                {16, qsl("OUTPUT_LF_DISPOSITION (16)")},
                {17, qsl("EXTENDED_ASCII (17)")},
                {18, qsl("LOGOUT (18)")},
                {19, qsl("BYTE_MACRO (19)")},
                {20, qsl("DATA_ENTRY_TERMINAL (20)")},
                {21, qsl("SUPDUP (21)")},
                {22, qsl("SUPDUP_OUTPUT (22)")},
                {23, qsl("SEND_LOCATION (23)")},
                {24, qsl("TTYPE (24)")},
                {25, qsl("EOR (25)")},
                {26, qsl("TACACS_USER_ID (26)")},
                {27, qsl("OUTPUT_MARKING (27)")},
                {28, qsl("TERMINAL_LOCATION_NUMBER (28)")},
                {29, qsl("TELNET_3270_REGIME (29)")},
                {30, qsl("X3_PAD (30)")},
                {31, qsl("NAWS (31)")},
                {32, qsl("TERMINAL_SPEED (32)")},
                {33, qsl("REMOTE_FLOW_CONTROL (33)")},
                {34, qsl("LINEMODE (34)")},
                {35, qsl("X_DISPLAY_LOCATION (35)")},
                {36, qsl("ENVIRONMENT_OPTION (36)")},
                {37, qsl("AUTHENTICATION_OPTION (37)")},
                {38, qsl("ENCRYPTION_OPTION (38)")},
                {39, qsl("NEW-ENVIRON (39)")},
                {40, qsl("TN3270E (40)")},
                {41, qsl("XAUTH (41)")},
                {42, qsl("CHARSET (42)")},
                {43, qsl("TELNET_REMOTE_SERIAL_PORT (43)")},
                {44, qsl("COM_PORT_CONTROL_OPTION (44)")},
                {45, qsl("TELNET_SUPPRESS_LOCAL_ECHO (45)")},
                {46, qsl("TELNET_START_TLS (46)")},
                {47, qsl("KERMIT (47)")},
                {48, qsl("SEND_URL (48)")},
                {49, qsl("FORWARD_X (49)")},
                {69, qsl("MSDP (69)")},
                {70, qsl("MSSP (70)")},
                {85, qsl("MCCP (85)")},
                {86, qsl("MCCP2 (86)")},
                {87, qsl("MCCP3 (87)")},
                {88, qsl("MCCP4 (88)")},
                {90, qsl("MSP (90)")},
                {91, qsl("MXP (91)")},
                {93, qsl("ZENITH (93)")},
                {102, qsl("AARDWOLF (102)")},
                {138, qsl("TELOPT_PRAGMA_LOGON (138)")},
                {139, qsl("TELOPT_SSPI_LOGON (139)")},
                {140, qsl("TELOPT_PRAGMA_HEARTBEAT (140)")},
                {200, qsl("ATCP (200)")},
                {201, qsl("GMCP (201)")},
                {255, qsl("EXTENDED_OPTIONS_LIST (255)")}};
    }

    // Feeds one IAC <command> <option> as if the server had sent it. Loopback
    // rather than the socket, so the whole sequence is processed before the call
    // returns and no wait is needed for the state it leaves behind.
    void announce(const char command, const int option)
    {
        QByteArray data;
        data.append(TN_IAC).append(command).append(static_cast<char>(static_cast<unsigned char>(option)));
        mpHost->mTelnet.loopbackTest(data);
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());

        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        // The report's "server"/"client" and "enabled"/"disabled" words go
        // through tr(), so without pinning a language the assertions below would
        // hold or not depending on the runner's locale. The option names
        // themselves are deliberately not translated.
        mudlet::self()->setInterfaceLanguage(qsl("en_US"));
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(mpHost, "Could not create the test profile - see the warning above for the step that timed out.");

        QSignalSpy connected(&mpHost->mTelnet, &cTelnet::signal_connected);
        if (connected.isEmpty()) {
            QVERIFY2(connected.wait(15s), "The test profile never connected to the stub server.");
        }
    }

    void cleanupTestCase()
    {
        mpHost = nullptr;
        delete mpServer;
        mpServer = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    // Both sides of the report, and the four state words it can pick, on a
    // connection where only these four options have been negotiated. This runs
    // before the sweep below, which sets every bit there is and so leaves no
    // option whose absence could still be shown.
    void test_reportDistinguishesTheTwoSidesAndTheirStates()
    {
        QVERIFY(mpHost);

        const QString untouched = mpHost->mTelnet.assembleTelnetOptionsReport();
        QCOMPARE(untouched.count(QLatin1Char('\n')), 1);
        QVERIFY2(!untouched.contains(QLatin1String("(24)")), qPrintable(qsl("options were already negotiated before the test announced any: %1").arg(untouched)));

        // Server side, accepted: Mudlet answers a WILL NAWS with DO.
        announce(TN_WILL, OPT_NAWS);
        // Server side, refused: Mudlet operates in line mode, so LINEMODE is
        // announced by the server and turned down.
        announce(TN_WILL, OPT_LINEMODE);
        // Client side, accepted: a DO TERMINAL_TYPE is answered with WILL.
        announce(TN_DO, OPT_TERMINAL_TYPE);
        // Client side, refused: nothing answers a DO SEND_LOCATION but WONT.
        announce(TN_DO, 23);

        const QString report = mpHost->mTelnet.assembleTelnetOptionsReport();
        QCOMPARE(report.count(QLatin1Char('\n')), 4);

        // An option only the server announced has no client half, and one only
        // Mudlet answered has no server half - so the side each line names is
        // as much a part of the report as the option name is.
        QCOMPARE(lineFor(report, qsl("NAWS (31)")), qsl("  NAWS (31): server enabled"));
        QCOMPARE(lineFor(report, qsl("LINEMODE (34)")), qsl("  LINEMODE (34): server disabled"));
        QCOMPARE(lineFor(report, qsl("TTYPE (24)")), qsl("  TTYPE (24): client enabled"));
        QCOMPARE(lineFor(report, qsl("SEND_LOCATION (23)")), qsl("  SEND_LOCATION (23): client disabled"));
    }

    // Every arm of decodeOption(), by announcing the whole option number space:
    // the official names, the unofficial ones, and the UNKNOWN fallback for the
    // numbers with no name. heAnnouncedState is set before any option-specific
    // handling, so an option Mudlet refuses is reported just the same.
    void test_everyAnnouncedOptionIsNamedInTheReport()
    {
        QVERIFY(mpHost);

        QByteArray sweep;
        for (int option = 0; option <= 255; ++option) {
            sweep.append(TN_IAC).append(TN_WILL).append(static_cast<char>(static_cast<unsigned char>(option)));
        }
        mpHost->mTelnet.loopbackTest(sweep);

        const QString report = mpHost->mTelnet.assembleTelnetOptionsReport();
        QCOMPARE(report.count(QLatin1Char('\n')), 256);

        // TTYPE was announced by Mudlet in the test above and by the server here,
        // so its line is the one that carries both halves - the join nothing else
        // in this file reaches.
        QCOMPARE(lineFor(report, qsl("TTYPE (24)")), qsl("  TTYPE (24): server enabled, client enabled"));

        const QHash<int, QString> named = expectedOptionNames();
        for (int option = 0; option <= 255; ++option) {
            // Every name carries its own number, so a plain search cannot match
            // the wrong line - and the fallback has to spell the number out too.
            const QString expected = named.value(option, qsl("UNKNOWN (%1)").arg(option));
            QVERIFY2(report.contains(expected),
                     qPrintable(qsl("option %1 was not reported as \"%2\"; its line reads \"%3\"").arg(QString::number(option), expected, lineFor(report, qsl("(%1)").arg(option)))));
        }
    }

private:
    // The single report line containing 'needle', for failure messages.
    static QString lineFor(const QString& report, const QString& needle)
    {
        const QStringList lines = report.split(QLatin1Char('\n'));
        for (const QString& line : lines) {
            if (line.contains(needle)) {
                return line;
            }
        }
        return QString();
    }
};

#include "TelnetOptionsReportTest.moc"
MUDLET_GROUPED_TEST_MAIN(TelnetOptionsReportTest)
