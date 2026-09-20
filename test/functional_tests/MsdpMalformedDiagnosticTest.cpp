/***************************************************************************
 *   Copyright (C) 2026 by Jay Howard - jay.patrick.howard@gmail.com       *
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

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include "MudletPaths.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TLuaInterpreter.h"
#include "TConsole.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

/*
 * Dropping a variable whose structure markers do not balance is only worth
 * anything for the diagnostic it leaves: from Lua the drop and a failed yajl
 * decode look identical - no variable, no event - so a spec cannot tell them
 * apart, and two in Telnet_spec.lua passed with the drop removed entirely.
 * The message is reachable here, because mEchoLuaErrors routes logError()
 * through postMessage() into the main console buffer.
 */
class MsdpMalformedDiagnosticTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mpHostname = "Test-MsdpDiagnostic";
    QString mpPort;
    const QString mpLocalhost = "localhost";

    static const char VAR = 1;
    static const char VAL = 2;
    static const char TABLE_OPEN = 3;
    static const char TABLE_CLOSE = 4;
    static const char ARRAY_CLOSE = 6;

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
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mpLocalhost, 0);
        mpPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mpHostname);
    }

    void test_theDiagnosticNamesAnUnopenedTableClose()
    {
        Host* host = bootProfile();
        QVERIFY(host);

        QByteArray payload;
        payload += VAR;
        payload += "MSDPOVER";
        payload += VAL;
        payload += TABLE_CLOSE;
        payload += VAR;
        payload += "MSDPNEXT";
        payload += VAL;
        payload += "ok";
        const QString text = feedAndReadDiagnostic(host, payload);
        QVERIFY2(text.contains(qsl("dropped MSDP variable \"MSDPOVER\"")), qPrintable(qsl("no diagnostic named the over-closing variable; console said: %1").arg(text)));
        QVERIFY2(text.contains(qsl("closed a table or array it never opened")), qPrintable(qsl("the reason did not say the game closed something it never opened; console said: %1").arg(text)));
    }

    // The arm no spec reaches: a spec asserting the variable survives passes
    // whether or not this close is flagged, so only the diagnostic covers it.
    void test_theDiagnosticNamesAnUnopenedArrayClose()
    {
        Host* host = bootProfile();
        QVERIFY(host);

        QByteArray payload;
        payload += VAR;
        payload += "MSDPOVERA";
        payload += VAL;
        payload += ARRAY_CLOSE;
        payload += VAR;
        payload += "MSDPNEXTA";
        payload += VAL;
        payload += "ok";
        const QString text = feedAndReadDiagnostic(host, payload);
        QVERIFY2(text.contains(qsl("dropped MSDP variable \"MSDPOVERA\"")), qPrintable(qsl("an unopened array close raised no diagnostic; console said: %1").arg(text)));
    }

    void test_theDiagnosticSaysWhenAStructureWasLeftOpen()
    {
        Host* host = bootProfile();
        QVERIFY(host);

        QByteArray payload;
        payload += VAR;
        payload += "MSDPCUT";
        payload += VAL;
        payload += TABLE_OPEN;
        const QString text = feedAndReadDiagnostic(host, payload);
        QVERIFY2(text.contains(qsl("ended the message with a table or array still open")), qPrintable(qsl("the reason did not say a structure was left open; console said: %1").arg(text)));
    }

    void test_aStrayCloseDoesNotBlameTheNextVariable()
    {
        Host* host = bootProfile();
        QVERIFY(host);

        // the close lands on a variable with no value, so nothing flushes and
        // nothing used to clear the malformed flag - the next variable was
        // dropped and named instead
        QByteArray stray;
        stray += VAR;
        stray += "MSDPKEPT";
        stray += VAL;
        stray += "1";
        stray += VAR;
        stray += "MSDPVOID";
        stray += TABLE_CLOSE;
        stray += VAR;
        stray += "MSDPBLAMED";
        stray += VAL;
        stray += "3";
        const QString text = feedAndReadDiagnostic(host, stray);
        // nothing is dropped at all: the valueless variable never reaches a flush, so
        // no malformed JSON is built and there is nothing to report
        QVERIFY2(!text.contains(qsl("dropped MSDP variable")), qPrintable(qsl("a stray close on a valueless variable dropped something; console said: %1").arg(text)));
        QVERIFY2(!text.contains(qsl("MSDPBLAMED")), qPrintable(qsl("a stray close on an earlier variable was blamed on a well-formed one; console said: %1").arg(text)));
    }

private:
    Host* bootProfile()
    {
        auto host = TestProfile::create(mpHostname, mpLocalhost, mpPort);
        if (!host) {
            QTest::qFail("No active host available for the test.", __FILE__, __LINE__);
            return nullptr;
        }
        QSignalSpy spy(&(host->mTelnet), &cTelnet::signal_connected);
        if (!spy.wait(2000)) {
            QTest::qFail("Could not connect with the host.", __FILE__, __LINE__);
            return nullptr;
        }
        // logError() only reaches a console a test can read when errors are echoed
        host->mEchoLuaErrors = true;
        // wider than any message asserted below, so none of them wrap: a wrapped one
        // arrives as several lineBuffer entries and a phrase can straddle two of them
        host->mpConsole->setWrapAt(1000);
        return host;
    }

    // Everything the main console gained while the payload was parsed.
    QString feedAndReadDiagnostic(Host* host, const QByteArray& payload)
    {
        TBuffer& buffer = host->mpConsole->buffer;
        const int before = buffer.lineBuffer.size();
        host->getLuaInterpreter()->msdp2Lua(payload.constData());
        qApp->processEvents();
        QStringList gained;
        for (int i = before; i < buffer.lineBuffer.size(); ++i) {
            gained << buffer.lineBuffer.at(i);
        }
        return gained.join(QChar('\n'));
    }

    void deleteProfileDirectory(const QString& profileName) { deleteDirectory(MudletPaths::getMudletPath(enums::profileHomePath, profileName)); }

    void deleteDirectory(const QString& path)
    {
        QDir dir(path);
        if (!dir.exists()) {
            return;
        }
        dir.removeRecursively();
    }

private slots:
    void cleanup()
    {
        const QString profilePath = MudletPaths::getMudletPath(enums::profileHomePath, mpHostname);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        deleteDirectory(profilePath);
    }
};

#include "MsdpMalformedDiagnosticTest.moc"
MUDLET_GROUPED_TEST_MAIN(MsdpMalformedDiagnosticTest)
