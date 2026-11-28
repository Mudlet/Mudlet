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
 * Compares the libghostty-vt SGR decoder (USE_LIBGHOSTTY) with Mudlet's own:
 * every row feeds one SGR sequence through TBuffer::translateToPlainText()
 * once with each and compares the style of the character that follows it.
 * Rows in sameStyle_data() and random mixes of codes changedStyle_data() does
 * not list must match; rows in changedStyle_data() are the known behaviour
 * changes adopting libghostty brings, and must still differ, so that list
 * stays true. fuzzMixedAgreement() and the bench* slots only report METRICs.
 *
 * Run with: ctest -R LibghosttySgrTest -V
 */

#include <QElapsedTimer>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <ghostty/vt/terminal.h>

#include "Host.h"
#include "MudletApp.h"
#include "MudletInstanceCoordinator.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "TBuffer.h"
#include "TMainConsole.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

#include "GroupedTest.h"

using namespace std::chrono_literals;

namespace {

struct Style
{
    QRgb foreground = 0;
    QRgb background = 0;
    TChar::AttributeFlags flags;
    bool operator==(const Style& other) const { return foreground == other.foreground && background == other.background && flags == other.flags; }
};

QString describe(const Style& style)
{
    return qsl("fg #%1 bg #%2 flags 0x%3")
            .arg(qRgb(qRed(style.foreground), qGreen(style.foreground), qBlue(style.foreground)) & 0xffffff, 6, 16, QLatin1Char('0'))
            .arg(qRgb(qRed(style.background), qGreen(style.background), qBlue(style.background)) & 0xffffff, 6, 16, QLatin1Char('0'))
            .arg(static_cast<int>(style.flags), 0, 16);
}

double secondsSince(const QElapsedTimer& timer)
{
    return static_cast<double>(timer.nsecsElapsed()) / 1.0e9;
}

} // namespace

class LibghosttySgrTest : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    Host* mpHost = nullptr;
    const QString mHostname = qsl("Libghostty-Sgr-Test-Host");
    const QString mLocalhost = qsl("localhost");
    QString mPort;

    TBuffer& buffer() { return mpHost->mainConsoleView()->buffer; }

    // 0;39 rather than 0 alone: SGR 0 leaves the bright foreground used for bold where it was
    Style styleAfter(const QByteArray& sgr, const bool ghostty)
    {
        TBuffer::setGhosttySgrEngine(ghostty);
        std::string bytes = std::string("\x1b[0;39m\x1b[") + sgr.toStdString() + "mX\x1b[0;39m\n";
        TBuffer& console = buffer();
        console.translateToPlainText(bytes, true);
        for (int i = console.getLastLineNumber(); i >= 0; --i) {
            if (console.line(i) == qsl("X")) {
                const TChar& ch = console.buffer.at(i).at(0);
                return {ch.foregroundRgba(), ch.backgroundRgba(), ch.allDisplayAttributes()};
            }
        }
        return {};
    }

    // knownChange: whether it used a code changedStyle_data() lists; strict leaves those codes out
    static QByteArray randomSgr(QRandomGenerator& random, bool& knownChange, const bool strict)
    {
        static const QByteArrayList changed{"6", "21", "58;5;196", "38;5", "38;3;1;2;3", "38;2;300;20;30"};
        static const QByteArrayList tokens{"0",
                                           "1",
                                           "2",
                                           "3",
                                           "4",
                                           "5",
                                           "6",
                                           "7",
                                           "8",
                                           "9",
                                           "21",
                                           "22",
                                           "23",
                                           "24",
                                           "25",
                                           "27",
                                           "28",
                                           "29",
                                           "31",
                                           "34",
                                           "37",
                                           "39",
                                           "41",
                                           "47",
                                           "49",
                                           "53",
                                           "55",
                                           "92",
                                           "96",
                                           "101",
                                           "107",
                                           "11",
                                           "19",
                                           "38;5;9",
                                           "38;5;200",
                                           "48;5;12",
                                           "48;5;240",
                                           "38;2;10;20;30",
                                           "48;2;200;100;0",
                                           "38:2::10:20:30",
                                           "38:5:33",
                                           "4:3",
                                           "4:0",
                                           "4:5",
                                           "3:1",
                                           "58;5;196",
                                           "58:2::1:2:3",
                                           "59",
                                           "38;2;300;20;30",
                                           "38;5",
                                           "38;3;1;2;3",
                                           "",
                                           "26",
                                           "99999"};
        QByteArray sequence;
        knownChange = false;
        const int count = 1 + random.bounded(5);
        for (int i = 0; i < count; ++i) {
            if (i) {
                sequence += ';';
            }
            QByteArray token = tokens.at(random.bounded(tokens.size()));
            while (strict && changed.contains(token)) {
                token = tokens.at(random.bounded(tokens.size()));
            }
            knownChange |= changed.contains(token);
            sequence += token;
        }
        return sequence;
    }

    // Lines a game sends: prompts, combat and room text in 16 and 256 colours
    static std::string sgrHeavyCorpus(const int lines)
    {
        std::string corpus;
        corpus.reserve(static_cast<size_t>(lines) * 90);
        for (int i = 0; i < lines; ++i) {
            switch (i % 5) {
            case 0:
                corpus += "\x1b[33mHP: \x1b[1;32m" + std::to_string(500 + i % 300) + "\x1b[0;33m/800 SP: \x1b[1;34m" + std::to_string(i % 200) + "\x1b[0m>\r\n";
                break;
            case 1:
                corpus += "\x1b[1;31mThe orc hits you \x1b[0;31mhard\x1b[0m for \x1b[1m" + std::to_string(i % 97) + "\x1b[0m damage!\r\n";
                break;
            case 2:
                corpus += "\x1b[38;5;208mA glowing ember drifts past the \x1b[38;5;" + std::to_string(16 + i % 200) + "mfountain\x1b[0m.\r\n";
                break;
            case 3:
                corpus += "\x1b[38;2;" + std::to_string(i % 256) + ";120;200mTruecolour sky\x1b[48;2;10;10;" + std::to_string(i % 256) + "m over the plain\x1b[0m\r\n";
                break;
            default:
                corpus += "\x1b[36mObvious exits: \x1b[1;37mnorth\x1b[0;36m, \x1b[1;37msouth\x1b[0;36m, \x1b[4;37meast\x1b[0m\r\n";
                break;
            }
        }
        return corpus;
    }

    double bestPipelinePass(const std::string& corpus, const bool ghostty, const int passes)
    {
        TBuffer::setGhosttySgrEngine(ghostty);
        double best = std::numeric_limits<double>::max();
        for (int pass = 0; pass < passes; ++pass) {
            buffer().clear();
            std::string copy = corpus;
            QElapsedTimer timer;
            timer.start();
            buffer().translateToPlainText(copy, true);
            best = std::min(best, secondsSince(timer));
        }
        return best;
    }

    static double bestGridPass(const std::string& corpus, const int passes)
    {
        double best = std::numeric_limits<double>::max();
        for (int pass = 0; pass < passes; ++pass) {
            GhosttyTerminal terminal = nullptr;
            if (ghostty_terminal_new(nullptr, &terminal, 100, 40) != GHOSTTY_SUCCESS) {
                return -1.0;
            }
            const size_t scrollback = 10000;
            ghostty_terminal_set(terminal, GHOSTTY_TERMINAL_OPT_SCROLLBACK_MAX_LINES, &scrollback);
            QElapsedTimer timer;
            timer.start();
            ghostty_terminal_vt_write(terminal, reinterpret_cast<const uint8_t*>(corpus.data()), corpus.size());
            best = std::min(best, secondsSince(timer));
            ghostty_terminal_free(terminal);
        }
        return best;
    }

    static void emitMetric(const char* name, const double value) { std::printf("METRIC %s %.3f\n", name, value); }

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

        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        QVERIFY2(mpServer->isListening(), "TelnetServerStub failed to bind a loopback port");
        mPort = QString::number(mpServer->serverPort());
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        QDir(MudletApp::getMudletPath(enums::profileHomePath, mHostname)).removeRecursively();

        mpHost = TestProfile::create(mHostname, mLocalhost, mPort);
        QVERIFY2(mpHost, "Could not create the test profile");
        QSignalSpy connected(&(mpHost->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(5s), "Could not connect with the host");
        // Feeding the buffer by hand would race with the stub's traffic
        mpHost->mTelnet.disconnectIt();
        QVERIFY(QTest::qWaitFor(
                [this]() {
                    return mpHost->mTelnet.getConnectionState() == QAbstractSocket::UnconnectedState;
                },
                5s));
    }

    void cleanupTestCase()
    {
        TBuffer::setGhosttySgrEngine(true);
        delete mudlet::self();
        delete mpServer;
        mpServer = nullptr;
        mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg);
    }

    void sameStyle_data()
    {
        QTest::addColumn<QByteArray>("sgr");
        const QByteArrayList rows{"",
                                  "0",
                                  "1",
                                  "2",
                                  "3",
                                  "4",
                                  "5",
                                  "7",
                                  "8",
                                  "9",
                                  "22",
                                  "23",
                                  "24",
                                  "25",
                                  "27",
                                  "28",
                                  "29",
                                  "53;55",
                                  "53",
                                  "1;31",
                                  "1;37;44",
                                  "30",
                                  "33",
                                  "37",
                                  "39",
                                  "40",
                                  "47",
                                  "49",
                                  "90",
                                  "97",
                                  "100",
                                  "107",
                                  "1;90",
                                  "38;5;0",
                                  "38;5;9",
                                  "38;5;15",
                                  "38;5;16",
                                  "38;5;231",
                                  "38;5;232",
                                  "38;5;255",
                                  "48;5;3",
                                  "48;5;12",
                                  "48;5;100",
                                  "48;5;250",
                                  "38:5:33",
                                  "48:5:200",
                                  "38;2;10;20;30",
                                  "48;2;200;100;0",
                                  "38:2::10:20:30",
                                  "48:2::1:2:3",
                                  "4:0",
                                  "4:1",
                                  "4:2",
                                  "4:3",
                                  "4:4",
                                  "4:5",
                                  "4:3;24",
                                  "3:0",
                                  "3:1",
                                  "3:2",
                                  "10",
                                  "11",
                                  "15",
                                  "19",
                                  "11;10",
                                  "1;;31",
                                  "31;",
                                  "1;2",
                                  "0;1;4;7;9",
                                  "38;5;9;1",
                                  "1;38;2;1;2;3;4",
                                  "11;23;4",
                                  "99999;19;101",
                                  "0;19;37",
                                  "4:5;38;5;",
                                  "41;;31"};
        for (const QByteArray& row : rows) {
            QTest::newRow(row.isEmpty() ? "(empty)" : row.constData()) << row;
        }
    }

    void sameStyle()
    {
        QFETCH(QByteArray, sgr);
        const Style native = styleAfter(sgr, false);
        const Style ghostty = styleAfter(sgr, true);
        QVERIFY2(native == ghostty, qPrintable(qsl("native %1, libghostty %2").arg(describe(native), describe(ghostty))));
    }

    void changedStyle_data()
    {
        QTest::addColumn<QByteArray>("sgr");
        // ECMA-48 double underline: Mudlet ignored it, libghostty underlines
        QTest::newRow("21 underlines") << QByteArray("21");
        // libghostty has one blink, so rapid blink is drawn as slow blink
        QTest::newRow("6 loses rapid blink") << QByteArray("6");
        // Underline colour: Mudlet read 58 as unknown and then 5 as blink
        QTest::newRow("58;5;196 no longer blinks") << QByteArray("58;5;196");
        QTest::newRow("58;2;1;2;3 no longer italicises") << QByteArray("58;2;1;2;3");
        // A truncated 256-colour code: Mudlet set black, libghostty skips 38 and reads 5 as blink
        QTest::newRow("38;5 truncated") << QByteArray("38;5");
        // CMY(K) colour: Mudlet skipped its arguments, libghostty reads them as codes
        QTest::newRow("38;3 arguments read as codes") << QByteArray("38;3;1;2;3");
        // Colon RGB with no colour space slot: Mudlet read 10 as the colour space, libghostty counts the colons
        QTest::newRow("38:2:r:g:b read as RGB") << QByteArray("38:2:10:20:30");
        // A colour type Mudlet does not draw: Mudlet steps over it, libghostty skips 38 and reads 0 as a full reset
        QTest::newRow("38;0 resets everything") << QByteArray("1;4;38;0");
        // Truncated truecolour: Mudlet fills the missing components with zero, libghostty reads them as codes
        QTest::newRow("38;2;r;g truncated") << QByteArray("38;2;10;20");
        // Out-of-range index: Mudlet leaves the colour, libghostty truncates 300 to index 44
        QTest::newRow("38;5;300 truncates") << QByteArray("31;38;5;300");
        // Out-of-range underline style: Mudlet turns underlining off, libghostty draws a single underline
        QTest::newRow("4:6 underlines") << QByteArray("4:3;4:6");
        // A colon colour type Mudlet does not draw: libghostty reads the 1 after it as bold
        QTest::newRow("38:1 reads as bold") << QByteArray("38:1");
        // Out-of-range RGB: Mudlet clamps to 255, libghostty truncates to the low byte
        QTest::newRow("38;2;300 truncates") << QByteArray("38;2;300;20;30");
    }

    void changedStyle()
    {
        QFETCH(QByteArray, sgr);
        const Style native = styleAfter(sgr, false);
        const Style ghostty = styleAfter(sgr, true);
        QVERIFY2(!(native == ghostty), qPrintable(qsl("expected a change, both gave %1").arg(describe(native))));
        qInfo().noquote() << "CHANGED" << sgr << "native:" << describe(native) << "libghostty:" << describe(ghostty);
    }

    // Random mixes of the codes changedStyle_data() does not list must decode the same, every time
    void fuzzStrictAgreement()
    {
        QRandomGenerator random(20261107);
        constexpr int rounds = 5000;
        QStringList differ;
        for (int i = 0; i < rounds; ++i) {
            bool knownChange = false;
            const QByteArray sgr = randomSgr(random, knownChange, true);
            const Style native = styleAfter(sgr, false);
            const Style ghostty = styleAfter(sgr, true);
            if (!(native == ghostty)) {
                differ << qsl("%1 (native %2, libghostty %3)").arg(QString::fromLatin1(sgr), describe(native), describe(ghostty));
            }
        }
        emitMetric("sgr_fuzz_strict_rounds", rounds);
        emitMetric("sgr_fuzz_strict_differences", differ.size());
        QVERIFY2(differ.isEmpty(), qPrintable(differ.mid(0, 10).join(qsl("\n"))));
    }

    // Report only: how often mixes that may include the listed codes decode the same
    void fuzzMixedAgreement()
    {
        QRandomGenerator random(20261108);
        constexpr int rounds = 5000;
        int same = 0;
        int involvesKnownChange = 0;
        for (int i = 0; i < rounds; ++i) {
            bool knownChange = false;
            const QByteArray sgr = randomSgr(random, knownChange, false);
            if (styleAfter(sgr, false) == styleAfter(sgr, true)) {
                ++same;
            }
            involvesKnownChange += knownChange ? 1 : 0;
        }
        emitMetric("sgr_fuzz_mixed_rounds", rounds);
        emitMetric("sgr_fuzz_mixed_same_percent", 100.0 * same / rounds);
        emitMetric("sgr_fuzz_mixed_with_listed_code_percent", 100.0 * involvesKnownChange / rounds);
    }

    void benchSgrDecode()
    {
        const int lines = qEnvironmentVariableIntValue("MUDLET_BENCH_LINES") > 0 ? qEnvironmentVariableIntValue("MUDLET_BENCH_LINES") : 20000;
        const std::string corpus = sgrHeavyCorpus(lines);
        constexpr int passes = 7;
        // Interleaved so both see the same cache and clock state
        double native = std::numeric_limits<double>::max();
        double ghostty = std::numeric_limits<double>::max();
        for (int round = 0; round < 3; ++round) {
            native = std::min(native, bestPipelinePass(corpus, false, passes));
            ghostty = std::min(ghostty, bestPipelinePass(corpus, true, passes));
        }
        // The same text with no escapes: the gap to it bounds what any SGR decoder can save
        std::string plain;
        plain.reserve(corpus.size());
        for (size_t i = 0; i < corpus.size(); ++i) {
            if (corpus[i] == '\x1b') {
                while (i < corpus.size() && corpus[i] != 'm') {
                    ++i;
                }
                continue;
            }
            plain += corpus[i];
        }
        double plainSeconds = std::numeric_limits<double>::max();
        for (int round = 0; round < 3; ++round) {
            plainSeconds = std::min(plainSeconds, bestPipelinePass(plain, false, passes));
        }
        const double grid = bestGridPass(corpus, passes * 3);
        QVERIFY(grid > 0);
        emitMetric("bench_lines", lines);
        emitMetric("bench_bytes", static_cast<double>(corpus.size()));
        emitMetric("tbuffer_native_ms", native * 1000.0);
        emitMetric("tbuffer_libghostty_sgr_ms", ghostty * 1000.0);
        emitMetric("libghostty_sgr_vs_native_ratio", ghostty / native);
        emitMetric("tbuffer_no_escapes_ms", plainSeconds * 1000.0);
        emitMetric("sgr_share_of_tbuffer_percent", 100.0 * (native - plainSeconds) / native);
        emitMetric("libghostty_terminal_grid_ms", grid * 1000.0);
        emitMetric("tbuffer_native_mb_per_sec", corpus.size() / 1.0e6 / native);
        emitMetric("libghostty_terminal_grid_mb_per_sec", corpus.size() / 1.0e6 / grid);
        buffer().clear();
    }
};

#include "LibghosttySgrTest.moc"
MUDLET_GROUPED_TEST_MAIN(LibghosttySgrTest)
