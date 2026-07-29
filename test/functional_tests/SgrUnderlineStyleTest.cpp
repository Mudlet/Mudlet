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

/*
 * Tests for the SGR colon-form underline sub-parameter decoding (ESC[4:Nm).
 *
 * The sub-parameter values follow the widely-adopted kitty/VTE convention:
 *   4:0 none, 4:1 single, 4:2 double, 4:3 curly, 4:4 dotted, 4:5 dashed.
 * These are decoded in TBuffer::decodeSGR(). This test injects each sequence
 * and asserts the resulting cell carries the expected internal underline
 * attributes - in particular that 4:5 yields a dashed underline rather than
 * clearing the underline entirely.
 *
 * Uses loopbackTest() to inject data directly into the telnet processing
 * pipeline, avoiding per-test TCP connections and profile creation.
 *
 * Run with: ctest -R SgrUnderlineStyleTest -V
 */

#include <QtTest/QtTest>

#include "MudletInstanceCoordinator.h"
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
void initializeQRCResourcesForUnderlineTest();

class SgrUnderlineStyleTest : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = "SGR-Underline-Test-Host";
    QString mPort; // assigned the stub's actual ephemeral port in initTestCase()
    const QString mLocalhost = "localhost";

    // Injects raw telnet data into the processing pipeline via loopback and
    // waits for the buffer to process it.
    void injectData(const QString& message)
    {
        QByteArray data = (message + qsl("\r\n")).toUtf8();
        mpHost->mTelnet.loopbackTest(data);
        QTest::qWait(50);
    }

    // Scans the buffer for the first cell whose grapheme matches marker and
    // returns its TChar, or std::nullopt if none is found.
    std::optional<TChar> findCell(QChar marker)
    {
        TMainConsole* console = mpHost->mpConsole;
        for (int line = 0; line <= console->buffer.getLastLineNumber(); ++line) {
            const QString& text = console->buffer.lineBuffer.at(line);
            for (int col = 0; col < text.length(); ++col) {
                if (text.at(col) == marker) {
                    return console->buffer.buffer.at(line).at(col);
                }
            }
        }
        return std::nullopt;
    }

private slots:
    // Start mudlet and create a profile once for all tests.
    void initTestCase()
    {
        initializeQRCResourcesForUnderlineTest();

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0); // ephemeral OS-assigned port avoids collisions across concurrent test runs
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);

        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();

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

        QSignalSpy spy(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!spy.wait(1000)) {
            QFAIL("Profile took too long to load.");
        }
        mpHost = mudlet::self()->getActiveHost();
        if (!mpHost) {
            QFAIL("No active host available for the test.");
        }

        QSignalSpy spy2(&(mpHost->mTelnet), &cTelnet::signal_connected);
        if (!spy2.wait(500)) {
            QFAIL("Could not connect with the host.");
        }
    }

    // Clear buffer before each test for isolation.
    void init()
    {
        QVERIFY(mpHost);
        QVERIFY(mpHost->mpConsole);
        mpHost->mpConsole->buffer.clear();
    }

    // Data-driven test: each ESC[4:Nm sequence must map to the expected internal
    // underline attributes following the kitty/VTE convention.
    void test_ColonUnderlineStyle_data()
    {
        QTest::addColumn<QString>("sequence");
        QTest::addColumn<bool>("underlined");
        QTest::addColumn<bool>("wavy");
        QTest::addColumn<bool>("dotted");
        QTest::addColumn<bool>("dashed");

        //                                          under  wavy   dotted dashed
        QTest::newRow("4:0 none") << qsl("\x1b[4:0m") << false << false << false << false;
        QTest::newRow("4:1 single") << qsl("\x1b[4:1m") << true << false << false << false;
        // Mudlet has no distinct double-underline style, so 4:2 shows as single.
        QTest::newRow("4:2 double") << qsl("\x1b[4:2m") << true << false << false << false;
        QTest::newRow("4:3 curly") << qsl("\x1b[4:3m") << true << true << false << false;
        QTest::newRow("4:4 dotted") << qsl("\x1b[4:4m") << true << false << true << false;
        QTest::newRow("4:5 dashed") << qsl("\x1b[4:5m") << true << false << false << true;
    }

    void test_ColonUnderlineStyle()
    {
        QFETCH(QString, sequence);
        QFETCH(bool, underlined);
        QFETCH(bool, wavy);
        QFETCH(bool, dotted);
        QFETCH(bool, dashed);

        // Reset the pen with ESC[0m so no prior test's underline state leaks in,
        // then apply the sequence and inspect the marker 'U' cell.
        injectData(qsl("\x1b[0m") + sequence + qsl("U"));

        auto cell = findCell(QLatin1Char('U'));
        QVERIFY2(cell.has_value(), "Marker character 'U' not found in buffer");

        QCOMPARE(cell->isUnderlined(), underlined);
        QCOMPARE(cell->isUnderlineWavy(), wavy);
        QCOMPARE(cell->isUnderlineDotted(), dotted);
        QCOMPARE(cell->isUnderlineDashed(), dashed);
    }

    // Data-driven test: applying a colon style over an existing curly underline
    // must clear the sibling style flags, actually turn the underline off for
    // 4:0, and fall back to no underline for out-of-range values. This guards
    // against the stale-state carry-over class of bug the fix addresses.
    void test_ColonUnderlineStyleTransition_data()
    {
        QTest::addColumn<QString>("sequence");
        QTest::addColumn<bool>("underlined");
        QTest::addColumn<bool>("wavy");
        QTest::addColumn<bool>("dotted");
        QTest::addColumn<bool>("dashed");

        //                                                    under  wavy   dotted dashed
        QTest::newRow("curly then 4:0 clears") << qsl("\x1b[4:0m") << false << false << false << false;
        QTest::newRow("curly then 4:4 dotted") << qsl("\x1b[4:4m") << true << false << true << false;
        QTest::newRow("curly then 4:5 dashed") << qsl("\x1b[4:5m") << true << false << false << true;
        // Out-of-range values hit the default arm and clear the underline.
        QTest::newRow("curly then 4:6 out-of-range") << qsl("\x1b[4:6m") << false << false << false << false;
    }

    void test_ColonUnderlineStyleTransition()
    {
        QFETCH(QString, sequence);
        QFETCH(bool, underlined);
        QFETCH(bool, wavy);
        QFETCH(bool, dotted);
        QFETCH(bool, dashed);

        // Establish a curly underline first, then apply the sequence under test
        // to the same pen so sibling-flag clearing is exercised.
        injectData(qsl("\x1b[0m\x1b[4:3m") + sequence + qsl("U"));

        auto cell = findCell(QLatin1Char('U'));
        QVERIFY2(cell.has_value(), "Marker character 'U' not found in buffer");

        QCOMPARE(cell->isUnderlined(), underlined);
        QCOMPARE(cell->isUnderlineWavy(), wavy);
        QCOMPARE(cell->isUnderlineDotted(), dotted);
        QCOMPARE(cell->isUnderlineDashed(), dashed);
    }

    // The plain numeric ESC[4m (no colon) must remain a single underline - the
    // fix only touches the colon sub-parameter path.
    void test_PlainUnderlineUnchanged()
    {
        injectData(qsl("\x1b[0m\x1b[4mU"));

        auto cell = findCell(QLatin1Char('U'));
        QVERIFY2(cell.has_value(), "Marker character 'U' not found in buffer");

        QVERIFY(cell->isUnderlined());
        QVERIFY(!cell->isUnderlineWavy());
        QVERIFY(!cell->isUnderlineDotted());
        QVERIFY(!cell->isUnderlineDashed());
    }

    void cleanupTestCase()
    {
        delete mpServer;
        mpServer = nullptr;
        mpHost = nullptr;
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
        QDir(path).removeRecursively();
        delete mudlet::self();
    }
};

void initializeQRCResourcesForUnderlineTest()
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

#include "SgrUnderlineStyleTest.moc"
QTEST_MAIN(SgrUnderlineStyleTest)
