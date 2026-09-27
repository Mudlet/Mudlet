p = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/src/TMap.cpp'
s = open(p).read()
start = s.index("    quint32 seed = 0;\n    int lowestId = std::numeric_limits<int>::max();")
end = s.index("    gAltK = k;\n    gAltBytes =")
new = r'''    // Landmarks go inside the largest strongly connected component: there every room both
    // reaches and is reached by every landmark, so both bounds are live for every pair in it.
    std::vector<qint32> sccOf(n, -1);
    qint32 largestScc = -1;
    std::size_t largestSize = 0;
    {
        std::vector<qint32> index(n, -1), low(n, 0);
        std::vector<char> onStack(n, 0);
        std::vector<quint32> stack;
        std::vector<std::pair<quint32, quint32>> call;
        qint32 counter = 0, sccCount = 0;
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
                        std::size_t size = 0;
                        quint32 w;
                        do {
                            w = stack.back();
                            stack.pop_back();
                            onStack[w] = 0;
                            sccOf[w] = sccCount;
                            ++size;
                        } while (w != done);
                        if (size > largestSize) {
                            largestSize = size;
                            largestScc = sccCount;
                        }
                        ++sccCount;
                    }
                }
            }
        }
    }
    gAltLargestScc = static_cast<qint64>(largestSize);

    quint32 seed = 0;
    int lowestId = std::numeric_limits<int>::max();
    for (std::size_t v = 0; v < n; ++v) {
        if (sccOf[v] == largestScc && locations[v].id < lowestId) {
            lowestId = locations[v].id;
            seed = v;
        }
    }
    const bool randomSelection = qEnvironmentVariable("MUDLET_ALT_SELECT") == qsl("random");
    std::vector<float> dist, back;
    // round trip to the nearest chosen landmark; the next landmark maximises it
    std::vector<float> nearestRoundTrip(n, inf);
    quint32 next = seed;
    {
        dijkstra(fOff, fTo, fW, seed, dist);
        dijkstra(rOff, rTo, rW, seed, back);
        float farthest = -1;
        for (std::size_t v = 0; v < n; ++v) {
            if (sccOf[v] == largestScc && dist[v] + back[v] > farthest) {
                farthest = dist[v] + back[v];
                next = v;
            }
        }
    }
    std::vector<quint32> sccMembers;
    if (randomSelection) {
        for (std::size_t v = 0; v < n; ++v) {
            if (sccOf[v] == largestScc) {
                sccMembers.push_back(v);
            }
        }
    }
    QRandomGenerator selectionRng(31337);

    mAltFrom.assign(n * k, inf);
    if (gAltUseTo) {
        mAltTo.assign(n * k, inf);
    }
    std::vector<char> chosen(n, 0);
    for (int landmark = 0; landmark < k; ++landmark) {
        if (randomSelection) {
            do {
                next = sccMembers[selectionRng.bounded(static_cast<quint32>(sccMembers.size()))];
            } while (chosen[next] && sccMembers.size() > static_cast<std::size_t>(landmark));
        }
        chosen[next] = 1;
        gAltLandmarkRooms.push_back(locations[next].id);
        dijkstra(fOff, fTo, fW, next, dist);
        dijkstra(rOff, rTo, rW, next, back);
        for (std::size_t v = 0; v < n; ++v) {
            mAltFrom[v * k + landmark] = dist[v];
            if (gAltUseTo) {
                mAltTo[v * k + landmark] = back[v];
            }
            if (sccOf[v] == largestScc) {
                nearestRoundTrip[v] = std::min(nearestRoundTrip[v], dist[v] + back[v]);
            }
        }
        ++gAltLandmarksBuilt;
        if (randomSelection) {
            continue;
        }
        float best = 0;
        bool found = false;
        for (std::size_t v = 0; v < n; ++v) {
            if (!chosen[v] && sccOf[v] == largestScc && nearestRoundTrip[v] > best) {
                best = nearestRoundTrip[v];
                next = v;
                found = true;
            }
        }
        if (!found) {
            break;
        }
    }
'''
s = s[:start] + new + s[end:]
if "#include <QRandomGenerator>" not in s:
    s = s.replace("#include <QElapsedTimer>\n", "#include <QElapsedTimer>\n#include <QRandomGenerator>\n", 1)
open(p, 'w').write(s)

p = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/src/TAstar.h'
s = open(p).read()
s = s.replace("inline int gAltLandmarksBuilt = 0;\n", "inline int gAltLandmarksBuilt = 0;\ninline qint64 gAltLargestScc = 0;\n", 1)
open(p, 'w').write(s)

p = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/test/functional_tests/PathfindBenchmark.cpp'
s = open(p).read()
old = '        emitMetric("alt_use_to", static_cast<qint64>(gAltUseTo ? 1 : 0));\n'
assert old in s
s = s.replace(old, old + '        emitMetric("alt_largest_scc", gAltLargestScc);\n        emitMetric("alt_select_random", static_cast<qint64>(qEnvironmentVariable("MUDLET_ALT_SELECT") == qsl("random") ? 1 : 0));\n', 1)
open(p, 'w').write(s)
print("selection patched")
