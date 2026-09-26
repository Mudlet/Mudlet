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
 * Report-only performance baseline for TMap::findPath(), the A* speedwalk
 * search, and for the TMap::initGraph() rebuild that feeds it.
 *
 * Report-only means nothing is asserted on timing: the gate is a before/after
 * comparison of two builds of this binary on the same machine, so a busy
 * machine invalidates a run rather than failing it. The map is supplied rather
 * than generated because the interesting workload is a real pathological one
 * far too big to commit:
 *
 *   MUDLET_BENCH_MAP=/path/to/map.dat QT_QPA_PLATFORM=offscreen ./PathfindBenchmark
 *
 * Set MUDLET_BENCH_AREA to pin a particular area instead of the busiest one.
 *
 * Scenarios walk between rooms of the busiest Z level of the chosen area, at
 * increasing separations, because A* cost scales with how much of the graph the
 * frontier has to sweep - and the whole question is how much of the per-search
 * cost is that sweep and how much is a fixed toll paid on every search
 * regardless of how far apart the two rooms are.
 */

#include <QFileInfo>
#include <QRandomGenerator>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <clocale>
#include <cstdio>
#include <limits>
#include <set>

#include "MudletApp.h"
#include "PortableModeTestHelper.h"
#include "ProfileTestHelper.h"
#include "Host.h"
#include "MudletInstanceCoordinator.h"
#include <boost/range/iterator_range.hpp>

#include "TArea.h"
#include "TAstar.h"
#include "TMap.h"
#include "TRoom.h"
#include "TRoomDB.h"
#include "TelnetServerStub.h"
#include "ctelnet.h"
#include "mudlet.h"

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

// Prototype of the per-vertex arrays A* could keep between searches. A map
// over one of them records each write, so the next search puts the defaults
// back for just those rather than for every room on the map - which is the
// whole of what astar_search() does before it looks at an edge. Repeat writes
// are recorded again, so the touched list counts writes, not distinct rooms.
template <typename Value>
class ScratchMap
{
public:
    typedef std::size_t key_type;
    typedef Value value_type;
    typedef Value& reference;
    typedef boost::read_write_property_map_tag category;

    ScratchMap(std::vector<Value>& store, std::vector<std::size_t>& touched)
    : mpStore(&store)
    , mpTouched(&touched)
    {
    }

    std::vector<Value>* mpStore;
    std::vector<std::size_t>* mpTouched;
};

template <typename Value>
inline Value get(const ScratchMap<Value>& map, std::size_t key)
{
    return (*map.mpStore)[key];
}

template <typename Value>
inline void put(const ScratchMap<Value>& map, std::size_t key, const Value& value)
{
    (*map.mpStore)[key] = value;
    map.mpTouched->push_back(key);
}

extern void qInitResources_mudlet();
extern void qInitResources_qm();
extern void qInitResources_additional_splash_screens();
extern void qInitResources_mudlet_fonts_common();
extern void qInitResources_mudlet_fonts_posix();
static void initializeQRCResources();

class PathfindBenchmark : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir mConfigDir;
    QByteArray mSavedXdg;
    TelnetServerStub* mpServer = nullptr;
    const QString mHostname = qsl("Pathfind-Benchmark-Host");
    const QString mLocalhost = qsl("localhost");
    quint16 mPort = 0;
    QString mMapPath;

    static constexpr int kPasses = 5;

    struct Scenario
    {
        const char* name;
        // Added to BOTH the x and y of the start room, so the target of
        // {"near", 10} is 10 map units away on each axis, not 10 in total.
        int offset;
    };

    // 0 means "the room nearest the far corner". The start is the middle of
    // the level, so that is about a half-diagonal, not its longest walk.
    static constexpr Scenario kScenarios[] = {
            {"adjacent", 1},
            {"near", 10},
            {"mid", 100},
            {"far", 400},
            {"corner", 0},
    };

    static void emitMetric(const char* name, double value) { std::printf("METRIC %s %.3f\n", name, value); }

    static void emitMetric(const char* name, qint64 value) { std::printf("METRIC %s %lld\n", name, value); }

    static void emitMetric(const QString& name, double value) { emitMetric(name.toUtf8().constData(), value); }

    static void emitMetric(const QString& name, qint64 value) { emitMetric(name.toUtf8().constData(), value); }

    static qint64 readPeakRssKb()
    {
#if defined(Q_OS_LINUX)
        QFile status(qsl("/proc/self/status"));
        if (!status.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return 0;
        }
        const QString text = QString::fromLatin1(status.readAll());
        for (const QString& line : text.split(QChar::LineFeed)) {
            if (line.startsWith(qsl("VmHWM:"))) {
                return QStringView{line}.mid(6).trimmed().split(QChar::Space).constFirst().toLongLong();
            }
        }
#endif
        return 0;
    }

private slots:
    void initTestCase()
    {
        mMapPath = qEnvironmentVariable("MUDLET_BENCH_MAP");
        if (mMapPath.isEmpty()) {
            QSKIP("MUDLET_BENCH_MAP is not set - point it at a saved Mudlet map file to run this benchmark");
        }
        if (!QFileInfo::exists(mMapPath)) {
            QFAIL(qPrintable(qsl("MUDLET_BENCH_MAP points at \"%1\", which does not exist").arg(mMapPath)));
        }
        if (portableMarkerPresent()) {
            QSKIP("portable.txt present - it takes precedence over XDG_CONFIG_HOME, so the config dir cannot be redirected");
        }

        QVERIFY(mConfigDir.isValid());
        QVERIFY(QDir().mkpath(qsl("%1/mudlet/profiles").arg(mConfigDir.path())));
        mSavedXdg = qgetenv("XDG_CONFIG_HOME");
        qputenv("XDG_CONFIG_HOME", mConfigDir.path().toUtf8());

        std::setlocale(LC_NUMERIC, "C");
        initializeQRCResources();
        emitMetric("build_asan", static_cast<qint64>(BENCH_BUILD_ASAN));
    }

    void cleanupTestCase() { mSavedXdg.isNull() ? qunsetenv("XDG_CONFIG_HOME") : qputenv("XDG_CONFIG_HOME", mSavedXdg); }

    void init()
    {
        mpServer = new TelnetServerStub(qApp);
        mpServer->start(mLocalhost, 0);
        mPort = mpServer->serverPort();
        mudlet::start();
        mudlet::self()->setupConfig();
        QCOMPARE(MudletApp::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        delete mudlet::self();
        deleteProfileDirectory();
    }

    void benchFindPath()
    {
        gHeuristicMode = qEnvironmentVariableIntValue("MUDLET_HEURISTIC_MODE");
        mudlet::self()->mSkipDefaultPackageInstall = true;
        Host* host = TestProfile::create(mHostname, mLocalhost, QString::number(mPort));
        QVERIFY(host);
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the stub");

        host->showHideOrCreateMapper(false);
        QVERIFY(host->mpMap);
        TMap* pMap = host->mpMap.data();

        QElapsedTimer timer;
        timer.start();
        QVERIFY2(pMap->restore(mMapPath), qPrintable(qsl("could not restore the map at \"%1\"").arg(mMapPath)));
        emitMetric("map_restore_seconds", timer.nsecsElapsed() / 1.0e9);

        emitMetric("map_rooms", static_cast<qint64>(pMap->mpRoomDB->size()));
        emitMetric("map_areas", static_cast<qint64>(pMap->mpRoomDB->getAreaMap().size()));

        const int areaId = chooseArea(pMap);
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        QVERIFY2(pArea, "the chosen area is not in the map");
        const int zLevel = chooseZLevel(pArea);
        const QSet<int> roomsOnLevel = pArea->getRoomsForZ(zLevel);
        QVERIFY2(!roomsOnLevel.isEmpty(), "the chosen Z level holds no rooms");

        emitMetric("bench_area_id", static_cast<qint64>(areaId));
        emitMetric("bench_z_level", static_cast<qint64>(zLevel));
        emitMetric("bench_rooms_on_z_level", static_cast<qint64>(roomsOnLevel.size()));

        // The graph is rebuilt from scratch whenever the map is touched, and
        // the first findPath() after that pays for it, so it is timed on its
        // own rather than being hidden inside the first scenario.
        timer.restart();
        pMap->initGraph();
        emitMetric("init_graph_ms", timer.nsecsElapsed() / 1.0e6);
        emitMetric("graph_vertices", static_cast<qint64>(pMap->roomidToIndex.size()));

        QHash<QPair<int, int>, int> byCoordinate;
        byCoordinate.reserve(roomsOnLevel.size());
        int minX = std::numeric_limits<int>::max();
        int maxX = std::numeric_limits<int>::min();
        int minY = std::numeric_limits<int>::max();
        int maxY = std::numeric_limits<int>::min();
        for (const int roomId : roomsOnLevel) {
            TRoom* pRoom = pMap->mpRoomDB->getRoom(roomId);
            if (!pRoom) {
                continue;
            }
            // Ties break on the lower room id so the picks are identical on
            // every run, which QSet iteration order alone would not give.
            const QPair<int, int> key{pRoom->x(), pRoom->y()};
            const auto it = byCoordinate.constFind(key);
            if (it == byCoordinate.constEnd() || roomId < it.value()) {
                byCoordinate.insert(key, roomId);
            }
            minX = std::min(minX, pRoom->x());
            maxX = std::max(maxX, pRoom->x());
            minY = std::min(minY, pRoom->y());
            maxY = std::max(maxY, pRoom->y());
        }
        emitMetric("bench_span_x", static_cast<qint64>(maxX - minX));
        emitMetric("bench_span_y", static_cast<qint64>(maxY - minY));

        const int centreX = (minX + maxX) / 2;
        const int centreY = (minY + maxY) / 2;
        const int startId = nearestTo(byCoordinate, centreX, centreY, maxX - minX);
        QVERIFY2(startId > 0, "no room found near the middle of the chosen Z level");
        emitMetric("bench_start_room", static_cast<qint64>(startId));

        for (const Scenario& scenario : kScenarios) {
            const int targetId = (scenario.offset == 0) ? nearestTo(byCoordinate, maxX, maxY, maxX - minX) : nearestTo(byCoordinate, centreX + scenario.offset, centreY + scenario.offset, maxX - minX);
            QVERIFY2(targetId > 0, qPrintable(qsl("scenario \"%1\" found no target room").arg(QString::fromUtf8(scenario.name))));
            if (targetId == startId) {
                continue;
            }

            // One untimed search first, so the timed passes cannot be the run
            // that faults in whatever the first search touches, and so a failed
            // search is caught here rather than silently timed as a success.
            QVERIFY2(pMap->findPath(startId, targetId),
                     qPrintable(qsl("scenario \"%1\" found no path from %2 to %3, so its timings would describe a failed search")
                                        .arg(QString::fromUtf8(scenario.name), QString::number(startId), QString::number(targetId))));
            const int steps = pMap->mPathList.size();

            double best = std::numeric_limits<double>::max();
            for (int pass = 0; pass < kPasses; ++pass) {
                timer.restart();
                const bool found = pMap->findPath(startId, targetId);
                best = std::min(best, timer.nsecsElapsed() / 1.0e6);
                // These are the repeats, so they are the passes running on state
                // the previous search left behind. A pass that stopped finding
                // the route would otherwise be reported as a faster one.
                QVERIFY2(found, qPrintable(qsl("scenario \"%1\" pass %2 found no path the first search found").arg(QString::fromUtf8(scenario.name), QString::number(pass))));
                QCOMPARE(pMap->mPathList.size(), steps);
            }

            const QString prefix = qsl("path_%1").arg(QString::fromUtf8(scenario.name));
            emitMetric(qsl("%1_ms").arg(prefix), best);
            // Workload invariant: a build that walked a different distance, or
            // to a different room, did not do the same work. Two routes of equal
            // length through different rooms would pass this.
            emitMetric(qsl("%1_steps").arg(prefix), static_cast<qint64>(steps));
            emitMetric(qsl("%1_target_room").arg(prefix), static_cast<qint64>(targetId));
        }

        // Where the fixed toll used to go. findPath() allocated two vertex-sized
        // vectors of its own and handed them to astar_search(), which writes four
        // property maps for every vertex in the WHOLE map before it looks at a
        // single edge - so a two-room walk paid for every room. Run on the
        // shortest scenario, where the search proper is a couple of vertices and
        // everything else is that toll.
        //
        // These describe the shape of that cost rather than reproducing what it
        // totalled: they report the best of several passes, so the arrays are
        // warm by then, and the heuristic no longer copies the location list per
        // search because that fix lives in the shared header this includes. For
        // the end-to-end figure compare path_adjacent_ms against a build made
        // without this change.
        {
            const int targetId = nearestTo(byCoordinate, centreX + 1, centreY, maxX - minX);
            const auto vertexCount = static_cast<std::size_t>(boost::num_vertices(pMap->g));
            const TMap::vertex start = pMap->roomidToIndex.value(startId);
            const TMap::vertex goal = pMap->roomidToIndex.value(targetId);
            double bestAlloc = std::numeric_limits<double>::max();
            double bestSearch = std::numeric_limits<double>::max();
            for (int pass = 0; pass < kPasses; ++pass) {
                timer.restart();
                std::vector<TMap::vertex> p(vertexCount);
                std::vector<cost> d(vertexCount);
                const double allocMs = timer.nsecsElapsed() / 1.0e6;
                timer.restart();
                try {
                    boost::astar_search(pMap->g,
                                        start,
                                        distance_heuristic<TMap::mygraph_t, cost, std::vector<location>>(pMap->locations, goal),
                                        boost::predecessor_map(&p[0]).distance_map(&d[0]).visitor(astar_goal_visitor<TMap::vertex>(goal)));
                } catch (const found_goal&) {
                }
                const double searchMs = timer.nsecsElapsed() / 1.0e6;
                bestAlloc = std::min(bestAlloc, allocMs);
                bestSearch = std::min(bestSearch, searchMs);
            }
            emitMetric("toll_scratch_alloc_ms", bestAlloc);
            emitMetric("toll_astar_search_ms", bestSearch);

            // Same search, but with every per-vertex array allocated once and
            // handed in: astar_search() still writes one entry per vertex, it
            // just no longer faults in fresh pages to do it.
            std::vector<TMap::vertex> predecessor(vertexCount);
            std::vector<cost> distance(vertexCount);
            std::vector<cost> rank(vertexCount);
            std::vector<boost::default_color_type> colour(vertexCount);
            double bestWarm = std::numeric_limits<double>::max();
            for (int pass = 0; pass < kPasses; ++pass) {
                timer.restart();
                try {
                    boost::astar_search(pMap->g,
                                        start,
                                        distance_heuristic<TMap::mygraph_t, cost, std::vector<location>>(pMap->locations, goal),
                                        boost::predecessor_map(&predecessor[0]).distance_map(&distance[0]).rank_map(&rank[0]).color_map(&colour[0]).visitor(astar_goal_visitor<TMap::vertex>(goal)));
                } catch (const found_goal&) {
                }
                bestWarm = std::min(bestWarm, timer.nsecsElapsed() / 1.0e6);
            }
            emitMetric("toll_astar_warm_ms", bestWarm);

            // And with no initialisation pass at all: the arrays start out at
            // their defaults and only the vertices the previous search touched
            // are put back, so the cost is the search and nothing else.
            constexpr cost infinite = std::numeric_limits<cost>::max();
            std::vector<std::size_t> touched;
            for (std::size_t i = 0; i < vertexCount; ++i) {
                predecessor[i] = i;
                distance[i] = infinite;
                rank[i] = infinite;
                colour[i] = boost::white_color;
            }
            const ScratchMap<TMap::vertex> predecessorMap(predecessor, touched);
            const ScratchMap<cost> distanceMap(distance, touched);
            const ScratchMap<cost> rankMap(rank, touched);
            const ScratchMap<boost::default_color_type> colourMap(colour, touched);
            double bestNoInit = std::numeric_limits<double>::max();
            for (int pass = 0; pass < kPasses; ++pass) {
                timer.restart();
                for (const std::size_t index : touched) {
                    predecessor[index] = index;
                    distance[index] = infinite;
                    rank[index] = infinite;
                    colour[index] = boost::white_color;
                }
                touched.clear();
                put(distanceMap, start, cost(0));
                put(rankMap, start, distance_heuristic<TMap::mygraph_t, cost, std::vector<location>>(pMap->locations, goal)(start));
                try {
                    boost::astar_search_no_init(
                            pMap->g,
                            start,
                            distance_heuristic<TMap::mygraph_t, cost, std::vector<location>>(pMap->locations, goal),
                            boost::predecessor_map(predecessorMap).distance_map(distanceMap).rank_map(rankMap).color_map(colourMap).visitor(astar_goal_visitor<TMap::vertex>(goal)));
                } catch (const found_goal&) {
                }
                bestNoInit = std::min(bestNoInit, timer.nsecsElapsed() / 1.0e6);
            }
            emitMetric("toll_astar_no_init_ms", bestNoInit);
            emitMetric("toll_astar_no_init_touched", static_cast<qint64>(touched.size()));

            // What replaced all of this is not measured again here: it is what
            // findPath() now runs, so the path_* timings above are already it.
        }

        if (const qint64 peakRssKb = readPeakRssKb(); peakRssKb > 0) {
            emitMetric("peak_rss_kb", peakRssKb);
        }
    }

    // EXPERIMENT (#3334): compare heuristic modes for optimality and work
    void benchHeuristics()
    {
        mudlet::self()->mSkipDefaultPackageInstall = true;
        Host* host = TestProfile::create(mHostname, mLocalhost, QString::number(mPort));
        QVERIFY(host);
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        QVERIFY2(connected.wait(3000), "could not connect to the stub");
        host->showHideOrCreateMapper(false);
        TMap* pMap = host->mpMap.data();
        QVERIFY2(pMap->restore(mMapPath), "could not restore map");
        gHeuristicMode = 0;
        pMap->initGraph();
        const int n = static_cast<int>(pMap->locations.size());
        emitMetric("map_rooms", static_cast<qint64>(pMap->mpRoomDB->size()));
        emitMetric("graph_vertices", static_cast<qint64>(n));
        emitMetric("heuristic_scale", static_cast<double>(gHeuristicScale));
        emitMetric("chebyshev_scale", static_cast<double>(gChebyshevScale));

        QHash<int, std::vector<int>> byArea;
        for (int i = 0; i < n; ++i) {
            byArea[pMap->locations[i].pR->getArea()].push_back(pMap->locations[i].id);
        }
        const int pairCount = qEnvironmentVariableIsSet("MUDLET_BENCH_PAIRS") ? qEnvironmentVariableIntValue("MUDLET_BENCH_PAIRS") : 300;

        auto pathCost = [pMap](int from) {
            double total = 0;
            unsigned int previous = from;
            for (const int roomId : std::as_const(pMap->mPathList)) {
                total += pMap->edgeHash.value(qMakePair(previous, static_cast<unsigned int>(roomId))).cost;
                previous = roomId;
            }
            return total;
        };

        for (const char* cls : {"uniform", "samearea"}) {
            QRandomGenerator rng(3334);
            std::vector<std::pair<int, int>> pairs;
            std::vector<double> optimum;
            int unreachable = 0;
            gHeuristicMode = 1;
            for (int attempt = 0; attempt < pairCount * 50 && static_cast<int>(pairs.size()) < pairCount; ++attempt) {
                const int from = pMap->locations[rng.bounded(n)].id;
                int to;
                if (qstrcmp(cls, "uniform") == 0) {
                    to = pMap->locations[rng.bounded(n)].id;
                } else {
                    const std::vector<int>& area = byArea[pMap->mpRoomDB->getRoom(from)->getArea()];
                    if (area.size() < 2) {
                        continue;
                    }
                    to = area[rng.bounded(static_cast<int>(area.size()))];
                }
                if (to == from) {
                    continue;
                }
                if (!pMap->findPath(from, to)) {
                    ++unreachable;
                    continue;
                }
                pairs.emplace_back(from, to);
                optimum.push_back(pathCost(from));
            }
            emitMetric(qsl("%1_pairs").arg(cls), static_cast<qint64>(pairs.size()));
            emitMetric(qsl("%1_unreachable_skipped").arg(cls), static_cast<qint64>(unreachable));

            const int modes[] = {0, 1, 4, 5};
            const char* modeNames[] = {"current", "zero", "scaled", "cheb", "chebarea", "chebtie"};
            std::vector<double> times[6];
            qint64 touched[6] = {0, 0, 0, 0, 0, 0};
            qint64 maxTouched[6] = {0, 0, 0, 0, 0, 0};
            int suboptimal[6] = {0, 0, 0, 0, 0, 0};
            double worstRatio[6] = {1, 1, 1, 1, 1, 1};
            int failed[6] = {0, 0, 0, 0, 0, 0};
            QElapsedTimer timer;
            for (std::size_t i = 0; i < pairs.size(); ++i) {
                for (const int m : modes) {
                    gHeuristicMode = m;
                    timer.restart();
                    const bool found = pMap->findPath(pairs[i].first, pairs[i].second);
                    times[m].push_back(timer.nsecsElapsed() / 1.0e6);
                    touched[m] += static_cast<qint64>(pMap->mLastSearchTouched);
                    maxTouched[m] = std::max(maxTouched[m], static_cast<qint64>(pMap->mLastSearchTouched));
                    if (!found) {
                        ++failed[m];
                        continue;
                    }
                    const double c = pathCost(pairs[i].first);
                    if (c > optimum[i] * (1 + 1e-5) + 1e-3) {
                        ++suboptimal[m];
                        worstRatio[m] = std::max(worstRatio[m], optimum[i] > 0 ? c / optimum[i] : 0.0);
                    }
                }
            }
            for (const int m : modes) {
                std::vector<double>& t = times[m];
                std::sort(t.begin(), t.end());
                double sum = 0;
                for (const double v : t) {
                    sum += v;
                }
                const QString p = qsl("%1_%2").arg(cls, modeNames[m]);
                emitMetric(qsl("%1_suboptimal").arg(p), static_cast<qint64>(suboptimal[m]));
                emitMetric(qsl("%1_worst_ratio").arg(p), worstRatio[m]);
                emitMetric(qsl("%1_failed").arg(p), static_cast<qint64>(failed[m]));
                emitMetric(qsl("%1_total_ms").arg(p), sum);
                if (!t.empty()) {
                    emitMetric(qsl("%1_median_ms").arg(p), t[t.size() / 2]);
                    emitMetric(qsl("%1_p95_ms").arg(p), t[t.size() * 95 / 100]);
                    emitMetric(qsl("%1_max_ms").arg(p), t.back());
                }
                emitMetric(qsl("%1_touched_total").arg(p), touched[m]);
                emitMetric(qsl("%1_touched_max").arg(p), maxTouched[m]);
            }
        }
        gHeuristicMode = 0;
    }

    // EXPERIMENT (#3334) verification pass: shuffled mode order, best-of-N timing,
    // h(start)/optimum, tie-plateau counts, equal-cost route differences, scenarios.
    Host* openBenchHost()
    {
        mudlet::self()->mSkipDefaultPackageInstall = true;
        Host* host = TestProfile::create(mHostname, mLocalhost, QString::number(mPort));
        if (!host) {
            return nullptr;
        }
        QSignalSpy connected(&(host->mTelnet), &cTelnet::signal_connected);
        connected.wait(3000);
        host->showHideOrCreateMapper(false);
        return host;
    }

    static QList<int> modesFromEnv()
    {
        QList<int> modes;
        const QString spec = qEnvironmentVariable("MUDLET_BENCH_MODES", qsl("0,1,4,5"));
        for (const QString& part : spec.split(QChar(','), Qt::SkipEmptyParts)) {
            modes.append(part.toInt());
        }
        return modes;
    }

    void verifyHeuristics()
    {
        Host* host = openBenchHost();
        QVERIFY(host);
        TMap* pMap = host->mpMap.data();
        QVERIFY2(pMap->restore(mMapPath), "could not restore map");
        const QList<int> modes = modesFromEnv();
        const int reps = qEnvironmentVariableIsSet("MUDLET_BENCH_REPS") ? qEnvironmentVariableIntValue("MUDLET_BENCH_REPS") : 5;
        const int pairCount = qEnvironmentVariableIsSet("MUDLET_BENCH_PAIRS") ? qEnvironmentVariableIntValue("MUDLET_BENCH_PAIRS") : 300;
        const QString mutate = qEnvironmentVariable("MUDLET_BENCH_MUTATE");
        const QHash<int, QString> names{{0, qsl("current")}, {1, qsl("zero")}, {2, qsl("scaled")}, {3, qsl("cheb")}, {4, qsl("chebarea")}, {5, qsl("chebtie")}};

        const int areaId = chooseArea(pMap);
        TArea* pArea = pMap->mpRoomDB->getArea(areaId);
        QVERIFY(pArea);
        const int zLevel = chooseZLevel(pArea);
        const QSet<int> roomsOnLevel = pArea->getRoomsForZ(zLevel);
        QHash<QPair<int, int>, int> byCoordinate;
        int minX = std::numeric_limits<int>::max(), maxX = std::numeric_limits<int>::min();
        int minY = std::numeric_limits<int>::max(), maxY = std::numeric_limits<int>::min();
        for (const int roomId : roomsOnLevel) {
            TRoom* pRoom = pMap->mpRoomDB->getRoom(roomId);
            if (!pRoom) {
                continue;
            }
            const QPair<int, int> key{pRoom->x(), pRoom->y()};
            const auto it = byCoordinate.constFind(key);
            if (it == byCoordinate.constEnd() || roomId < it.value()) {
                byCoordinate.insert(key, roomId);
            }
            minX = std::min(minX, pRoom->x());
            maxX = std::max(maxX, pRoom->x());
            minY = std::min(minY, pRoom->y());
            maxY = std::max(maxY, pRoom->y());
        }
        const int centreX = (minX + maxX) / 2;
        const int centreY = (minY + maxY) / 2;
        const int scenarioStart = nearestTo(byCoordinate, centreX, centreY, maxX - minX);
        const int cornerRoom = nearestTo(byCoordinate, maxX, maxY, maxX - minX);

        if (mutate == qsl("teleport")) {
            TRoom* pStart = pMap->mpRoomDB->getRoom(scenarioStart);
            QVERIFY(pStart);
            pStart->setSpecialExit(cornerRoom, qsl("experimentTeleport"));
            pMap->mMapGraphNeedsUpdate = true;
            emitMetric("mutate_teleport_from", static_cast<qint64>(scenarioStart));
            emitMetric("mutate_teleport_to", static_cast<qint64>(cornerRoom));
        }

        gHeuristicMode = 0;
        QElapsedTimer timer;
        timer.start();
        pMap->initGraph();
        emitMetric("init_graph_ms", timer.nsecsElapsed() / 1.0e6);
        emitMetric("scale_pass_ms", gScalePassMs);
        emitMetric("cheb_pass_ms", gChebPassMs);
        const int n = static_cast<int>(pMap->locations.size());
        emitMetric("map_rooms", static_cast<qint64>(pMap->mpRoomDB->size()));
        emitMetric("graph_vertices", static_cast<qint64>(n));
        emitMetric("chebyshev_scale_global", static_cast<double>(gChebyshevScale));
        emitMetric("bench_area_id", static_cast<qint64>(areaId));
        for (const location& l : pMap->locations) {
            if (l.pR->getArea() == areaId) {
                emitMetric("bench_area_chebyshev_scale", static_cast<double>(l.areaChebyshevScale));
                break;
            }
        }

        {
            qint64 edges = 0, special = 0, crossArea = 0, diagonal = 0, costAboveOne = 0;
            for (auto it = pMap->edgeHash.cbegin(); it != pMap->edgeHash.cend(); ++it) {
                ++edges;
                const quint8 dir = it.value().direction;
                if (dir == DIR_OTHER) {
                    ++special;
                }
                if (dir == DIR_NORTHEAST || dir == DIR_NORTHWEST || dir == DIR_SOUTHEAST || dir == DIR_SOUTHWEST) {
                    ++diagonal;
                }
                if (it.value().cost > 1) {
                    ++costAboveOne;
                }
                TRoom* a = pMap->mpRoomDB->getRoom(it.key().first);
                TRoom* b = pMap->mpRoomDB->getRoom(it.key().second);
                if (a && b && a->getArea() != b->getArea()) {
                    ++crossArea;
                }
            }
            emitMetric("edges_total", edges);
            emitMetric("edges_special", special);
            emitMetric("edges_cross_area", crossArea);
            emitMetric("edges_diagonal", diagonal);
            emitMetric("edges_cost_above_1", costAboveOne);
            QHash<QPair<int, int>, int> spacing;
            for (auto it = pMap->edgeHash.cbegin(); it != pMap->edgeHash.cend(); ++it) {
                const quint8 dir = it.value().direction;
                if (dir != DIR_EAST && dir != DIR_NORTH) {
                    continue;
                }
                TRoom* a = pMap->mpRoomDB->getRoom(it.key().first);
                TRoom* b = pMap->mpRoomDB->getRoom(it.key().second);
                if (a && b && a->getArea() == b->getArea()) {
                    spacing[{std::abs(a->x() - b->x()), std::abs(a->y() - b->y())}]++;
                }
            }
            QList<QPair<int, QPair<int, int>>> ranked;
            for (auto it = spacing.cbegin(); it != spacing.cend(); ++it) {
                ranked.append({it.value(), it.key()});
            }
            std::sort(ranked.begin(), ranked.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
            for (int i = 0; i < std::min<int>(3, ranked.size()); ++i) {
                emitMetric(qsl("ns_ew_spacing_rank%1_dx%2_dy%3_count").arg(i).arg(ranked[i].second.first).arg(ranked[i].second.second), static_cast<qint64>(ranked[i].first));
            }
        }

        QHash<int, std::vector<int>> byArea;
        for (int i = 0; i < n; ++i) {
            byArea[pMap->locations[i].pR->getArea()].push_back(pMap->locations[i].id);
        }
        auto pathCost = [pMap](int from) {
            double total = 0;
            unsigned int previous = from;
            for (const int roomId : std::as_const(pMap->mPathList)) {
                total += pMap->edgeHash.value(qMakePair(previous, static_cast<unsigned int>(roomId))).cost;
                previous = roomId;
            }
            return total;
        };
        auto sameCost = [](double a, double b) { return std::abs(a - b) <= 1e-5 * std::max(1.0, std::abs(b)) + 1e-3; };
        QRandomGenerator orderRng(99);

        struct Stats
        {
            std::vector<double> best;
            qint64 touched = 0;
            qint64 expanded = 0;
            qint64 expandedAtFinalF = 0;
            int suboptimal = 0;
            double worstRatio = 1;
            int failed = 0;
            std::vector<double> hRatio;
            int tieRouteDiffers = 0;
            int tieComparable = 0;
        };

        auto runPairs = [&](const QString& cls, const std::vector<std::pair<int, int>>& pairs, const std::vector<double>& optimum) {
            QHash<int, Stats> stats;
            for (std::size_t i = 0; i < pairs.size(); ++i) {
                QList<int> order = modes;
                for (int k = order.size() - 1; k > 0; --k) {
                    order.swapItemsAt(k, orderRng.bounded(k + 1));
                }
                QHash<int, double> best;
                for (int rep = 0; rep < reps; ++rep) {
                    for (const int m : std::as_const(order)) {
                        gHeuristicMode = m;
                        // untimed warm-up: the timed search then only pays for resetting its own
                        // previous state, not whatever another mode left behind
                        pMap->findPath(pairs[i].first, pairs[i].second);
                        timer.restart();
                        pMap->findPath(pairs[i].first, pairs[i].second);
                        const double ms = timer.nsecsElapsed() / 1.0e6;
                        best[m] = best.contains(m) ? std::min(best[m], ms) : ms;
                    }
                }
                QList<int> currentPath;
                bool currentOptimal = false;
                for (const int m : modes) {
                    Stats& st = stats[m];
                    st.best.push_back(best[m]);
                    gHeuristicMode = m;
                    gRecordF = true;
                    const bool found = pMap->findPath(pairs[i].first, pairs[i].second);
                    gRecordF = false;
                    st.touched += static_cast<qint64>(pMap->mLastSearchTouched);
                    st.expanded += static_cast<qint64>(gExpandedF.size());
                    if (!found) {
                        ++st.failed;
                        continue;
                    }
                    const double c = pathCost(pairs[i].first);
                    for (const float f : gExpandedF) {
                        if (sameCost(f, c)) {
                            ++st.expandedAtFinalF;
                        }
                    }
                    if (!sameCost(c, optimum[i]) && c > optimum[i]) {
                        ++st.suboptimal;
                        st.worstRatio = std::max(st.worstRatio, c / optimum[i]);
                    }
                    const TMap::vertex sv = pMap->roomidToIndex.value(pairs[i].first);
                    const TMap::vertex gv = pMap->roomidToIndex.value(pairs[i].second);
                    distance_heuristic<TMap::mygraph_t, cost, std::vector<location>> h(pMap->locations, gv);
                    if (optimum[i] > 0) {
                        st.hRatio.push_back(h(sv) / optimum[i]);
                    }
                    if (m == 0) {
                        currentPath = pMap->mPathList;
                        currentOptimal = sameCost(c, optimum[i]);
                    }
                }
                if (modes.contains(0) && currentOptimal) {
                    for (const int m : modes) {
                        if (m == 0 || m == 1) {
                            continue;
                        }
                        gHeuristicMode = m;
                        pMap->findPath(pairs[i].first, pairs[i].second);
                        stats[m].tieComparable++;
                        if (pMap->mPathList != currentPath) {
                            stats[m].tieRouteDiffers++;
                        }
                    }
                }
            }
            for (const int m : modes) {
                Stats& st = stats[m];
                const QString p = qsl("%1_%2").arg(cls, names.value(m));
                std::sort(st.best.begin(), st.best.end());
                std::sort(st.hRatio.begin(), st.hRatio.end());
                double sum = 0;
                for (const double v : st.best) {
                    sum += v;
                }
                emitMetric(qsl("%1_suboptimal").arg(p), static_cast<qint64>(st.suboptimal));
                emitMetric(qsl("%1_worst_ratio").arg(p), st.worstRatio);
                emitMetric(qsl("%1_failed").arg(p), static_cast<qint64>(st.failed));
                emitMetric(qsl("%1_best_total_ms").arg(p), sum);
                if (!st.best.empty()) {
                    emitMetric(qsl("%1_best_median_ms").arg(p), st.best[st.best.size() / 2]);
                    emitMetric(qsl("%1_best_p95_ms").arg(p), st.best[st.best.size() * 95 / 100]);
                    emitMetric(qsl("%1_best_max_ms").arg(p), st.best.back());
                }
                emitMetric(qsl("%1_touched_total").arg(p), st.touched);
                emitMetric(qsl("%1_expanded_total").arg(p), st.expanded);
                emitMetric(qsl("%1_expanded_at_final_f").arg(p), st.expandedAtFinalF);
                if (!st.hRatio.empty()) {
                    emitMetric(qsl("%1_hstart_ratio_median").arg(p), st.hRatio[st.hRatio.size() / 2]);
                    emitMetric(qsl("%1_hstart_ratio_max").arg(p), st.hRatio.back());
                    qint64 over = 0;
                    for (const double r : st.hRatio) {
                        if (r > 1 + 1e-5) {
                            ++over;
                        }
                    }
                    emitMetric(qsl("%1_hstart_over_1").arg(p), over);
                }
                if (m != 0 && m != 1) {
                    emitMetric(qsl("%1_tie_comparable").arg(p), static_cast<qint64>(st.tieComparable));
                    emitMetric(qsl("%1_tie_route_differs").arg(p), static_cast<qint64>(st.tieRouteDiffers));
                }
            }
        };

        for (const char* cls : {"uniform", "samearea"}) {
            QRandomGenerator rng(3334);
            std::vector<std::pair<int, int>> pairs;
            std::vector<double> optimum;
            int unreachable = 0;
            gHeuristicMode = 1;
            for (int attempt = 0; attempt < pairCount * 50 && static_cast<int>(pairs.size()) < pairCount; ++attempt) {
                const int from = pMap->locations[rng.bounded(n)].id;
                int to;
                if (qstrcmp(cls, "uniform") == 0) {
                    to = pMap->locations[rng.bounded(n)].id;
                } else {
                    const std::vector<int>& area = byArea[pMap->mpRoomDB->getRoom(from)->getArea()];
                    if (area.size() < 2) {
                        continue;
                    }
                    to = area[rng.bounded(static_cast<int>(area.size()))];
                }
                if (to == from) {
                    continue;
                }
                if (!pMap->findPath(from, to)) {
                    ++unreachable;
                    continue;
                }
                pairs.emplace_back(from, to);
                optimum.push_back(pathCost(from));
            }
            std::set<std::pair<int, int>> distinct(pairs.begin(), pairs.end());
            emitMetric(qsl("%1_pairs").arg(cls), static_cast<qint64>(pairs.size()));
            emitMetric(qsl("%1_pairs_distinct").arg(cls), static_cast<qint64>(distinct.size()));
            emitMetric(qsl("%1_unreachable_skipped").arg(cls), static_cast<qint64>(unreachable));
            runPairs(QString::fromLatin1(cls), pairs, optimum);
        }

        if (qEnvironmentVariableIntValue("MUDLET_BENCH_SCENARIOS") == 1 && scenarioStart > 0) {
            for (const Scenario& scenario : kScenarios) {
                const int targetId = (scenario.offset == 0) ? cornerRoom : nearestTo(byCoordinate, centreX + scenario.offset, centreY + scenario.offset, maxX - minX);
                if (targetId <= 0 || targetId == scenarioStart) {
                    continue;
                }
                gHeuristicMode = 1;
                QVERIFY(pMap->findPath(scenarioStart, targetId));
                const std::vector<std::pair<int, int>> one{{scenarioStart, targetId}};
                const std::vector<double> opt{pathCost(scenarioStart)};
                emitMetric(qsl("scen_%1_optimum").arg(QString::fromLatin1(scenario.name)), opt[0]);
                runPairs(qsl("scen_%1").arg(QString::fromLatin1(scenario.name)), one, opt);
            }
        }
        gHeuristicMode = 0;
        if (const qint64 peakRssKb = readPeakRssKb(); peakRssKb > 0) {
            emitMetric("peak_rss_kb", peakRssKb);
        }
    }

    // A cheap round trip out of the goal's area defeats an area-local lower bound;
    // also checks that the API cannot produce a step cost below 1.
    void adversarial()
    {
        Host* host = openBenchHost();
        QVERIFY(host);
        TMap* pMap = host->mpMap.data();
        const QString script = qsl(R"LUA(
local A = addAreaName("advA"); local B = addAreaName("advB"); local C = addAreaName("advC")
local function mk(id, area, x, y) addRoom(id); setRoomArea(id, area); setRoomCoordinates(id, x, y, 0) end
for i = 0, 10 do mk(100 + i, A, i * 10, 0) end
for i = 0, 9 do setExit(100 + i, 101 + i, "east"); setExit(101 + i, 100 + i, "west") end
mk(200, C, 0, 50)
mk(300, B, 0, 0)
setExit(200, 100, "south")
addSpecialExit(100, 300, "jump")
addSpecialExit(300, 110, "land")
addSpecialExit(200, 109, "long"); setExitWeight(200, "long", 5)
mk(400, A, 0, -10); mk(401, A, 10, -10)
setExit(400, 401, "east")
setExitWeight(400, "e", 0)
setRoomWeight(401, 0)
local ew = getExitWeights(400)
advResult = string.format("exitWeightAfterSet0=%s roomWeightAfterSet0=%s", tostring(ew and ew["e"]), tostring(getRoomWeight(401)))
)LUA");
        QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "adversarial map script failed");
        QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("local f = io.open(os.getenv('ADV_OUT'), 'w'); f:write(advResult); f:close()")));
        {
            QFile out(qEnvironmentVariable("ADV_OUT"));
            QVERIFY(out.open(QIODevice::ReadOnly));
            std::printf("ADV api %s\n", out.readAll().constData());
        }
        pMap->mMapGraphNeedsUpdate = true;
        for (const int m : {0, 1, 4, 5}) {
            gHeuristicMode = m;
            const bool found = pMap->findPath(200, 110);
            double total = 0;
            unsigned int previous = 200;
            QStringList steps;
            for (const int roomId : std::as_const(pMap->mPathList)) {
                total += pMap->edgeHash.value(qMakePair(previous, static_cast<unsigned int>(roomId))).cost;
                previous = roomId;
                steps << QString::number(roomId);
            }
            std::printf("ADV mode %d found %d cost %.1f path 200,%s\n", m, found, total, qPrintable(steps.join(QChar(','))));
        }
        {
            const unsigned int a = 400, b = 401;
            emitMetric("adv_edge_400_401_cost", static_cast<double>(pMap->edgeHash.value(qMakePair(a, b)).cost));
        }
        gHeuristicMode = 0;
    }

    // Do rooms of weight INT_MAX mislead the float search? Independent double Dijkstra in Lua.
    void floatCheck()
    {
        Host* host = openBenchHost();
        QVERIFY(host);
        TMap* pMap = host->mpMap.data();
        QVERIFY2(pMap->restore(mMapPath), "could not restore map");
        pMap->mMapGraphNeedsUpdate = true;
        const QString script = qsl(R"LUA(
local ids = {}
for id in pairs(getRooms()) do ids[#ids + 1] = id end
table.sort(ids)
local heavy = 0
for _, id in ipairs(ids) do if getRoomWeight(id) > 1 then heavy = heavy + 1 end end
local adj = {}
for _, id in ipairs(ids) do
  local n, ew = {}, getExitWeights(id) or {}
  local function add(dir, to) if roomExists(to) and not roomLocked(to) then local c = ew[dir] or getRoomWeight(to); if not n[to] or c < n[to] then n[to] = c end end end
  for dir, to in pairs(getRoomExits(id) or {}) do add(dir, to) end
  for cmd, to in pairs(getSpecialExitsSwap(id) or {}) do add(cmd, to) end
  adj[id] = n
end
local function exactCost(from)
  local total, prev = 0, from
  for _, rawId in ipairs(speedWalkPath) do
    local id = tonumber(rawId)
    local c = adj[prev][id]
    if not c then return nil end
    total = total + c; prev = id
  end
  return total
end
local total, mismatch, viaHeavy, example = 0, 0, 0, ""
for _, from in ipairs(ids) do
  local dist, done = {[from] = 0}, {}
  while true do
    local best, bd
    for id, d in pairs(dist) do if not done[id] and (not bd or d < bd) then best, bd = id, d end end
    if not best then break end
    done[best] = true
    for to, c in pairs(adj[best]) do if not dist[to] or bd + c < dist[to] then dist[to] = bd + c end end
  end
  for _, to in ipairs(ids) do
    if to ~= from and dist[to] then
      total = total + 1
      if dist[to] >= 2147483647 then viaHeavy = viaHeavy + 1 end
      if getPath(from, to) then
        local c = exactCost(from)
        if c ~= dist[to] then
          mismatch = mismatch + 1
          if dist[to] >= 2147483647 then mismatchHeavy = (mismatchHeavy or 0) + 1; maxExtra = math.max(maxExtra or 0, (c or 0) - dist[to]) end
          if example == "" then example = string.format("%d->%d got %.0f optimum %.0f", from, to, c or -1, dist[to]) end
        end
      else
        mismatch = mismatch + 1
      end
    end
  end
end
local f = io.open(os.getenv("ADV_OUT"), "w")
f:write(string.format("heavyRooms=%d pairs=%d pairsWhoseOptimumCrossesHeavy=%d mismatches=%d ofWhichOptimumCrossesHeavy=%d maxExtraCostWhenHeavy=%.0f firstMismatch=%s", heavy, total, viaHeavy, mismatch, mismatchHeavy or 0, maxExtra or 0, example))
mismatchHeavy, maxExtra = nil, nil
f:close()
)LUA");
        for (const int m : {1, 0}) {
            gHeuristicMode = m;
            QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "float check script failed");
            QFile out(qEnvironmentVariable("ADV_OUT"));
            QVERIFY(out.open(QIODevice::ReadOnly));
            std::printf("FLOATCHECK mode %d %s\n", m, out.readAll().constData());
        }
        gHeuristicMode = 0;
    }

private:
    int chooseArea(TMap* pMap) const
    {
        if (const int pinned = qEnvironmentVariableIntValue("MUDLET_BENCH_AREA"); pinned != 0) {
            return pinned;
        }
        int bestId = 0;
        int bestCount = -1;
        const QMap<int, TArea*>& areas = pMap->mpRoomDB->getAreaMap();
        for (auto it = areas.constBegin(); it != areas.constEnd(); ++it) {
            if (!it.value()) {
                continue;
            }
            const int count = it.value()->getAreaRooms().size();
            if (count > bestCount) {
                bestCount = count;
                bestId = it.key();
            }
        }
        return bestId;
    }

    static int chooseZLevel(TArea* pArea)
    {
        int bestZ = 0;
        int bestCount = -1;
        for (int z = pArea->min_z; z <= pArea->max_z; ++z) {
            const int count = pArea->getRoomsForZ(z).size();
            if (count > bestCount) {
                bestCount = count;
                bestZ = z;
            }
        }
        return bestZ;
    }

    // The room at (x, y) if the level holds one, else one found by an outward
    // ring search, so a level with holes in it still yields a pair. The rings
    // are square, so this is nearest by Chebyshev distance, and ties within a
    // ring go to the lowest room id.
    static int nearestTo(const QHash<QPair<int, int>, int>& byCoordinate, int x, int y, int maxRadius)
    {
        if (const auto it = byCoordinate.constFind({x, y}); it != byCoordinate.constEnd()) {
            return it.value();
        }
        for (int radius = 1; radius <= maxRadius; ++radius) {
            int best = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                for (int dy = -radius; dy <= radius; ++dy) {
                    if (std::max(std::abs(dx), std::abs(dy)) != radius) {
                        continue;
                    }
                    const auto it = byCoordinate.constFind({x + dx, y + dy});
                    if (it != byCoordinate.constEnd() && (best == 0 || it.value() < best)) {
                        best = it.value();
                    }
                }
            }
            if (best > 0) {
                return best;
            }
        }
        return 0;
    }

    void deleteProfileDirectory()
    {
        TestProfile::removeProfileDirectory(mHostname);
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

#include "PathfindBenchmark.moc"
QTEST_MAIN(PathfindBenchmark)
