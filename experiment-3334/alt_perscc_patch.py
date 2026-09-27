W = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/'
p = W + 'src/TMap.cpp'
s = open(p).read()
start = s.index("    // Landmarks go inside the largest strongly connected component: there every room both")
end = s.index("    gAltK = k;\n")
new = r'''    // Every strongly connected component gets its own landmarks, and a room's K slots refer
    // to its own component's. A shortest path between two rooms of one component never leaves
    // it, so each landmark's searches stay inside the component and the total work is about
    // K x rooms however the map is split.
    std::vector<qint32> sccOf(n, -1);
    std::vector<quint32> sccSize;
    {
        std::vector<qint32> index(n, -1), low(n, 0);
        std::vector<char> onStack(n, 0);
        std::vector<quint32> stack;
        std::vector<std::pair<quint32, quint32>> call;
        qint32 counter = 0;
        for (std::size_t root = 0; root < n; ++root) {
            if (index[root] != -1) {
                continue;
            }
            call.push_back({static_cast<quint32>(root), fOff[root]});
            index[root] = low[root] = counter++;
            stack.push_back(root);
            onStack[root] = 1;
            while (!call.empty()) {
                auto& [v, i] = call.back();
                if (i < fOff[v + 1]) {
                    const quint32 w = fTo[i++];
                    if (index[w] == -1) {
                        index[w] = low[w] = counter++;
                        stack.push_back(w);
                        onStack[w] = 1;
                        call.push_back({w, fOff[w]});
                    } else if (onStack[w]) {
                        low[v] = std::min(low[v], index[w]);
                    }
                } else {
                    const quint32 done = v;
                    call.pop_back();
                    if (!call.empty()) {
                        low[call.back().first] = std::min(low[call.back().first], low[done]);
                    }
                    if (low[done] == index[done]) {
                        const qint32 id = static_cast<qint32>(sccSize.size());
                        quint32 size = 0;
                        quint32 w;
                        do {
                            w = stack.back();
                            stack.pop_back();
                            onStack[w] = 0;
                            sccOf[w] = id;
                            ++size;
                        } while (w != done);
                        sccSize.push_back(size);
                    }
                }
            }
        }
    }
    const std::size_t sccCount = sccSize.size();
    std::vector<quint32> sccStart(sccCount + 1, 0);
    for (std::size_t c = 0; c < sccCount; ++c) {
        sccStart[c + 1] = sccStart[c] + sccSize[c];
    }
    std::vector<quint32> members(n), localIndex(n);
    {
        std::vector<quint32> cursor(sccStart.begin(), sccStart.end() - 1);
        for (std::size_t v = 0; v < n; ++v) {
            const quint32 c = sccOf[v];
            localIndex[v] = cursor[c] - sccStart[c];
            members[cursor[c]++] = v;
        }
    }
    quint32 largest = 0;
    for (const quint32 size : sccSize) {
        largest = std::max(largest, size);
    }
    gAltLargestScc = largest;
    mAltScc = sccOf;

    auto localDijkstra = [&](const std::vector<quint32>& off, const std::vector<quint32>& to, const std::vector<cost>& w, quint32 source, std::vector<cost>& dist) {
        const qint32 c = sccOf[source];
        dist.assign(sccSize[c], inf);
        std::priority_queue<entry, std::vector<entry>, std::greater<entry>> queue;
        dist[localIndex[source]] = 0;
        queue.push({0, source});
        while (!queue.empty()) {
            const auto [d, v] = queue.top();
            queue.pop();
            if (d > dist[localIndex[v]]) {
                continue;
            }
            for (quint32 i = off[v]; i < off[v + 1]; ++i) {
                const quint32 t = to[i];
                if (sccOf[t] != c) {
                    continue;
                }
                const cost nd = d + w[i];
                if (nd < dist[localIndex[t]]) {
                    dist[localIndex[t]] = nd;
                    queue.push({nd, t});
                }
            }
        }
    };

    const bool randomSelection = qEnvironmentVariable("MUDLET_ALT_SELECT") == qsl("random");
    QRandomGenerator selectionRng(31337);
    mAltFrom.assign(n * k, inf);
    if (gAltUseTo) {
        mAltTo.assign(n * k, inf);
    }
    std::vector<cost> dist, back, nearestRoundTrip;
    std::vector<char> chosen;
    qint64 componentsWithLandmarks = 0;
    for (std::size_t c = 0; c < sccCount; ++c) {
        const quint32 size = sccSize[c];
        if (size < 2) {
            continue;
        }
        ++componentsWithLandmarks;
        const quint32* memberBegin = members.data() + sccStart[c];
        quint32 seed = memberBegin[0];
        for (quint32 i = 1; i < size; ++i) {
            if (locations[memberBegin[i]].id < locations[seed].id) {
                seed = memberBegin[i];
            }
        }
        localDijkstra(fOff, fTo, fW, seed, dist);
        localDijkstra(rOff, rTo, rW, seed, back);
        quint32 next = seed;
        cost farthest = -1;
        for (quint32 i = 0; i < size; ++i) {
            if (dist[i] + back[i] > farthest) {
                farthest = dist[i] + back[i];
                next = memberBegin[i];
            }
        }
        nearestRoundTrip.assign(size, inf);
        chosen.assign(size, 0);
        const int landmarks = static_cast<int>(std::min<quint32>(k, size));
        for (int landmark = 0; landmark < landmarks; ++landmark) {
            if (randomSelection) {
                do {
                    next = memberBegin[selectionRng.bounded(size)];
                } while (chosen[localIndex[next]]);
            }
            chosen[localIndex[next]] = 1;
            if (static_cast<quint32>(largest) == size) {
                gAltLandmarkRooms.push_back(locations[next].id);
            }
            localDijkstra(fOff, fTo, fW, next, dist);
            localDijkstra(rOff, rTo, rW, next, back);
            for (quint32 i = 0; i < size; ++i) {
                const std::size_t v = memberBegin[i];
                mAltFrom[v * k + landmark] = dist[i];
                if (gAltUseTo) {
                    mAltTo[v * k + landmark] = back[i];
                }
                nearestRoundTrip[i] = std::min(nearestRoundTrip[i], dist[i] + back[i]);
            }
            ++gAltLandmarksBuilt;
            if (randomSelection) {
                continue;
            }
            cost best = -1;
            for (quint32 i = 0; i < size; ++i) {
                if (!chosen[i] && nearestRoundTrip[i] > best) {
                    best = nearestRoundTrip[i];
                    next = memberBegin[i];
                }
            }
        }
    }
    gAltComponentsWithLandmarks = componentsWithLandmarks;
    gAltComponents = static_cast<qint64>(sccCount);
'''
s = s[:start] + new + s[end:]
# searchGraph: publish the component ids and the goal's component
old = """        gAltFrom = mAltFrom.data();
        gAltTo = mAltTo.empty() ? nullptr : mAltTo.data();"""
assert old in s
s = s.replace(old, old + """
        gAltScc = mAltScc.data();
        gAltGoalScc = mAltScc[goal];""", 1)
# clear on rebuild
old = "    std::vector<cost>().swap(mAltTo);\n"
assert old in s
s = s.replace(old, old + "    std::vector<qint32>().swap(mAltScc);\n    gAltScc = nullptr;\n", 1)
open(p, 'w').write(s)

p = W + 'src/TMap.h'
s = open(p).read()
old = "    std::vector<cost> mAltTo;   // EXPERIMENT\n"
assert old in s
s = s.replace(old, old + "    std::vector<qint32> mAltScc; // EXPERIMENT: strongly connected component of each vertex\n", 1)
open(p, 'w').write(s)

p = W + 'src/TAstar.h'
s = open(p).read()
old = "inline qint64 gAltLargestScc = 0;\n"
assert old in s
s = s.replace(old, old + "inline qint64 gAltComponents = 0;\ninline qint64 gAltComponentsWithLandmarks = 0;\ninline const qint32* gAltScc = nullptr;\ninline qint32 gAltGoalScc = -1;\n", 1)
old = """            if (gAltK == 0 || !gAltFrom) {
                return 0;
            }"""
assert old in s
s = s.replace(old, """            // slots name the room's own component's landmarks, so they only compare with the goal's
            if (gAltK == 0 || !gAltFrom || !gAltScc || gAltScc[u] != gAltGoalScc) {
                return 0;
            }""", 1)
open(p, 'w').write(s)

p = W + 'test/functional_tests/PathfindBenchmark.cpp'
s = open(p).read()
old = '        emitMetric("alt_largest_scc", gAltLargestScc);\n'
assert old in s
s = s.replace(old, old + '        emitMetric("alt_components", gAltComponents);\n        emitMetric("alt_components_with_landmarks", gAltComponentsWithLandmarks);\n', 1)
# audit: set component pointers too, and classify inconsistencies
old = """                if ((m == 6 || m == 7) && gAltK > 0) {
                    gAltFrom = pMap->mAltFrom.data();"""
assert old in s
s = s.replace(old, """                if ((m == 6 || m == 7) && gAltK > 0) {
                    gAltScc = pMap->mAltScc.data();
                    gAltGoalScc = pMap->mAltScc[goal];
                    gAltFrom = pMap->mAltFrom.data();""", 1)
old = """                        if (gap > 1e-4 * std::max(1.0f, hv[u]) + 1e-3) {
                            consistencyViolations[m]++;
                            worstInconsistency[m] = std::max(worstInconsistency.value(m, 0), gap);
                        }"""
assert old in s
s = s.replace(old, """                        if (gap > 1e-4 * std::max(1.0f, hv[u]) + 1e-3) {
                            consistencyViolations[m]++;
                            worstInconsistency[m] = std::max(worstInconsistency.value(m, 0), gap);
                            // an edge whose far end cannot reach the goal is never on a route to it
                            if (dist[u] < inf && dist[v] < inf) {
                                consistencyViolationsOnRoutes[m]++;
                            }
                        }""", 1)
old = "        QHash<int, qint64> admissibleChecked, admissibleViolations, consistencyChecked, consistencyViolations;\n"
assert old in s
s = s.replace(old, "        QHash<int, qint64> admissibleChecked, admissibleViolations, consistencyChecked, consistencyViolations, consistencyViolationsOnRoutes;\n", 1)
old = """            emitMetric(qsl("%1_worst_inconsistency").arg(p), worstInconsistency.value(m, 0));"""
assert old in s
s = s.replace(old, old + """
            emitMetric(qsl("%1_consistency_violations_goal_reachable").arg(p), consistencyViolationsOnRoutes.value(m));""", 1)
open(p, 'w').write(s)
print("per-SCC ALT patched")
