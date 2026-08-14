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
 * The long_* rows pad the argument past std::string's small-string
 * optimisation on purpose. The parser built its QByteArray over a substr()
 * temporary with QByteArray::fromRawData(), so it parsed bytes that had already
 * been freed; only an argument long enough to have been heap allocated makes
 * that read land somewhere the allocator has really taken back, and the
 * sequence then fails to parse at all. AddressSanitizer stays silent on it
 * either way, because QByteArray::toInt() reads those bytes inside Qt rather
 * than inside instrumented Mudlet code.
 *
 * Run with: ctest -R CsiCursorForwardTest -V
 */

#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
#include "TBuffer.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "dlgConnectionProfiles.h"
#include "mudlet.h"

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

class CsiCursorForwardTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("CSI-Cursor-Forward-Test-Host");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

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

private slots:
    void initTestCase()
    {
        initializeQRCResources();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        QTimer::singleShot(0, qApp, [this]() {
            mudlet::self()->startAutoLogin({});
            QTest::qWait(100);
            QTest::mouseClick(mudlet::self()->mpConnectionDialog->new_profile_button, Qt::LeftButton);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mHostname);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mLocalhost);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Tab);
            QTest::qWait(100);
            QTest::keyClicks(QApplication::focusWidget(), mPort);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy loaded(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!loaded.wait(5000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
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
        QDir(mudlet::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();
        delete mudlet::self();
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
        // Leading zeros only pad the argument past the small-string optimisation;
        // they do not change the value parsed out of it
        QTest::newRow("long_5") << QByteArray("0000000000000000005") << 5;
        QTest::newRow("long_12") << QByteArray("000000000000000000000000012") << 12;
    }

    void test_cursorForwardInsertsSpaces()
    {
        QFETCH(QByteArray, argument);
        QFETCH(int, expectedSpaces);

        injectData(QByteArray("AB\x1b[") + argument + QByteArray("CCD"));

        QCOMPARE(injectedLine(), qsl("AB%1CD").arg(QString(expectedSpaces, QChar::Space)));
    }

    // A CUF argument that makes no sense leaves the line untouched. The parser
    // reports it, which is what reads the argument a second time.
    void test_nonsenseCursorForwardIsIgnored()
    {
        injectData(QByteArray("AB\x1b[00000000000000000009999999999999999999CCD"));

        QCOMPARE(injectedLine(), qsl("ABCD"));
    }

    // Every ED variant is incompatible with a scrollback buffer, so all Mudlet
    // does is report the argument - and reporting it is the whole exposure.
    void test_eraseInDisplayIsIgnored_data()
    {
        QTest::addColumn<QByteArray>("argument");

        QTest::newRow("short_0") << QByteArray("0");
        QTest::newRow("short_2") << QByteArray("2");
        QTest::newRow("long_2") << QByteArray("0000000000000000002");
        QTest::newRow("long_out_of_range") << QByteArray("0000000000000000009");
        QTest::newRow("long_nonsense") << QByteArray("00000000000000000099999999999999999999");
    }

    void test_eraseInDisplayIsIgnored()
    {
        QFETCH(QByteArray, argument);

        injectData(QByteArray("AB\x1b[") + argument + QByteArray("JCD"));

        QCOMPARE(injectedLine(), qsl("ABCD"));
    }
};

static void initializeQRCResources()
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

#include "CsiCursorForwardTest.moc"
QTEST_MAIN(CsiCursorForwardTest)
