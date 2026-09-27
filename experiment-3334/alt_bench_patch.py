p = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/test/functional_tests/PathfindBenchmark.cpp'
s = open(p).read()

old = '{4, qsl("chebarea")}, {5, qsl("chebtie")}};'
assert old in s
s = s.replace(old, '{4, qsl("chebarea")}, {5, qsl("chebtie")}, {6, qsl("alt")}, {7, qsl("altnotie")}};', 1)

old = '        emitMetric("cheb_pass_ms", gChebPassMs);\n'
assert old in s
s = s.replace(old, old + '''        emitMetric("alt_pass_ms", gAltPassMs);
        emitMetric("alt_bytes", static_cast<qint64>(gAltBytes));
        emitMetric("alt_k", static_cast<qint64>(gAltK));
        emitMetric("alt_landmarks_built", static_cast<qint64>(gAltLandmarksBuilt));
        emitMetric("alt_use_to", static_cast<qint64>(gAltUseTo ? 1 : 0));
''', 1)

old = "        for (const int m : {0, 1, 4, 5}) {\n            gHeuristicMode = m;\n            const bool found = pMap->findPath(200, 110);"
assert old in s
s = s.replace(old, "        for (const int m : {0, 1, 4, 5, 6}) {\n            gHeuristicMode = m;\n            const bool found = pMap->findPath(200, 110);", 1)

old = "        for (const int m : {1, 0}) {\n            gHeuristicMode = m;\n            QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), \"float check script failed\");"
assert old in s
s = s.replace(old, "        for (const int m : {1, 0, 6}) {\n            gHeuristicMode = m;\n            QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), \"float check script failed\");", 1)

anchor = """private:
    int chooseArea(TMap* pMap) const"""
assert anchor in s
code = r'''    // EXPERIMENT (#3334): exact admissibility and consistency audit. For sampled goals, exact
    // d(u, goal) for every u by reverse Dijkstra; then h(u) <= d(u, goal) for all u, and
    // h(u) <= c(u, v) + h(v) for every edge.
    void auditHeuristic()
    {
        Host* host = openBenchHost();
        QVERIFY(host);
        TMap* pMap = host->mpMap.data();
        QVERIFY2(pMap->restore(mMapPath), "could not restore map");
        if (qEnvironmentVariable("MUDLET_BENCH_MUTATE") == qsl("teleport")) {
            const int areaId = chooseArea(pMap);
            TArea* pArea = pMap->mpRoomDB->getArea(areaId);
            QVERIFY(pArea);
            const QSet<int> rooms = pArea->getRoomsForZ(chooseZLevel(pArea));
            int lo = std::numeric_limits<int>::max(), hi = 0;
            for (const int id : rooms) {
                lo = std::min(lo, id);
                hi = std::max(hi, id);
            }
            pMap->mpRoomDB->getRoom(lo)->setSpecialExit(hi, qsl("experimentTeleport"));
            pMap->mMapGraphNeedsUpdate = true;
        }
        gHeuristicMode = 0;
        pMap->initGraph();
        emitMetric("alt_k", static_cast<qint64>(gAltK));
        const std::size_t n = boost::num_vertices(pMap->g);
        const TMap::WeightMap weights = boost::get(boost::edge_weight, pMap->g);
        std::vector<std::vector<std::pair<quint32, float>>> reverse(n);
        for (std::size_t v = 0; v < n; ++v) {
            for (const auto& e : boost::make_iterator_range(boost::out_edges(v, pMap->g))) {
                reverse[boost::target(e, pMap->g)].push_back({static_cast<quint32>(v), boost::get(weights, e)});
            }
        }
        const int goals = qEnvironmentVariableIsSet("MUDLET_AUDIT_GOALS") ? qEnvironmentVariableIntValue("MUDLET_AUDIT_GOALS") : 30;
        const QList<int> modes = modesFromEnv();
        const QHash<int, QString> names{{0, qsl("current")}, {4, qsl("chebarea")}, {5, qsl("chebtie")}, {6, qsl("alt")}, {7, qsl("altnotie")}, {1, qsl("zero")}};
        QHash<int, qint64> admissibleChecked, admissibleViolations, consistencyChecked, consistencyViolations;
        QHash<int, double> worstExcess, worstInconsistency;
        QRandomGenerator rng(4242);
        constexpr float inf = std::numeric_limits<float>::infinity();
        std::vector<float> dist;
        for (int gi = 0; gi < goals; ++gi) {
            const quint32 goal = rng.bounded(static_cast<quint32>(n));
            dist.assign(n, inf);
            typedef std::pair<float, quint32> entry;
            std::priority_queue<entry, std::vector<entry>, std::greater<entry>> queue;
            dist[goal] = 0;
            queue.push({0, goal});
            while (!queue.empty()) {
                const auto [d, v] = queue.top();
                queue.pop();
                if (d > dist[v]) {
                    continue;
                }
                for (const auto& [u, w] : reverse[v]) {
                    if (d + w < dist[u]) {
                        dist[u] = d + w;
                        queue.push({d + w, u});
                    }
                }
            }
            for (const int m : modes) {
                gHeuristicMode = m;
                if ((m == 6 || m == 7) && gAltK > 0) {
                    gAltFrom = pMap->mAltFrom.data();
                    gAltTo = pMap->mAltTo.empty() ? nullptr : pMap->mAltTo.data();
                    for (int k = 0; k < gAltK; ++k) {
                        gAltGoalFrom[k] = pMap->mAltFrom[static_cast<std::size_t>(goal) * gAltK + k];
                        gAltGoalTo[k] = pMap->mAltTo.empty() ? inf : pMap->mAltTo[static_cast<std::size_t>(goal) * gAltK + k];
                    }
                }
                distance_heuristic<TMap::mygraph_t, cost, std::vector<location>> h(pMap->locations, goal);
                std::vector<float> hv(n);
                for (std::size_t u = 0; u < n; ++u) {
                    hv[u] = h(u);
                }
                for (std::size_t u = 0; u < n; ++u) {
                    if (dist[u] < inf) {
                        admissibleChecked[m]++;
                        const double excess = hv[u] - dist[u];
                        if (excess > 1e-4 * std::max(1.0f, dist[u]) + 1e-3) {
                            admissibleViolations[m]++;
                            worstExcess[m] = std::max(worstExcess.value(m, 0), excess);
                        }
                    }
                    for (const auto& e : boost::make_iterator_range(boost::out_edges(u, pMap->g))) {
                        const std::size_t v = boost::target(e, pMap->g);
                        consistencyChecked[m]++;
                        const double gap = hv[u] - (boost::get(weights, e) + hv[v]);
                        if (gap > 1e-4 * std::max(1.0f, hv[u]) + 1e-3) {
                            consistencyViolations[m]++;
                            worstInconsistency[m] = std::max(worstInconsistency.value(m, 0), gap);
                        }
                    }
                }
            }
        }
        for (const int m : modes) {
            const QString p = qsl("audit_%1").arg(names.value(m, QString::number(m)));
            emitMetric(qsl("%1_admissible_checked").arg(p), admissibleChecked.value(m));
            emitMetric(qsl("%1_admissible_violations").arg(p), admissibleViolations.value(m));
            emitMetric(qsl("%1_worst_excess").arg(p), worstExcess.value(m, 0));
            emitMetric(qsl("%1_consistency_checked").arg(p), consistencyChecked.value(m));
            emitMetric(qsl("%1_consistency_violations").arg(p), consistencyViolations.value(m));
            emitMetric(qsl("%1_worst_inconsistency").arg(p), worstInconsistency.value(m, 0));
        }
        gHeuristicMode = 0;
    }

    // EXPERIMENT (#3334): mapping workload - every edit invalidates the graph, so the next
    // getPath() pays initGraph() (and, with ALT, the landmark rebuild).
    void mappingLoop()
    {
        Host* host = openBenchHost();
        QVERIFY(host);
        TMap* pMap = host->mpMap.data();
        QVERIFY2(pMap->restore(mMapPath), "could not restore map");
        const int cycles = qEnvironmentVariableIsSet("MUDLET_MAPPING_CYCLES") ? qEnvironmentVariableIntValue("MUDLET_MAPPING_CYCLES") : 10;
        const QByteArray altK = qgetenv("MUDLET_ALT_K");
        gHeuristicMode = 0;
        pMap->initGraph();
        const int n = static_cast<int>(pMap->locations.size());
        QRandomGenerator rng(777);
        QElapsedTimer timer;
        for (const bool withAlt : {false, true}) {
            qputenv("MUDLET_ALT_K", withAlt ? altK : QByteArray("0"));
            gHeuristicMode = withAlt ? 6 : 0;
            std::vector<double> times;
            for (int c = 0; c < cycles; ++c) {
                const int anchor = pMap->locations[rng.bounded(n)].id;
                const int from = pMap->locations[rng.bounded(n)].id;
                const QString script = qsl("local a = %1; local id = createRoomID(); addRoom(id); setRoomArea(id, getRoomArea(a)); "
                                           "local x, y, z = getRoomCoordinates(a); setRoomCoordinates(id, x, y, z + 1); "
                                           "setExit(a, id, 'up'); setExit(id, a, 'down'); mappingLoopRoom = id")
                                               .arg(anchor);
                QVERIFY2(host->getLuaInterpreter()->compileAndExecuteScript(script), "mapping edit failed");
                QVERIFY(host->getLuaInterpreter()->compileAndExecuteScript(qsl("local f = io.open(os.getenv('ADV_OUT'), 'w'); f:write(mappingLoopRoom); f:close()")));
                QFile out(qEnvironmentVariable("ADV_OUT"));
                QVERIFY(out.open(QIODevice::ReadOnly));
                const int newRoom = out.readAll().toInt();
                QVERIFY(pMap->mMapGraphNeedsUpdate);
                timer.restart();
                pMap->findPath(from, newRoom);
                times.push_back(timer.nsecsElapsed() / 1.0e6);
            }
            std::sort(times.begin(), times.end());
            const QString p = withAlt ? qsl("mapping_alt") : qsl("mapping_current");
            emitMetric(qsl("%1_median_ms").arg(p), times[times.size() / 2]);
            emitMetric(qsl("%1_max_ms").arg(p), times.back());
        }
        qputenv("MUDLET_ALT_K", altK);
        gHeuristicMode = 0;
    }

'''
s = s.replace(anchor, code + anchor, 1)
if "#include <queue>" not in s:
    s = s.replace("#include <set>\n", "#include <queue>\n#include <set>\n", 1)
open(p, 'w').write(s)
print("bench patched")
