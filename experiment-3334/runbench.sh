#!/bin/bash
# usage: runbench.sh <map> <pairs> [test function]
cd /home/vadi/.claude/jobs/3ae74264/tmp
B=/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/build-linux-release/test/functional_tests/PathfindBenchmark
busy() { read -r _ a b c d e f g _ < /proc/stat; t1=$((a+b+c+d+e+f+g)); i1=$((d+e)); sleep 3; read -r _ a b c d e f g _ < /proc/stat; t2=$((a+b+c+d+e+f+g)); i2=$((d+e)); echo $(( 100 - 100*(i2-i1)/(t2-t1) )); }
echo "METRIC machine_busy_pct_before $(busy)"
MUDLET_BENCH_MAP="maps/$1" MUDLET_BENCH_PAIRS="$2" QT_QPA_PLATFORM=offscreen MUDLET_TEST_MODE=1 timeout 3000 "$B" "${3:-benchHeuristics}" 2>&1 | grep -E "METRIC|FAIL!|Totals"
echo "METRIC machine_busy_pct_after $(busy)"
