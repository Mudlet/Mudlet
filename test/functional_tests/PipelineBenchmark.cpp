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
 * Report-only performance baseline for the text and trigger pipelines, for the
 * libmudlet refactor's "no more than 10% throughput loss" gate (issue #9011).
 *
 * Absolute numbers are meaningless across machines, so nothing is asserted on
 * timing and no baseline is committed: the gate is enforced by comparing an
 * older and a newer build of this binary on the SAME machine with
 * test/compare-perf-baseline.py. The benchmark feeds a fixed, deterministic
 * corpus through the production cTelnet::loopbackTest() path and prints one
 * `METRIC <name> <value>` line per measurement.
 *
 * `text_*`, `trigger_*`, `display_*` and `peak_rss_kb` come from a profile with
 * the default packages suppressed; `defaults_*` from one carrying them.
 *
 * Built with the functional tests but deliberately NOT registered with ctest by
 * default (report-only and slow); run it directly, or configure with
 * -DREGISTER_PERF_BENCHMARK=ON to also get it under ctest:
 *   QT_QPA_PLATFORM=offscreen ./PipelineBenchmark
 *
 * Companion for the live-GUI display/echo path is the Stressinator display
 * package; see docs/libmudlet-perf-baseline.md.
 */

#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <clocale>
#include <cstdio>
#include <limits>
#include <random>

// Whether this binary is AddressSanitizer-instrumented. Emitted as an invariant
// so the compare script refuses an ASan-vs-release comparison (their absolute
// numbers are incomparable). Clang reports it through __has_feature; GCC through
// __SANITIZE_ADDRESS__ (and any Qt __has_feature shim harmlessly returns 0, so
// the GCC path still catches it).
#if defined(__has_feature)
#if __has_feature(address_sanitizer)
#define BENCH_BUILD_ASAN 1
#endif
#endif
#if !defined(BENCH_BUILD_ASAN) && defined(__SANITIZE_ADDRESS__)
#define BENCH_BUILD_ASAN 1
#endif
#ifndef BENCH_BUILD_ASAN
#define BENCH_BUILD_ASAN 0
#endif

#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include "TLuaInterpreter.h"
#include "TMainConsole.h"
#include "TTextEdit.h"
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
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Perf-Baseline-Host");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;

    // Both phases feed these identical bytes, so text and trigger numbers are
    // directly comparable.
    QByteArray mCorpus;
    int mCorpusLines = 0;
    qint64 mCorpusBytes = 0;
    double mTextBestPassSeconds = 0.0;

    // Report the FASTEST pass, not the average: the least-disturbed pass isolates
    // intrinsic speed from transient CPU contention (this often runs on a shared/CI
    // box), which is what a before/after gate wants. More passes raise the chance
    // one lands in a clean window, at the cost of buffer memory: the console's
    // line limit sits far above what this corpus produces, so no pass is trimmed
    // away and every pass adds to the buffer.
    static constexpr int kCorpusLines = 25000;
    static constexpr int kFeedPasses = 6;
    // Bump with any change to generateCorpus(). compare-perf-baseline.py reads
    // this as an invariant, so it refuses a cross-corpus comparison instead of
    // reporting the corpus difference as a code regression.
    static constexpr int kCorpusVersion = 2;
    // What kCorpusVersion 2 generates. Checked at startup, so editing the corpus
    // without bumping the version fails here instead of silently invalidating
    // every comparison against an earlier run - and so a shape accidentally left
    // without a switch case, which the functional tests build too permissively to
    // warn about, cannot quietly skew the mix either.
    static constexpr qint64 kCorpusBytesForVersion = 1467189;

    // A typical main-window size, and enough paints per pass to sit well clear
    // of timer noise.
    static constexpr int kDisplayWindowWidth = 1200;
    static constexpr int kDisplayWindowHeight = 800;
    static constexpr int kDisplayPaints = 200;
    static constexpr int kDisplayPasses = 5;

    // A cramped window and a roomy one, so that benchDisplayTail() can report
    // whether one paint's cost follows the pane's area or the single line that
    // actually changed. Both have to be sizes the main window will really adopt;
    // the pass asserts that the row counts came out far enough apart to compare.
    static constexpr int kDisplayTailSmallWidth = 640;
    static constexpr int kDisplayTailSmallHeight = 400;
    static constexpr int kDisplayTailLargeWidth = 1600;
    static constexpr int kDisplayTailLargeHeight = 1000;
    static constexpr int kDisplayTailPaints = 400;

    enum class LineShape {
        Prompt,
        Damage,
        RoomTitle,
        Tell,
        Experience,
        Inventory,
        Utf8,
        Ember,
        Forest,
        MapArt,
        Ack,
        Afflictions,
        Narrative,
        RoomEvent,
        RoomDescription,
        LongParagraph,
    };

    struct ShapeWeight
    {
        LineShape shape;
        int weight;
    };

    // Mixed to the line-length distribution measured over 25 000 lines of real
    // Achaea logs: mean 55 characters, median 50, p90 82, ~3% of lines at 20
    // characters or fewer and ~2% at 100 or more. Weights are relative to their
    // own sum, so one can be retuned without rebalancing the others - but any
    // change to the mix or to a shape's text moves every absolute metric the
    // benchmark reports, so bump kCorpusVersion with it.
    static constexpr ShapeWeight kLineMix[] = {
            {LineShape::Prompt, 185},
            {LineShape::Damage, 120},
            {LineShape::RoomTitle, 10},
            {LineShape::Tell, 50},
            {LineShape::Experience, 65},
            {LineShape::Inventory, 40},
            {LineShape::Utf8, 25},
            {LineShape::Ember, 35},
            {LineShape::Forest, 35},
            {LineShape::MapArt, 32},
            {LineShape::Ack, 185},
            {LineShape::Afflictions, 75},
            {LineShape::Narrative, 95},
            {LineShape::RoomEvent, 105},
            {LineShape::RoomDescription, 22},
            {LineShape::LongParagraph, 5},
    };

    static LineShape shapeForRoll(int roll)
    {
        LineShape shape = kLineMix[0].shape;
        int cursor = 0;
        for (const ShapeWeight& entry : kLineMix) {
            shape = entry.shape;
            cursor += entry.weight;
            if (roll < cursor) {
                break;
            }
        }
        return shape;
    }

    // Seeded with a constant so the corpus bytes are identical on every run and
    // every machine; one line per '\n' keeps the processed-line count exact.
    static QByteArray generateCorpus(int lines, int& outLineCount)
    {
        std::mt19937 rng(0xC0FFEEu);
        auto pick = [&rng](int n) {
            return static_cast<int>(rng() % static_cast<unsigned>(n));
        };

        // Varied building blocks so substring/regex triggers have realistic text
        // to match (and mostly miss) against.
        static const char* const rooms[] = {"Village Square", "Dark Forest", "Ancient Tower", "Misty Harbour", "Goblin Warren"};
        static const char* const actors[] = {"Gandalf", "Aragorn", "Legolas", "Gimli", "Frodo"};
        static const char* const foes[] = {"orc", "goblin", "troll", "wraith", "spider"};
        static const char* const items[] = {"a rusty sword", "a wooden shield", "a healing potion", "a silver ring", "a torn map"};
        static const char* const ailments[] = {"asthma", "clumsiness", "paresis", "slickness", "anorexia", "haemophilia", "lethargy", "dementia"};
        static const char* const bearings[] = {"north", "southeast", "west", "northwest", "east"};
        static const char* const acks[] = {"You have recovered equilibrium.",
                                           "You may drink another elixir.",
                                           "Your muscles are too numb to do that.",
                                           "You must regain your balance first.",
                                           "Your bleeding slows to a trickle.",
                                           "There is nothing here to pick up.",
                                           "Balance Used: 0.89 seconds"};
        static const char* const tells[] = {"meet me at the tower", "are you coming to the raid tonight", "bring rope and a lantern", "the warren is clear, head north when you can"};
        static const char* const asides[] = {
                "a nervous fear on his face.", "watching the treeline for any movement.", "and vanishes into the crowd without a word.", "shouting something you cannot quite make out."};
        static const char* const events[] = {"sets the corpse of a fallen goblin alight upon a righteous pyre.",
                                             "steps through the gate and is gone before you can call out.",
                                             "drops a torn map underfoot and does not stop to pick it up.",
                                             "hauls a crate of salted fish up from the water steps."};
        static const char* const descriptions[] = {"A wide flagstone plaza opens out here, ringed by market stalls whose awnings snap and billow in the wind.",
                                                   "Shelves of dark timber lean inward overhead, so heavy with ledgers that the passage seems about to close over you.",
                                                   "The road widens into a muddy yard where cart tracks cross and re-cross beneath a swinging lantern."};

        QByteArray out;
        out.reserve(static_cast<qsizetype>(lines) * 72);
        int weightTotal = 0;
        for (const ShapeWeight& entry : kLineMix) {
            weightTotal += entry.weight;
        }
        int count = 0;
        for (int i = 0; i < lines; ++i) {
            switch (shapeForRoll(pick(weightTotal))) {
            case LineShape::Prompt:
                out += "\x1b[33mHP: ";
                out += QByteArray::number(pick(100) + 1);
                out += "/100 MP: ";
                out += QByteArray::number(pick(50) + 1);
                out += "/50 EX: ";
                out += QByteArray::number(pick(900000) + 100000);
                out += " [eb] [s] [Bld:";
                out += QByteArray::number(pick(80));
                out += "] [";
                out += actors[pick(5)];
                out += "]\x1b[0m";
                break;
            case LineShape::Damage:
                out += "\x1b[1;31mThe ";
                out += foes[pick(5)];
                out += " hits you for ";
                out += QByteArray::number(pick(40) + 1);
                out += " damage!\x1b[0m";
                break;
            case LineShape::RoomTitle:
                out += "\x1b[32mThe ";
                out += rooms[pick(5)];
                out += "\x1b[0m";
                break;
            case LineShape::Tell:
                out += "\x1b[36m";
                out += actors[pick(5)];
                out += " tells you '";
                out += tells[pick(4)];
                out += "'\x1b[0m";
                break;
            case LineShape::Experience:
                out += "You gain ";
                out += QByteArray::number(pick(500) + 1);
                out += " experience points.";
                break;
            case LineShape::Inventory:
                out += "You are carrying: ";
                out += items[pick(5)];
                out += ", ";
                out += items[pick(5)];
                out += ", and ";
                out += QByteArray::number(pick(100));
                out += " gold coins.";
                break;
            case LineShape::Utf8:
                out += "The caf\xc3\xa9 serves cr\xc3\xa8me br\xc3\xbbl\xc3\xa9"
                       "e. \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e \xe2\x98\xba";
                break;
            case LineShape::Ember:
                out += "\x1b[38;5;208mA glowing ember drifts past the ";
                out += rooms[pick(5)];
                out += ".\x1b[0m";
                break;
            case LineShape::Forest:
                out += "You are standing in a dark forest. The trees tower above you.";
                break;
            case LineShape::MapArt: {
                // Rows of U+2588 FULL BLOCK, as a game's drawn map produces. The
                // only shape that puts a Block Elements glyph in front of the
                // renderer benchDisplay() times.
                const int indent = pick(6);
                for (int space = 0; space < indent; ++space) {
                    out += ' ';
                }
                const int runs = 1 + pick(2);
                for (int run = 0; run < runs; ++run) {
                    if (run) {
                        out += "  ";
                    }
                    const int width = 1 + pick(8);
                    for (int block = 0; block < width; ++block) {
                        out += "\xe2\x96\x88";
                    }
                }
                break;
            }
            case LineShape::Ack:
                out += acks[pick(7)];
                break;
            case LineShape::Afflictions: {
                out += "You are afflicted with: ";
                const int listed = 2 + pick(5);
                for (int entry = 0; entry < listed; ++entry) {
                    if (entry) {
                        out += ", ";
                    }
                    out += ailments[pick(8)];
                }
                out += ".";
                break;
            }
            case LineShape::Narrative:
                out += actors[pick(5)];
                out += " turns to the ";
                out += bearings[pick(5)];
                out += ", ";
                out += asides[pick(4)];
                break;
            case LineShape::RoomEvent:
                out += "(";
                out += rooms[pick(5)];
                out += ") ";
                out += actors[pick(5)];
                out += " ";
                out += events[pick(4)];
                break;
            case LineShape::RoomDescription:
                out += descriptions[pick(3)];
                break;
            case LineShape::LongParagraph:
                // One long single-line paragraph, to force word-wrap passes the
                // short shapes never exercise. Rare, matching the real logs'
                // handful of lines past 300 characters.
                out += "The ancient library stretches away in every direction, its towering shelves crammed with "
                       "mouldering tomes, cracked scrolls and curiosities gathered across a hundred forgotten ages; "
                       "dust drifts through the amber shafts of light that spill from the high stained-glass windows, "
                       "and somewhere far above the slow tick of a great clock marks out the patient centuries.";
                break;
            }
            out += "\r\n";
            ++count;
        }
        outLineCount = count;
        return out;
    }

    // A realistic ~four-dozen always-active trigger mix. Some patterns never
    // match, so the miss path is costed too. Lua-code matchers are excluded and
    // every trigger carries an empty script, so a match runs the full regex +
    // capture path (the cost we want) but TTrigger::execute() returns before any
    // Lua runs - keeping Lua execution and buffer pollution out of the timed path.
    // Prompt triggers are omitted: they need a GA signal a loopback feed cannot send.
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

        // Exact-match patterns cost the whole line on every call, so they are
        // costed at the same count as the substring group.
        for (const QString& s : {qsl("You are hungry."),
                                 qsl("You are thirsty."),
                                 qsl("It is pitch black."),
                                 qsl("The door is closed."),
                                 qsl("You have no keys."),
                                 qsl("Nothing happens."),
                                 qsl("You feel better."),
                                 qsl("Your wounds close."),
                                 qsl("The orc dies."),
                                 qsl("You are hidden."),
                                 qsl("A cool breeze blows."),
                                 qsl("You cannot go that way.")}) {
            addKind({s}, REGEX_EXACT_MATCH, false);
        }

        addColor(1, TTrigger::scmIgnored);
        addColor(2, TTrigger::scmIgnored);
        addColor(3, TTrigger::scmIgnored);
        addColor(6, TTrigger::scmIgnored);

        addKind({qsl("The (\\w+) hits you"), qsl("damage")}, REGEX_PERL, true);
        addKind({qsl("(\\w+) tells you"), qsl("tower")}, REGEX_PERL, true);

        return n;
    }

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

    static void emitMetric(const char* name, qint64 value)
    {
        std::printf("METRIC %s %lld\n", name, value);
        std::fflush(stdout);
    }

    // A pane that drew nothing still yields a pixmap, just a uniform one - so
    // more than one colour is the proof that benchDisplay timed real drawing
    // rather than an empty widget.
    static bool frameHasContent(const QImage& frame)
    {
        if (frame.isNull()) {
            return false;
        }
        const QRgb first = frame.pixel(0, 0);
        for (int y = 0; y < frame.height(); ++y) {
            for (int x = 0; x < frame.width(); ++x) {
                if (frame.pixel(x, y) != first) {
                    return true;
                }
            }
        }
        return false;
    }

    // Process-wide peak RSS in kB (VmHWM never decreases). /proc pseudo-files
    // report a size of 0, so QFile::atEnd() is immediately true and readLine()
    // loops never start - read it all in one go.
    static qint64 readPeakRssKb()
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

        // QApplication's construction adopts the environment locale, which on some
        // machines makes printf("%f") emit comma decimals the compare script cannot
        // parse. Force C numeric formatting for every METRIC line, independent of
        // whatever the environment or Lua startup leaves LC_NUMERIC at.
        std::setlocale(LC_NUMERIC, "C");
        initializeQRCResources();
        mCorpus = generateCorpus(kCorpusLines, mCorpusLines);
        mCorpusBytes = mCorpus.size();
        QCOMPARE(mCorpusLines, kCorpusLines);
        QCOMPARE(mCorpusBytes, kCorpusBytesForVersion);
        // Invariants, emitted here so they are present regardless of which bench
        // slots run: the compare script rejects an ASan-vs-release comparison,
        // and a comparison across two different corpora.
        emitMetric("build_asan", static_cast<qint64>(BENCH_BUILD_ASAN));
        emitMetric("corpus_version", static_cast<qint64>(kCorpusVersion));
        qInfo().nospace() << "Corpus: " << mCorpusLines << " lines, " << mCorpusBytes << " bytes";
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        // Ephemeral port (0) so parallel worktree runs never collide; read the
        // actual port back afterwards.
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
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

    void benchTextPipeline()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(noTriggersAreRunningYet(host));

        const double seconds = feedCorpusBestPass(host, kFeedPasses);
        mTextBestPassSeconds = seconds;
        // A silently-disconnected pipeline would report absurdly good numbers, so
        // prove data flowed: the console must hold thousands of lines rather than
        // just a login banner.
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        emitMetric("text_corpus_lines", static_cast<qint64>(mCorpusLines));
        emitMetric("text_corpus_bytes", mCorpusBytes);
        emitMetric("text_lines_per_sec", mCorpusLines / seconds);
        emitMetric("text_mb_per_sec", (mCorpusBytes / 1.0e6) / seconds);
        emitMetric("text_best_pass_ms", seconds * 1000.0);
    }

    // ISO 8859-1 has no lookup table, so every received byte takes the single-byte
    // branch of the decoder - unlike the default encoding, which never enters it.
    // Decoding the UTF-8 corpus as Latin-1 yields mojibake, which is irrelevant:
    // the byte count through that branch is what is being timed.
    void benchLatin1Decode()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(noTriggersAreRunningYet(host));

        const auto result = host->mTelnet.setEncoding("ISO 8859-1", false);
        QVERIFY2(result.first, qPrintable(result.second));

        const double seconds = feedCorpusBestPass(host, kFeedPasses);
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        emitMetric("latin1_lines_per_sec", mCorpusLines / seconds);
        emitMetric("latin1_mb_per_sec", (mCorpusBytes / 1.0e6) / seconds);
        emitMetric("latin1_best_pass_ms", seconds * 1000.0);
    }

    void benchTriggerEngine()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(noTriggersAreRunningYet(host));

        bool triggersOk = true;
        const int triggerCount = installTriggerSet(host, triggersOk);
        QVERIFY2(triggerCount > 0, "no triggers were installed");
        QVERIFY2(triggersOk, "a trigger failed to compile, register or take its script");
        // trigger_overhead_ms subtracts the text pass, so the count reported has
        // to be the count actually running.
        const int rootTriggers = static_cast<int>(host->getTriggerUnit()->getTriggerRootNodeList().size());
        QVERIFY2(rootTriggers == triggerCount,
                 qPrintable(qsl("installed %1 root triggers but %2 are running - something else registered triggers on this profile").arg(triggerCount).arg(rootTriggers)));

        const double seconds = feedCorpusBestPass(host, kFeedPasses);
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        // Untimed sentinel proving TriggerUnit consumes what the loopback path
        // feeds - a disconnected trigger engine would just flatter the timed numbers.
        auto* sentinel = new TTrigger(qsl("bench_sentinel"), {qsl("__bench_sentinel__")}, {REGEX_SUBSTRING}, false, host);
        sentinel->setIsFolder(false);
        sentinel->setTemporary(false);
        sentinel->setIsActive(true);
        QVERIFY(sentinel->registerTrigger());
        QVERIFY(sentinel->setScript(qsl("benchSentinelFired = true")));
        QVERIFY(sentinel->state());
        QByteArray probe{"__bench_sentinel__\r\n"};
        host->mTelnet.loopbackTest(probe);
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(qsl("assert(benchSentinelFired)")), "sentinel trigger did not fire - the trigger engine is not seeing pipeline data");

        emitMetric("trigger_count", static_cast<qint64>(triggerCount));
        emitMetric("trigger_lines_per_sec", mCorpusLines / seconds);
        emitMetric("trigger_mb_per_sec", (mCorpusBytes / 1.0e6) / seconds);
        emitMetric("trigger_best_pass_ms", seconds * 1000.0);
        if (mTextBestPassSeconds > 0.0) {
            // Trigger throughput includes the text-pipeline cost, which dilutes a
            // matcher-only regression ~4x; subtracting isolates it (valid because
            // both phases feed identical bytes).
            emitMetric("trigger_overhead_ms", (seconds - mTextBestPassSeconds) * 1000.0);
        }
    }

    // VmHWM is process-wide and monotonic, so reading it after the feed phases
    // captures the true peak for the whole run.
    void benchPeakMemory()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(noTriggersAreRunningYet(host));
        // Feed one pass so the peak still reflects pipeline work when this slot
        // runs on its own.
        feedCorpusBestPass(host, 1);
        // Skip the metric entirely when the read fails (non-Linux, or /proc
        // unavailable) rather than emitting a bogus -1 the compare script would
        // read as a real value.
        const qint64 peakRssKb = readPeakRssKb();
        if (peakRssKb >= 0) {
            emitMetric("peak_rss_kb", peakRssKb);
        }
    }

    // Must run after benchPeakMemory: VmHWM is process-wide and monotonic, so
    // the bare peak_rss_kb has to be read before any packaged profile exists.
    // defaults_peak_rss_kb is then the high-water mark including this pass, and
    // its excess over peak_rss_kb is what the packages cost.
    void benchDefaultPackages()
    {
        Host* host = startProfile(DefaultPackages::Install);
        QVERIFY(host);
        const int rootTriggers = static_cast<int>(host->getTriggerUnit()->getTriggerRootNodeList().size());
        // A trigger count would not catch a missing starter UI: the other default
        // packages register root folders of their own.
        QVERIFY2(host->mInstalledPackages.contains(qsl("mudlet-base-ui")),
                 "the starter UI is not installed, so this profile is not the one a new user gets and defaults_* "
                 "would describe something else entirely.");

        const double seconds = feedCorpusBestPass(host, kFeedPasses);
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        emitMetric("defaults_root_triggers", static_cast<qint64>(rootTriggers));
        emitMetric("defaults_text_lines_per_sec", mCorpusLines / seconds);
        emitMetric("defaults_text_best_pass_ms", seconds * 1000.0);
        const qint64 peakRssKb = readPeakRssKb();
        if (peakRssKb >= 0) {
            emitMetric("defaults_peak_rss_kb", peakRssKb);
        }
    }

    // Everything above measures how fast text gets INTO the buffer; this measures
    // how fast it gets back OUT onto the screen. render() runs paintEvent
    // synchronously, so the timing is the drawing code rather than a race with
    // Qt's paint coalescing.
    //
    // Declared last on purpose: its render target and the paint path's cached
    // screen pixmap would otherwise land between peak_rss_kb and
    // defaults_peak_rss_kb, whose difference is what the default packages cost.
    void benchDisplay()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(noTriggersAreRunningYet(host));

        // Real coloured, wide-glyph text to draw rather than blank rows.
        host->mTelnet.loopbackTest(mCorpus);
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        // Sized through the main window rather than the pane: TTextEdit takes its
        // row count from its VISIBLE region, so a pane resized on its own stays
        // clipped by its unchanged parents and quietly redraws a fraction of
        // itself at a flattering speed.
        mudlet::self()->resize(kDisplayWindowWidth, kDisplayWindowHeight);

        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);
        QVERIFY2(pane->visibleRegion().boundingRect().height() >= pane->height(),
                 qPrintable(qsl("the display pane is %1px tall but only %2px of it are unclipped, so this would time a partial redraw")
                                    .arg(pane->height())
                                    .arg(pane->visibleRegion().boundingRect().height())));
        const int rows = pane->getScreenHeight();
        QVERIFY2(rows > 1, qPrintable(qsl("the display pane draws %1 rows, which is too few to describe a console").arg(rows)));

        // scrollTo(line) draws the rows ending just ABOVE line, so `rows` is the
        // first argument that fills the pane. Advancing by more than one screenful
        // per paint keeps drawForeground()'s scroll-blit shortcut from serving any
        // part of a frame from the previous one, so every paint is a full redraw.
        const int stride = rows + 1;
        const int span = std::max(1, bufferedLines - rows);

        QPixmap target(pane->size());
        target.fill(Qt::magenta);
        // Rendered at the position the timed loop starts from, so what is proven
        // here is a frame the loop actually draws.
        pane->scrollTo(rows);
        pane->render(&target);
        // Proves text really reaches the pixmap, and keeps first-paint costs -
        // glyph caches, the pane's own screen pixmap - out of the timed passes.
        QVERIFY2(frameHasContent(target.toImage()), "the rendered frame is a single flat colour - nothing was drawn, so the timings below would describe an empty widget");

        // drawForeground() only skips its scroll-blit shortcut while the scroll
        // between two paints exceeds the rows on screen, and imageTopLine() is the
        // very offset it differences to decide that. Prove the stride clears it:
        // otherwise most of each frame is served from the previous one and the
        // timings describe a partial redraw, several times too fast, with every
        // other assertion here still passing.
        const int firstTop = pane->imageTopLine();
        pane->scrollTo(rows + stride);
        const int steppedTop = pane->imageTopLine();
        QVERIFY2(qAbs(steppedTop - firstTop) > rows,
                 qPrintable(qsl("a %1-line stride moves the top line by %2 on a %3-row screen, so consecutive paints would share cached rows").arg(stride).arg(qAbs(steppedTop - firstTop)).arg(rows)));

        double best = std::numeric_limits<double>::max();
        for (int pass = 0; pass < kDisplayPasses; ++pass) {
            QElapsedTimer timer;
            timer.start();
            for (int i = 0; i < kDisplayPaints; ++i) {
                pane->scrollTo(rows + (i * stride) % span);
                pane->render(&target);
            }
            best = std::min(best, timer.nsecsElapsed() / 1.0e9);
        }

        const double paintsPerSec = kDisplayPaints / best;
        emitMetric("display_paints_per_sec", paintsPerSec);
        emitMetric("display_paint_ms", (best / kDisplayPaints) * 1000.0);
        // The size of the drawn area, and so of the workload: both are invariants
        // the compare script refuses to look past, because a font or layout
        // difference between two builds means they did not draw the same thing.
        emitMetric("display_rows_per_paint", static_cast<qint64>(rows));
        emitMetric("display_cols_per_paint", static_cast<qint64>(pane->getColumnCount()));
        // Lines/s the display can sustain, directly comparable to text_lines_per_sec.
        emitMetric("display_lines_per_sec", paintsPerSec * rows);
    }

    // benchDisplay() above measures the worst case, a full redraw every paint. A
    // console following a game does the opposite: one line arrives and the rest
    // of the screen is already drawn, which is what drawForeground()'s scroll
    // shortcut exists for. What this measures is how much per-paint cost SURVIVES
    // that shortcut - and specifically whether it scales with the pane's area
    // rather than with the one line that changed, which is what makes a game feel
    // slower in a maximised window than in a small one.
    void benchDisplayTail()
    {
        Host* host = startProfile();
        QVERIFY(host);
        QVERIFY(noTriggersAreRunningYet(host));

        host->mTelnet.loopbackTest(mCorpus);
        const int bufferedLines = host->mpConsole->buffer.getLastLineNumber();
        QVERIFY2(bufferedLines > 1000, qPrintable(qsl("console buffer only holds %1 lines - the pipeline did not process the corpus").arg(bufferedLines)));

        TTextEdit* pane = host->mpConsole->mUpperPane;
        QVERIFY(pane);

        TailResult small;
        measureTailPaints(pane, bufferedLines, kDisplayTailSmallWidth, kDisplayTailSmallHeight, small);
        QVERIFY2(!QTest::currentTestFailed(), "the small-window pass did not produce a usable measurement");

        TailResult large;
        measureTailPaints(pane, bufferedLines, kDisplayTailLargeWidth, kDisplayTailLargeHeight, large);
        QVERIFY2(!QTest::currentTestFailed(), "the large-window pass did not produce a usable measurement");

        // Without a real size difference between the two passes there is no ratio
        // worth reporting - the window refused to resize, and every number below
        // would read as a flat 1.0 whatever the paint path does.
        QVERIFY2(large.cells > small.cells * 2.0,
                 qPrintable(qsl("the large pane draws %1 cells against the small pane's %2, which is too close to compare - the main window did not take one of the two sizes")
                                    .arg(large.cells)
                                    .arg(small.cells)));

        emitMetric("display_tail_small_paint_ms", small.paintMs);
        emitMetric("display_tail_large_paint_ms", large.paintMs);
        // Cells drawn at each size: invariants, because two builds that drew
        // differently sized screens did not do the same work.
        emitMetric("display_tail_small_cells", static_cast<qint64>(small.cells));
        emitMetric("display_tail_large_cells", static_cast<qint64>(large.cells));
        // The headline pair. area_ratio is how much bigger the surface got;
        // cost_ratio is how much dearer one paint got with it. A paint path that
        // really only redraws the line that changed holds cost_ratio near 1.0
        // while area_ratio climbs.
        emitMetric("display_tail_area_ratio", large.cells / small.cells);
        emitMetric("display_tail_cost_ratio", large.paintMs / small.paintMs);
    }

private:
    enum class DefaultPackages { Skip, Install };

    struct TailResult
    {
        double paintMs = 0.0;
        // Character cells on screen. Proportional to the painted pixel area, and
        // unlike a pixel count it does not move with the platform's font metrics.
        double cells = 0.0;
        int rows = 0;
    };

    // One line of new text per paint at a given window size, which is the shape
    // of a console following a game rather than one being scrolled through.
    // Fills `result` rather than returning it so that the QVERIFY macros - which
    // expand to a bare `return` - can be used here at all; the caller checks
    // QTest::currentTestFailed() afterwards.
    void measureTailPaints(TTextEdit* pane, const int bufferedLines, const int windowWidth, const int windowHeight, TailResult& result)
    {
        // Sized through the main window for the reason benchDisplay() gives: a
        // pane resized on its own stays clipped by its unchanged parents.
        mudlet::self()->resize(windowWidth, windowHeight);
        // Outside every timed region below. It has to run for the resize to reach
        // the pane at all, and it is also the only thing that can fire the
        // scroll-stopped timer, whose slot forces a full redraw.
        qApp->processEvents();

        result.rows = pane->getScreenHeight();
        QVERIFY2(result.rows > 1, qPrintable(qsl("the display pane draws %1 rows, which is too few to describe a console").arg(result.rows)));
        result.cells = static_cast<double>(result.rows) * pane->getColumnCount();

        // Far enough into the buffer that imageTopLine() clears the threshold
        // below which drawForeground() gives up on the scroll shortcut, and with
        // room for every paint of every pass to land on real text.
        const int firstLine = result.rows + 16;
        QVERIFY2(bufferedLines > firstLine + kDisplayTailPaints,
                 qPrintable(qsl("%1 buffered lines cannot feed %2 single-line paints below a %3-row screen").arg(bufferedLines).arg(kDisplayTailPaints).arg(result.rows)));

        QPixmap target(pane->size());
        target.fill(Qt::magenta);

        // Two warm-up paints, not one: the first leaves tail mode and is drawn
        // under the forced full redraw that transition asks for, and only the
        // second establishes the last-rendered offset that the scroll shortcut
        // differences against.
        pane->scrollTo(firstLine);
        pane->render(&target);
        pane->scrollTo(firstLine + 1);
        pane->render(&target);
        QVERIFY2(frameHasContent(target.toImage()), "the rendered frame is a single flat colour - nothing was drawn, so the timings below would describe an empty widget");

        // The shortcut only runs while consecutive paints scroll by less than a
        // screenful, and imageTopLine() is the offset it differences to decide
        // that. Prove a one-line step really moves it by one line: if it did not,
        // this would quietly time full redraws and measure benchDisplay() again.
        const int beforeTop = pane->imageTopLine();
        pane->scrollTo(firstLine + 2);
        const int afterTop = pane->imageTopLine();
        QVERIFY2(afterTop - beforeTop == 1, qPrintable(qsl("a one-line scroll moved the top line by %1, so these paints would not be the incremental ones this measures").arg(afterTop - beforeTop)));
        QVERIFY2(beforeTop > 10, qPrintable(qsl("the top line is %1, close enough to the start of the buffer that drawForeground() forces full redraws").arg(beforeTop)));

        double best = std::numeric_limits<double>::max();
        for (int pass = 0; pass < kDisplayPasses; ++pass) {
            QElapsedTimer timer;
            timer.start();
            for (int i = 0; i < kDisplayTailPaints; ++i) {
                pane->scrollTo(firstLine + i);
                pane->render(&target);
            }
            best = std::min(best, timer.nsecsElapsed() / 1.0e9);
        }
        result.paintMs = (best / kDisplayTailPaints) * 1000.0;
    }

    // Called before the benchmark installs any of its own, so anything running
    // came from elsewhere and would be timed as pipeline cost.
    bool noTriggersAreRunningYet(Host* host)
    {
        const size_t rootTriggers = host->getTriggerUnit()->getTriggerRootNodeList().size();
        if (rootTriggers == 0) {
            return true;
        }
        qWarning("%s",
                 qPrintable(qsl("%1 root triggers are running on a profile that should have none - a package or a "
                                "leftover profile is being measured as pipeline cost")
                                    .arg(rootTriggers)));
        return false;
    }

    // Mirrors the profile-creation helper the other functional tests use.
    Host* startProfile(DefaultPackages defaultPackages = DefaultPackages::Skip)
    {
        mudlet::self()->mSkipDefaultPackageInstall = (defaultPackages == DefaultPackages::Skip);
        const QString port = QString::number(mPort);
        Host* host = TestProfile::create(mHostname, mLocalhost, port);
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
