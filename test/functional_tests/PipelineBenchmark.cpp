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
 * Report-only performance baseline for the text and trigger pipelines.
 *
 * Established for the libmudlet refactor (issue #9011): extracting Qt Widgets
 * from mudlet_core carries a "no more than 10% throughput loss" gate, which is
 * unenforceable without a committed baseline. This benchmark drives a real
 * Mudlet profile with a fixed, deterministically-generated corpus (mixed plain
 * text, ANSI SGR colour and UTF-8) through the production
 * cTelnet::processSocketData -> TBuffer::translateToPlainText -> console ->
 * TriggerUnit path via cTelnet::loopbackTest(), the same code an online session
 * runs, and measures:
 *   a. text pipeline throughput (no triggers active)   -> lines/sec, MB/s
 *   b. trigger engine throughput (a realistic trigger set active) -> lines/sec, MB/s
 *   c. peak resident set size for the whole process (Linux VmHWM)
 *
 * Results are printed one metric per line as `METRIC <name> <value>` so future
 * runs can be diffed mechanically. The test PASSES unconditionally aside from
 * sanity checks that the pipeline actually filled the console buffer and the
 * trigger engine actually saw the data - it deliberately makes NO timing
 * assertions, because absolute speed varies wildly between machines and CI
 * runners. Compare two runs of this binary on the SAME machine instead.
 *
 * Run with: ctest -R PipelineBenchmark -V
 * or directly: QT_QPA_PLATFORM=offscreen ./PipelineBenchmark
 */

#include <QtTest/QtTest>

#include <algorithm>
#include <cstdio>
#include <limits>
#include <random>

#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTrigger.h"
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

class PipelineBenchmark : public QObject
{
    Q_OBJECT

private:
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Perf-Baseline-Host");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0; // assigned the stub's actual loopback port in init()

    // The shared corpus, generated once. Both phases feed the identical bytes so
    // the text and trigger numbers are directly comparable.
    QByteArray mCorpus;
    int mCorpusLines = 0;
    qint64 mCorpusBytes = 0;
    double mTextBestPassSeconds = 0.0;

    // We report the FASTEST single pass, not the total: the least-disturbed pass
    // isolates the code's intrinsic speed from transient CPU contention (this test
    // often runs on a shared/CI box next to other builds), which is exactly what a
    // before/after perf gate wants to compare. More passes = a better chance one lands
    // in a clean window. Each 25 000-line pass is a multi-second window on its own,
    // long enough to average out sub-second scheduler jitter; the 10 000-line
    // scrollback cap (TConsole mLinesLimit) keeps memory bounded regardless.
    static constexpr int kCorpusLines = 25000;
    static constexpr int kFeedPasses = 6;

    // Deterministic corpus: mixed plain text, ANSI SGR colour and UTF-8, one line
    // per '\n' so the processed-line count is exact. Seeded with a constant so the
    // bytes are byte-for-byte identical on every run and every machine.
    static QByteArray generateCorpus(int lines, int& outLineCount)
    {
        std::mt19937 rng(0xC0FFEEu);
        auto pick = [&rng](int n) {
            return static_cast<int>(rng() % static_cast<unsigned>(n));
        };

        // Building blocks reused across templates so substring/regex triggers have
        // realistic, varied text to match (and mostly miss) against.
        static const char* const rooms[] = {"Village Square", "Dark Forest", "Ancient Tower", "Misty Harbour", "Goblin Warren"};
        static const char* const actors[] = {"Gandalf", "Aragorn", "Legolas", "Gimli", "Frodo"};
        static const char* const foes[] = {"orc", "goblin", "troll", "wraith", "spider"};
        static const char* const items[] = {"a rusty sword", "a wooden shield", "a healing potion", "a silver ring", "a torn map"};

        QByteArray out;
        out.reserve(static_cast<qsizetype>(lines) * 96);
        int count = 0;
        for (int i = 0; i < lines; ++i) {
            switch (pick(10)) {
            case 0:
                out += "You are standing in a dark forest. The trees tower above you.";
                break;
            case 1:
                out += "\x1b[1;31mThe ";
                out += foes[pick(5)];
                out += " hits you for ";
                out += QByteArray::number(pick(40) + 1);
                out += " damage!\x1b[0m";
                break;
            case 2:
                out += "\x1b[32mThe ";
                out += rooms[pick(5)];
                out += "\x1b[0m";
                break;
            case 3:
                out += "\x1b[36m";
                out += actors[pick(5)];
                out += " tells you 'meet me at the tower'\x1b[0m";
                break;
            case 4:
                out += "You gain ";
                out += QByteArray::number(pick(500) + 1);
                out += " experience points.";
                break;
            case 5:
                // UTF-8 flavour: accented Latin, CJK and a symbol
                out += "The caf\xc3\xa9 serves cr\xc3\xa8me br\xc3\xbbl\xc3\xa9"
                       "e. \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e \xe2\x98\xba";
                break;
            case 6:
                out += "\x1b[33mHP: ";
                out += QByteArray::number(pick(100) + 1);
                out += "/100 MP: ";
                out += QByteArray::number(pick(50) + 1);
                out += "/50\x1b[0m";
                break;
            case 7:
                out += "You are carrying: ";
                out += items[pick(5)];
                out += ", ";
                out += items[pick(5)];
                out += ", and ";
                out += QByteArray::number(pick(100));
                out += " gold coins.";
                break;
            case 8:
                out += "\x1b[38;5;208mA glowing ember drifts past the ";
                out += rooms[pick(5)];
                out += ".\x1b[0m";
                break;
            default:
                out += "A gentle breeze carries the scent of pine and distant woodsmoke across the clearing "
                       "as you catch your breath and survey the winding path ahead.";
                break;
            }
            out += "\r\n";
            ++count;
        }
        outLineCount = count;
        return out;
    }

    // Build a realistic mix of ~three dozen always-active triggers covering the
    // substring, Perl-regex (with capture groups), begin-of-line, ANSI-colour and
    // multiline matchers; some patterns intentionally never match so the miss
    // path is costed too. Lua-code matchers are deliberately excluded to keep Lua
    // execution out of the timed path, and prompt triggers need a GA signal a
    // loopback feed cannot produce. Each trigger carries an empty action, so a
    // match runs the full regex + capture-extraction path (the cost we want to
    // measure) but TTrigger::execute() returns before any Lua runs - keeping the
    // run free of buffer pollution and Lua-execution noise.
    int installTriggerSet(Host* host, bool& allOk)
    {
        int n = 0;

        auto addKind = [&](const QStringList& patterns, int kind, bool multiline) {
            QList<int> kinds;
            kinds.reserve(patterns.size());
            for (int i = 0; i < patterns.size(); ++i) {
                kinds << kind;
            }
            auto* pT = new TTrigger(qsl("bench_%1").arg(n), patterns, kinds, multiline, host);
            pT->setIsFolder(false);
            pT->setTemporary(false);
            pT->setConditionLineDelta(5);
            pT->setIsActive(true);
            allOk = pT->registerTrigger() && allOk;
            allOk = pT->setScript(QString()) && allOk;
            allOk = pT->state() && allOk;
            ++n;
        };

        auto addColor = [&](int ansiFg, int ansiBg) {
            auto* pT = new TTrigger(nullptr, host);
            pT->setIsFolder(false);
            pT->setTemporary(false);
            allOk = pT->setupTmpColorTrigger(ansiFg, ansiBg) && allOk;
            pT->setIsActive(true);
            allOk = pT->registerTrigger() && allOk;
            allOk = pT->setScript(QString()) && allOk;
            allOk = pT->state() && allOk;
            pT->setName(qsl("bench_%1").arg(n));
            ++n;
        };

        for (const QString& s :
             {qsl("forest"), qsl("orc"), qsl("gold"), qsl("experience"), qsl("sword"), qsl("tower"), qsl("damage"), qsl("coins"), qsl("café"), qsl("Square"), qsl("dragon"), qsl("teleport")}) {
            addKind({s}, REGEX_SUBSTRING, false);
        }

        for (const QString& r : {qsl("^(\\w+) tells you '(.+)'$"),
                                 qsl("You gain (\\d+) experience"),
                                 qsl("hits you for (\\d+) damage"),
                                 qsl("HP: (\\d+)/(\\d+) MP: (\\d+)/(\\d+)"),
                                 qsl("carrying: (.+)$"),
                                 qsl("(\\d+) gold coins"),
                                 qsl("The (\\w+ \\w+)"),
                                 qsl("^A glowing (\\w+)"),
                                 qsl("whisper from (\\w+):"),
                                 qsl("^\\[(\\d{2}):(\\d{2})\\]"),
                                 qsl("reaches level (\\d+)"),
                                 qsl("(\\w+) arrives from the (\\w+)")}) {
            addKind({r}, REGEX_PERL, false);
        }

        for (const QString& s : {qsl("You are"), qsl("The"), qsl("HP:"), qsl("You gain")}) {
            addKind({s}, REGEX_BEGIN_OF_LINE_SUBSTRING, false);
        }

        addColor(1, TTrigger::scmIgnored);
        addColor(2, TTrigger::scmIgnored);
        addColor(3, TTrigger::scmIgnored);
        addColor(6, TTrigger::scmIgnored);

        addKind({qsl("The (\\w+) hits you"), qsl("damage")}, REGEX_PERL, true);
        addKind({qsl("(\\w+) tells you"), qsl("tower")}, REGEX_PERL, true);

        return n;
    }

    // Returns the fastest pass in seconds. loopbackTest() writes NUL bytes up to
    // two past the data end, so the corpus is over-reserved in initTestCase().
    double feedCorpusBestPass(Host* host, int passes)
    {
        double best = std::numeric_limits<double>::max();
        for (int i = 0; i < passes; ++i) {
            QElapsedTimer timer;
            timer.start();
            host->mTelnet.loopbackTest(mCorpus);
            best = std::min(best, timer.nsecsElapsed() / 1.0e9);
        }
        return best;
    }

    static void emitMetric(const char* name, double value)
    {
        std::printf("METRIC %s %.2f\n", name, value);
        std::fflush(stdout);
    }

    static void emitMetric(const char* name, long long value)
    {
        std::printf("METRIC %s %lld\n", name, value);
        std::fflush(stdout);
    }

    // Peak resident set size of the whole process, in kB, from /proc/self/status
    // (VmHWM is the high-water mark and never decreases). /proc pseudo-files
    // report a size of 0, which makes QFile::atEnd() true immediately and
    // readLine() loops never start - so read it all in one go.
    static long long readPeakRssKb()
    {
#if defined(Q_OS_LINUX)
        QFile status(qsl("/proc/self/status"));
        if (!status.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return -1;
        }
        const QList<QByteArray> lines = status.readAll().split('\n');
        for (const QByteArray& line : lines) {
            if (line.startsWith("VmHWM:")) {
                const QList<QByteArray> parts = line.simplified().split(' ');
                if (parts.size() >= 2) {
                    return parts.at(1).toLongLong();
                }
            }
        }
        return -1;
#else
        return -1;
#endif
    }

private slots:
    void initTestCase()
    {
        initializeQRCResources();
        mCorpus = generateCorpus(kCorpusLines, mCorpusLines);
        mCorpusBytes = mCorpus.size();
        // loopbackTest()/processSocketData() write NUL bytes up to two past the
        // data end, so guarantee slack capacity beyond the logical size.
        mCorpus.reserve(mCorpus.size() + 16);
        qInfo().nospace() << "Corpus: " << mCorpusLines << " lines, " << mCorpusBytes << " bytes";
    }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Bind an ephemeral port (0) so parallel worktree runs never collide, then
        // read the actual port back from the QTcpServer the stub derives from.
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory(mHostname);
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory(mHostname);
        delete mudlet::self();
    }

    // (a) Text pipeline: no triggers active.
    void benchTextPipeline()
    {
        Host* host = startProfile();
        QVERIFY(host);

        const double seconds = feedCorpusBestPass(host, kFeedPasses);
        mTextBestPassSeconds = seconds;
        // A silently-disconnected pipeline would report absurdly good numbers, so
        // prove data flowed: after 150k fed lines the console must sit near its
        // 10 000-line scrollback cap.
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        emitMetric("text_corpus_lines", static_cast<long long>(mCorpusLines));
        emitMetric("text_corpus_bytes", mCorpusBytes);
        emitMetric("text_lines_per_sec", mCorpusLines / seconds);
        emitMetric("text_mb_per_sec", (mCorpusBytes / 1.0e6) / seconds);
        emitMetric("text_best_pass_ms", seconds * 1000.0);
    }

    // (b) Trigger engine: same corpus with a realistic trigger set active.
    void benchTriggerEngine()
    {
        Host* host = startProfile();
        QVERIFY(host);

        bool triggersOk = true;
        const int triggerCount = installTriggerSet(host, triggersOk);
        QVERIFY2(triggerCount > 0, "no triggers were installed");
        QVERIFY2(triggersOk, "a trigger failed to compile, register or take its script");

        const double seconds = feedCorpusBestPass(host, kFeedPasses);
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        // Untimed sentinel: prove TriggerUnit actually consumes what this path
        // feeds - a disconnected trigger engine would otherwise just make the
        // timed numbers look better.
        auto* sentinel = new TTrigger(qsl("bench_sentinel"), {qsl("__bench_sentinel__")}, {REGEX_SUBSTRING}, false, host);
        sentinel->setIsFolder(false);
        sentinel->setTemporary(false);
        sentinel->setIsActive(true);
        QVERIFY(sentinel->registerTrigger());
        QVERIFY(sentinel->setScript(qsl("benchSentinelFired = true")));
        QVERIFY(sentinel->state());
        QByteArray probe{"__bench_sentinel__\r\n"};
        probe.reserve(probe.size() + 16);
        host->mTelnet.loopbackTest(probe);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(benchSentinelFired)")), "sentinel trigger did not fire - the trigger engine is not seeing pipeline data");

        emitMetric("trigger_count", static_cast<long long>(triggerCount));
        emitMetric("trigger_lines_per_sec", mCorpusLines / seconds);
        emitMetric("trigger_mb_per_sec", (mCorpusBytes / 1.0e6) / seconds);
        emitMetric("trigger_best_pass_ms", seconds * 1000.0);
        if (mTextBestPassSeconds > 0.0) {
            // Trigger throughput includes the text-pipeline cost, which dilutes a
            // matcher-only regression ~4x; the subtraction isolates it (both
            // phases feed identical bytes, which is what makes it valid).
            emitMetric("trigger_overhead_ms", (seconds - mTextBestPassSeconds) * 1000.0);
        }
    }

    // (c) Peak memory: VmHWM is process-wide and monotonic, so reading it after the
    // two feed phases captures the true peak for the whole run.
    void benchPeakMemory()
    {
        Host* host = startProfile();
        QVERIFY(host);
        // Feed one pass so the peak still reflects pipeline work when this slot
        // is run on its own.
        feedCorpusBestPass(host, 1);
        emitMetric("peak_rss_kb", readPeakRssKb());
    }

private:
    // Mirrors the profile-creation helper the other functional tests use.
    Host* startProfile()
    {
        const QString port = QString::number(mPort);
        QTimer::singleShot(0, qApp, [this, port]() {
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
            QTest::keyClicks(QApplication::focusWidget(), port);
            QTest::qWait(100);
            QTest::keyClick(QApplication::focusWidget(), Qt::Key_Return);
        });

        QSignalSpy loaded(mudlet::self(), &mudlet::signal_profileLoaded);
        if (!loaded.wait(5000)) {
            qWarning("Profile took too long to load");
            return nullptr;
        }
        Host* host = mudlet::self()->getActiveHost();
        if (!host) {
            qWarning("No active host");
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        if (!connected.wait(3000)) {
            qWarning("Could not connect to the stub");
            return nullptr;
        }
        return host;
    }

    void deleteProfileDirectory(const QString& profileName)
    {
        const QString path = mudlet::getMudletPath(enums::profileHomePath, profileName);
        QDir dir(path);
        if (dir.exists()) {
            dir.removeRecursively();
        }
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

#include "PipelineBenchmark.moc"
QTEST_MAIN(PipelineBenchmark)
