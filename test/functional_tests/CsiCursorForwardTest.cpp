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

/*
 * Tests the two CSI sequences TBuffer::translateToPlainText() reads a numeric
 * argument out of: CUF (ESC[nC, emulated as n spaces) and ED (ESC[nJ, reported
 * and ignored). Both are live on mud.durismud.com.
 *
 * Both handlers point a QByteArray at the argument's bytes without copying
 * them, so the rows here guard against those bytes being gone by the time the
 * argument is parsed and logged. Keep the long_* arguments long: a short one is
 * inside whatever holds it, so a stale read of it can quietly succeed. Assert on
 * what Mudlet does with the argument rather than on AddressSanitizer, which
 * stays silent here - QByteArray::toInt() reads the bytes inside Qt, which is
 * not instrumented.
 *
 * Run with: ctest -R CsiCursorForwardTest -V
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <QRegularExpression>

#include "ProfileTestHelper.h"
#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

#include "GroupedTest.h"

class CsiCursorForwardTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("CSI-Cursor-Forward-Test-Host");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    // Leading zeros do not change the value parsed out of an argument, but they
    // do carry it past the small-string optimisation of every standard library
    // the project builds against - libstdc++ inlines up to 15 bytes, libc++ up
    // to 22 - so a stale read lands in memory the allocator really has taken back
    static QByteArray padded(const char* digits) { return QByteArray("000000000000000000000000") + digits; }

    // Every branch that rejects a sequence logs the argument it parsed, as
    // "<argument><final byte> received". For the sequences Mudlet does not act
    // on that log is the only visible read of those bytes, so a stale one
    // reports something else and leaves this expectation unfulfilled.
    static void expectTheArgumentIsReadBack(const QByteArray& argument, char finalByte)
    {
        QTest::ignoreMessage(QtDebugMsg, QRegularExpression(QRegularExpression::escape(QString::fromUtf8(argument) + QLatin1Char(finalByte) + qsl(" received"))));
    }

    void injectData(const QByteArray& payload)
    {
        QByteArray data = payload;
        data.append("\r\n");
        mpHost->mTelnet.loopbackTest(data);
        QTest::qWait(50);
    }

    // The buffer's last line is the empty one waiting for the next output, so
    // look for the line the payload actually landed on
    QString injectedLine() const
    {
        TBuffer& buffer = mpHost->mpConsole->buffer;
        for (int line = buffer.getLastLineNumber(); line >= 0; --line) {
            const QString& text = buffer.lineBuffer.at(line);
            if (text.startsWith(qsl("AB"))) {
                return text;
            }
        }
        return QString();
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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            QFAIL("Could not connect with the host.");
        }
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        // Null when initTestCase skipped or failed ahead of mudlet::start(), and
        // getMudletPath() dereferences the instance rather than checking it
        if (mudlet::self()) {
            QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
            delete mudlet::self();
        }
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mpConsole->buffer.clear();
    }

    void test_cursorForwardInsertsSpaces_data()
    {
        QTest::addColumn<QByteArray>("argument");
        QTest::addColumn<int>("expectedSpaces");

        QTest::newRow("short_5") << QByteArray("5") << 5;
        QTest::newRow("short_1") << QByteArray("1") << 1;
        QTest::newRow("long_5") << padded("5") << 5;
        QTest::newRow("long_12") << padded("12") << 12;
    }

    void test_cursorForwardInsertsSpaces()
    {
        QFETCH(QByteArray, argument);
        QFETCH(int, expectedSpaces);

        injectData(QByteArray("AB\x1b[") + argument + QByteArray("CCD"));

        QCOMPARE(injectedLine(), qsl("AB%1CD").arg(QString(expectedSpaces, QChar::Space)));
    }

    // A CUF argument that makes no sense leaves the line alone, so only the
    // diagnostic can tell a good read from a bad one here.
    void test_nonsenseCursorForwardIsIgnored()
    {
        const QByteArray argument = padded("9999999999999999999");

        expectTheArgumentIsReadBack(argument, 'C');
        injectData(QByteArray("AB\x1b[") + argument + QByteArray("CCD"));

        QCOMPARE(injectedLine(), qsl("ABCD"));
    }

    // Mudlet acts on no ED variant, it only logs the argument, so as with the
    // nonsense CUF above the diagnostic is the only thing that can discriminate.
    void test_eraseInDisplayIsIgnored_data()
    {
        QTest::addColumn<QByteArray>("argument");

        QTest::newRow("short_0") << QByteArray("0");
        QTest::newRow("short_2") << QByteArray("2");
        QTest::newRow("long_2") << padded("2");
        QTest::newRow("long_out_of_range") << padded("9");
        QTest::newRow("long_nonsense") << padded("9999999999999999999");
    }

    void test_eraseInDisplayIsIgnored()
    {
        QFETCH(QByteArray, argument);

        expectTheArgumentIsReadBack(argument, 'J');
        injectData(QByteArray("AB\x1b[") + argument + QByteArray("JCD"));

        QCOMPARE(injectedLine(), qsl("ABCD"));
    }
};

#include "CsiCursorForwardTest.moc"
MUDLET_GROUPED_TEST_MAIN(CsiCursorForwardTest)
