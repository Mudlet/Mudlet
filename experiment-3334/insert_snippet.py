p = '/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/test/functional_tests/PathfindBenchmark.cpp'
s = open(p).read()
anchor = """private:
    int chooseArea(TMap* pMap) const"""
assert anchor in s
snippet = open('/home/vadi/.claude/jobs/3ae74264/tmp/verify_snippet.cpp').read()
s = s.replace(anchor, snippet + anchor, 1)
if "#include <set>" not in s:
    s = s.replace("#include <limits>\n", "#include <limits>\n#include <set>\n", 1)
open(p, 'w').write(s)
print("inserted")
