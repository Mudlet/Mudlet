import re
W = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/'

p = W + 'src/TAstar.h'
s = open(p).read()
s = s.replace("typedef float cost;\n", """#ifdef MUDLET_EXPERIMENT_DOUBLE_COST
typedef double cost;
#else
typedef float cost;
#endif
""", 1)
s = s.replace("inline const float* gAltFrom = nullptr; // d(L_k, v)", "inline const cost* gAltFrom = nullptr; // d(L_k, v)", 1)
s = s.replace("inline const float* gAltTo = nullptr;   // d(v, L_k)", "inline const cost* gAltTo = nullptr;   // d(v, L_k)", 1)
s = s.replace("inline float gAltGoalFrom[64];", "inline cost gAltGoalFrom[64];", 1)
s = s.replace("inline float gAltGoalTo[64];", "inline cost gAltGoalTo[64];", 1)
s = s.replace("    float cost;              // Needed during establishing the best parallel edge", "    ::cost weight() const { return cost; }\n#ifdef MUDLET_EXPERIMENT_DOUBLE_COST\n    double cost;\n#else\n    float cost;              // Needed during establishing the best parallel edge\n#endif", 1)
s = s.replace("            constexpr float inf = std::numeric_limits<float>::infinity();\n            const float* fromLandmark", "            constexpr cost inf = std::numeric_limits<cost>::infinity();\n            const cost* fromLandmark", 1)
s = s.replace("            const float* toLandmark", "            const cost* toLandmark", 1)
open(p, 'w').write(s)

p = W + 'src/TMap.h'
s = open(p).read()
s = s.replace("    std::vector<float> mAltFrom; // EXPERIMENT\n    std::vector<float> mAltTo;   // EXPERIMENT\n", "    std::vector<cost> mAltFrom; // EXPERIMENT\n    std::vector<cost> mAltTo;   // EXPERIMENT\n", 1)
open(p, 'w').write(s)

p = W + 'src/TMap.cpp'
s = open(p).read()
a = s.index("void TMap::computeLandmarks()")
b = s.index("bool TMap::searchGraph(")
body = s[a:b]
body = body.replace("static constexpr float inf = std::numeric_limits<float>::infinity();", "static constexpr cost inf = std::numeric_limits<cost>::infinity();")
body = body.replace("std::vector<float> fW(m), rW(m);", "std::vector<cost> fW(m), rW(m);")
body = body.replace("const float w = boost::get(weights, e);", "const cost w = boost::get(weights, e);")
body = body.replace("typedef std::pair<float, quint32> entry;", "typedef std::pair<cost, quint32> entry;")
body = body.replace("const std::vector<float>& w, quint32 source, std::vector<float>& dist", "const std::vector<cost>& w, quint32 source, std::vector<cost>& dist")
body = body.replace("const float nd = d + w[i];", "const cost nd = d + w[i];")
body = body.replace("std::vector<float> dist, back;", "std::vector<cost> dist, back;")
body = body.replace("std::vector<float> nearestRoundTrip(n, inf);", "std::vector<cost> nearestRoundTrip(n, inf);")
body = body.replace("float farthest = -1;", "cost farthest = -1;")
body = body.replace("float best = 0;", "cost best = 0;")
body = body.replace("gAltBytes = (mAltFrom.capacity() + mAltTo.capacity()) * sizeof(float);", "gAltBytes = (mAltFrom.capacity() + mAltTo.capacity()) * sizeof(cost);")
s = s[:a] + body + s[b:]
s = s.replace("gAltGoalTo[k] = mAltTo.empty() ? std::numeric_limits<float>::infinity() :", "gAltGoalTo[k] = mAltTo.empty() ? std::numeric_limits<cost>::infinity() :", 1)
open(p, 'w').write(s)
print("double switch added")
