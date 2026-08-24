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
 * Scenarios walk between rooms of the busiest Z level of the busiest area, at
 * increasing separations, because A* cost scales with how much of the graph the
 * frontier has to sweep - and the whole question is how much of the per-search
 * cost is that sweep and how much is a fixed toll paid on every search
 * regardless of how far apart the two rooms are.
 */

#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <algorithm>
#include <clocale>
#include <cstdio>
#include <limits>
#include <queue>

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
// over one of them remembers every vertex it wrote to, so the next search puts
// the defaults back for just those rather than for every room on the map -
// which is the whole of what astar_search() does before it looks at an edge.
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
        // Separation between the two rooms, in map units along one axis.
        int offset;
    };

    // 0 means "the far corner of the level", the longest walk it can hold.
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
        QCOMPARE(mudlet::getMudletPath(enums::mainPath), qsl("%1/mudlet").arg(mConfigDir.path()));
        mudlet::self()->takeOwnershipOfInstanceCoordinator(std::make_unique<MudletInstanceCoordinator>("MudletInstanceCoordinator"));
        mudlet::self()->init();
        mudlet::self()->setStorePasswordsSecurely(false);
        deleteProfileDirectory();
    }

    void cleanup()
    {
        delete mpServer;
        mpServer = nullptr;
        deleteProfileDirectory();
        delete mudlet::self();
    }

    void benchFindPath()
    {
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

            // One untimed search first: it settles any lazily-grown property
            // map inside BGL, and proves the pair really is connected before
            // the timings claim to describe a successful search.
            QVERIFY2(pMap->findPath(startId, targetId),
                     qPrintable(qsl("scenario \"%1\" found no path from %2 to %3, so its timings would describe a failed search")
                                        .arg(QString::fromUtf8(scenario.name), QString::number(startId), QString::number(targetId))));
            const int steps = pMap->mPathList.size();

            double best = std::numeric_limits<double>::max();
            for (int pass = 0; pass < kPasses; ++pass) {
                timer.restart();
                pMap->findPath(startId, targetId);
                best = std::min(best, timer.nsecsElapsed() / 1.0e6);
            }

            const QString prefix = qsl("path_%1").arg(QString::fromUtf8(scenario.name));
            emitMetric(qsl("%1_ms").arg(prefix), best);
            // Workload invariants: a build that walked a different distance, or
            // found a different route, did not do the same work.
            emitMetric(qsl("%1_steps").arg(prefix), static_cast<qint64>(steps));
            emitMetric(qsl("%1_target_room").arg(prefix), static_cast<qint64>(targetId));
        }

        // Where a search's fixed toll goes. findPath() allocates two
        // vertex-sized vectors of its own, and astar_search() then writes four
        // property maps for every vertex in the WHOLE map - both before it has
        // looked at a single edge - so a two-room walk on a 2.3M room map pays
        // for 2.3M rooms. Measured on the shortest scenario, where the search
        // proper is a couple of vertices and everything else is that toll.
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
            // handed in: astar_search() still writes all 2.3M entries, it just
            // no longer faults in fresh pages to do it.
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

            // And the same search written out by hand over the same persistent
            // arrays: a binary heap with lazy deletion, no BGL scaffolding, and
            // so nothing at all that is sized by the map rather than by the
            // search.
            const boost::property_map<TMap::mygraph_t, boost::edge_weight_t>::type weights = boost::get(boost::edge_weight, pMap->g);
            distance_heuristic<TMap::mygraph_t, cost, std::vector<location>> heuristic(pMap->locations, goal);
            typedef std::pair<cost, TMap::vertex> queueEntry;
            std::priority_queue<queueEntry, std::vector<queueEntry>, std::greater<queueEntry>> open;
            double bestManual = std::numeric_limits<double>::max();
            std::size_t manualExamined = 0;
            for (int pass = 0; pass < kPasses; ++pass) {
                timer.restart();
                for (const std::size_t index : touched) {
                    predecessor[index] = index;
                    distance[index] = infinite;
                    rank[index] = infinite;
                    colour[index] = boost::white_color;
                }
                touched.clear();
                open = {};
                manualExamined = 0;
                distance[start] = 0;
                touched.push_back(start);
                open.push({heuristic(start), start});
                while (!open.empty()) {
                    const TMap::vertex u = open.top().second;
                    open.pop();
                    if (colour[u] == boost::black_color) {
                        continue;
                    }
                    colour[u] = boost::black_color;
                    touched.push_back(u);
                    ++manualExamined;
                    if (u == goal) {
                        break;
                    }
                    for (const auto& edge : boost::make_iterator_range(boost::out_edges(u, pMap->g))) {
                        const TMap::vertex v = boost::target(edge, pMap->g);
                        const cost through = distance[u] + boost::get(weights, edge);
                        if (through < distance[v]) {
                            distance[v] = through;
                            predecessor[v] = u;
                            touched.push_back(v);
                            open.push({through + heuristic(v), v});
                        }
                    }
                }
                bestManual = std::min(bestManual, timer.nsecsElapsed() / 1.0e6);
            }
            emitMetric("toll_astar_manual_ms", bestManual);
            emitMetric("toll_astar_manual_examined", static_cast<qint64>(manualExamined));
        }

        if (const qint64 peakRssKb = readPeakRssKb(); peakRssKb > 0) {
            emitMetric("peak_rss_kb", peakRssKb);
        }
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

    // The room at (x, y) if the level holds one, else the nearest in an
    // outward ring search, so a level with holes in it still yields a pair.
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
        const QString path = mudlet::getMudletPath(enums::profileHomePath, mHostname);
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

#include "PathfindBenchmark.moc"
QTEST_MAIN(PathfindBenchmark)
