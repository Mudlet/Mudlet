W = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/'

# ---- TAstar.h ----
p = W + 'src/TAstar.h'
s = open(p).read()
old = "inline double gChebPassMs = 0;\n"
assert old in s
s = s.replace(old, old + """// 6 = ALT (landmarks + triangle inequality) with tie-break on h, 7 = ALT without tie-break.
// Landmark tables are vertex-major: [v * gAltK + k].
inline int gAltK = 0;
inline bool gAltUseTo = true;
inline const float* gAltFrom = nullptr; // d(L_k, v)
inline const float* gAltTo = nullptr;   // d(v, L_k)
inline float gAltGoalFrom[64];
inline float gAltGoalTo[64];
inline double gAltPassMs = 0;
inline std::size_t gAltBytes = 0;
inline int gAltLandmarksBuilt = 0;
inline std::vector<int> gAltLandmarkRooms;
""", 1)
old = """        if (gHeuristicMode == 1) {
            return 0;
        }"""
assert old in s
s = s.replace(old, old + """
        if (gHeuristicMode == 6 || gHeuristicMode == 7) {
            if (gAltK == 0 || !gAltFrom) {
                return 0;
            }
            constexpr float inf = std::numeric_limits<float>::infinity();
            const float* fromLandmark = gAltFrom + static_cast<std::size_t>(u) * gAltK;
            const float* toLandmark = gAltUseTo ? gAltTo + static_cast<std::size_t>(u) * gAltK : nullptr;
            CostType best = 0;
            for (int k = 0; k < gAltK; ++k) {
                // d(u, g) >= d(L, g) - d(L, u), valid only when L reaches u
                if (fromLandmark[k] < inf && gAltGoalFrom[k] < inf) {
                    best = std::max(best, gAltGoalFrom[k] - fromLandmark[k]);
                }
                // d(u, g) >= d(u, L) - d(g, L), valid only when g reaches L
                if (toLandmark && toLandmark[k] < inf && gAltGoalTo[k] < inf) {
                    best = std::max(best, toLandmark[k] - gAltGoalTo[k]);
                }
            }
            return best;
        }""", 1)
open(p, 'w').write(s)

# ---- TMap.h ----
p = W + 'src/TMap.h'
s = open(p).read()
old = "    void initGraph();\n"
assert old in s
s = s.replace(old, old + "    void computeLandmarks(); // EXPERIMENT\n    std::vector<float> mAltFrom; // EXPERIMENT\n    std::vector<float> mAltTo;   // EXPERIMENT\n", 1)
open(p, 'w').write(s)

# ---- TMap.cpp ----
p = W + 'src/TMap.cpp'
s = open(p).read()
old = "    mMapGraphNeedsUpdate = false;\n"
assert s.count(old) == 1
s = s.replace(old, "    computeLandmarks();\n" + old, 1)
s = s.replace("gHeuristicMode == 5 ? heuristic(start) : 0", "(gHeuristicMode == 5 || gHeuristicMode == 6) ? heuristic(start) : 0", 1)
s = s.replace("gHeuristicMode == 5 ? h : 0", "(gHeuristicMode == 5 || gHeuristicMode == 6) ? h : 0", 1)
old = """    gExpandedF.clear();
    mSearchDistance[start] = 0;"""
assert old in s
s = s.replace(old, """    gExpandedF.clear();
    if ((gHeuristicMode == 6 || gHeuristicMode == 7) && gAltK > 0 && !mAltFrom.empty()) {
        gAltFrom = mAltFrom.data();
        gAltTo = mAltTo.empty() ? nullptr : mAltTo.data();
        for (int k = 0; k < gAltK; ++k) {
            gAltGoalFrom[k] = mAltFrom[static_cast<std::size_t>(goal) * gAltK + k];
            gAltGoalTo[k] = mAltTo.empty() ? std::numeric_limits<float>::infinity() : mAltTo[static_cast<std::size_t>(goal) * gAltK + k];
        }
    }
    mSearchDistance[start] = 0;""", 1)

old = "bool TMap::searchGraph(const vertex start, const vertex goal)\n"
assert old in s
s = s.replace(old, r'''// EXPERIMENT (#3334): ALT landmark tables. Landmarks by farthest-point selection
// over forward distances; forward and (optionally) reverse Dijkstra per landmark.
void TMap::computeLandmarks()
{
    QElapsedTimer timer;
    timer.start();
    const int wantK = qEnvironmentVariableIsSet("MUDLET_ALT_K") ? qEnvironmentVariableIntValue("MUDLET_ALT_K") : 0;
    gAltUseTo = qEnvironmentVariableIntValue("MUDLET_ALT_FWD_ONLY") != 1;
    std::vector<float>().swap(mAltFrom);
    std::vector<float>().swap(mAltTo);
    gAltK = 0;
    gAltFrom = nullptr;
    gAltTo = nullptr;
    gAltLandmarksBuilt = 0;
    gAltLandmarkRooms.clear();
    const std::size_t n = boost::num_vertices(g);
    if (wantK <= 0 || n == 0) {
        gAltBytes = 0;
        gAltPassMs = timer.nsecsElapsed() / 1.0e6;
        return;
    }
    const int k = std::min(wantK, 64);
    constexpr float inf = std::numeric_limits<float>::infinity();

    std::vector<quint32> fOff(n + 1, 0), rOff(n + 1, 0);
    const WeightMap weights = boost::get(boost::edge_weight, g);
    for (std::size_t v = 0; v < n; ++v) {
        for (const auto& e : boost::make_iterator_range(boost::out_edges(v, g))) {
            ++fOff[v + 1];
            ++rOff[boost::target(e, g) + 1];
        }
    }
    for (std::size_t v = 0; v < n; ++v) {
        fOff[v + 1] += fOff[v];
        rOff[v + 1] += rOff[v];
    }
    const std::size_t m = fOff[n];
    std::vector<quint32> fTo(m), rTo(m);
    std::vector<float> fW(m), rW(m);
    {
        std::vector<quint32> fCur(fOff.begin(), fOff.end() - 1), rCur(rOff.begin(), rOff.end() - 1);
        for (std::size_t v = 0; v < n; ++v) {
            for (const auto& e : boost::make_iterator_range(boost::out_edges(v, g))) {
                const std::size_t t = boost::target(e, g);
                const float w = boost::get(weights, e);
                fTo[fCur[v]] = t;
                fW[fCur[v]++] = w;
                rTo[rCur[t]] = v;
                rW[rCur[t]++] = w;
            }
        }
    }
    typedef std::pair<float, quint32> entry;
    auto dijkstra = [n](const std::vector<quint32>& off, const std::vector<quint32>& to, const std::vector<float>& w, quint32 source, std::vector<float>& dist) {
        dist.assign(n, inf);
        std::priority_queue<entry, std::vector<entry>, std::greater<entry>> queue;
        dist[source] = 0;
        queue.push({0, source});
        while (!queue.empty()) {
            const auto [d, v] = queue.top();
            queue.pop();
            if (d > dist[v]) {
                continue;
            }
            for (quint32 i = off[v]; i < off[v + 1]; ++i) {
                const float nd = d + w[i];
                if (nd < dist[to[i]]) {
                    dist[to[i]] = nd;
                    queue.push({nd, to[i]});
                }
            }
        }
    };

    quint32 seed = 0;
    int lowestId = std::numeric_limits<int>::max();
    for (std::size_t v = 0; v < n; ++v) {
        if (locations[v].id < lowestId) {
            lowestId = locations[v].id;
            seed = v;
        }
    }
    std::vector<float> dist;
    dijkstra(fOff, fTo, fW, seed, dist);
    quint32 next = seed;
    float farthest = -1;
    for (std::size_t v = 0; v < n; ++v) {
        if (dist[v] < inf && dist[v] > farthest) {
            farthest = dist[v];
            next = v;
        }
    }

    mAltFrom.assign(n * k, inf);
    if (gAltUseTo) {
        mAltTo.assign(n * k, inf);
    }
    std::vector<float> nearestLandmark(n, inf);
    std::vector<char> chosen(n, 0);
    for (int landmark = 0; landmark < k; ++landmark) {
        chosen[next] = 1;
        gAltLandmarkRooms.push_back(locations[next].id);
        dijkstra(fOff, fTo, fW, next, dist);
        for (std::size_t v = 0; v < n; ++v) {
            mAltFrom[v * k + landmark] = dist[v];
            if (dist[v] < nearestLandmark[v]) {
                nearestLandmark[v] = dist[v];
            }
        }
        if (gAltUseTo) {
            dijkstra(rOff, rTo, rW, next, dist);
            for (std::size_t v = 0; v < n; ++v) {
                mAltTo[v * k + landmark] = dist[v];
            }
        }
        ++gAltLandmarksBuilt;
        float best = 0;
        bool found = false;
        for (std::size_t v = 0; v < n; ++v) {
            if (!chosen[v] && nearestLandmark[v] < inf && nearestLandmark[v] > best) {
                best = nearestLandmark[v];
                next = v;
                found = true;
            }
        }
        if (!found) {
            break;
        }
    }
    gAltK = k;
    gAltBytes = (mAltFrom.capacity() + mAltTo.capacity()) * sizeof(float);
    gAltPassMs = timer.nsecsElapsed() / 1.0e6;
}

''' + old, 1)
if "#include <queue>" not in s:
    s = s.replace("#include <QElapsedTimer>\n", "#include <QElapsedTimer>\n#include <queue>\n", 1)
open(p, 'w').write(s)
print("ALT patched")
