p = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/test/functional_tests/PathfindBenchmark.cpp'
s = open(p).read()
anchor = """private:
    int chooseArea(TMap* pMap) const"""
assert anchor in s
code = r'''    // EXPERIMENT (#3334): component structure - weakly connected components (union-find) and
    // strongly connected components (Tarjan, iterative), with the share of rooms in the largest.
    void mapStats()
    {
        Host* host = openBenchHost();
        QVERIFY(host);
        TMap* pMap = host->mpMap.data();
        QVERIFY2(pMap->restore(mMapPath), "could not restore map");
        qputenv("MUDLET_ALT_K", "0");
        pMap->initGraph();
        const std::size_t n = boost::num_vertices(pMap->g);
        std::vector<quint32> parent(n);
        for (std::size_t v = 0; v < n; ++v) {
            parent[v] = v;
        }
        auto find = [&parent](quint32 v) {
            while (parent[v] != v) {
                parent[v] = parent[parent[v]];
                v = parent[v];
            }
            return v;
        };
        qint64 sinks = 0, sources = 0;
        std::vector<quint32> inDegree(n, 0);
        for (std::size_t v = 0; v < n; ++v) {
            if (boost::out_degree(v, pMap->g) == 0) {
                ++sinks;
            }
            for (const auto& e : boost::make_iterator_range(boost::out_edges(v, pMap->g))) {
                const quint32 t = boost::target(e, pMap->g);
                ++inDegree[t];
                const quint32 a = find(v), b = find(t);
                if (a != b) {
                    parent[a] = b;
                }
            }
        }
        for (std::size_t v = 0; v < n; ++v) {
            if (inDegree[v] == 0) {
                ++sources;
            }
        }
        QHash<quint32, qint64> sizes;
        for (std::size_t v = 0; v < n; ++v) {
            sizes[find(v)]++;
        }
        QList<qint64> sorted = sizes.values();
        std::sort(sorted.begin(), sorted.end(), std::greater<qint64>());
        emitMetric("stats_vertices", static_cast<qint64>(n));
        emitMetric("stats_sinks_no_exit", sinks);
        emitMetric("stats_sources_no_entrance", sources);
        emitMetric("stats_wcc_count", static_cast<qint64>(sorted.size()));
        for (int i = 0; i < std::min<int>(5, sorted.size()); ++i) {
            emitMetric(qsl("stats_wcc_size_rank%1").arg(i), sorted[i]);
        }
        qint64 singletons = 0;
        for (const qint64 size : sorted) {
            if (size == 1) {
                ++singletons;
            }
        }
        emitMetric("stats_wcc_singletons", singletons);

        // iterative Tarjan
        std::vector<qint32> index(n, -1), low(n, 0);
        std::vector<char> onStack(n, 0);
        std::vector<quint32> stack;
        std::vector<std::pair<quint32, std::size_t>> call;
        std::vector<qint64> sccSizes;
        qint32 counter = 0;
        std::vector<std::vector<quint32>> adjacency(n);
        for (std::size_t v = 0; v < n; ++v) {
            for (const auto& e : boost::make_iterator_range(boost::out_edges(v, pMap->g))) {
                adjacency[v].push_back(boost::target(e, pMap->g));
            }
        }
        for (std::size_t root = 0; root < n; ++root) {
            if (index[root] != -1) {
                continue;
            }
            call.push_back({static_cast<quint32>(root), 0});
            index[root] = low[root] = counter++;
            stack.push_back(root);
            onStack[root] = 1;
            while (!call.empty()) {
                auto& [v, i] = call.back();
                if (i < adjacency[v].size()) {
                    const quint32 w = adjacency[v][i++];
                    if (index[w] == -1) {
                        index[w] = low[w] = counter++;
                        stack.push_back(w);
                        onStack[w] = 1;
                        call.push_back({w, 0});
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
                        qint64 size = 0;
                        quint32 w;
                        do {
                            w = stack.back();
                            stack.pop_back();
                            onStack[w] = 0;
                            ++size;
                        } while (w != done);
                        sccSizes.push_back(size);
                    }
                }
            }
        }
        std::sort(sccSizes.begin(), sccSizes.end(), std::greater<qint64>());
        emitMetric("stats_scc_count", static_cast<qint64>(sccSizes.size()));
        for (int i = 0; i < std::min<int>(5, static_cast<int>(sccSizes.size())); ++i) {
            emitMetric(qsl("stats_scc_size_rank%1").arg(i), sccSizes[i]);
        }
    }

'''
s = s.replace(anchor, code + anchor, 1)
open(p, 'w').write(s)
print("mapStats added")
