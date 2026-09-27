#!/bin/bash
# usage: runbench2.sh <map> <test function> ; env passes MUDLET_BENCH_* through
cd /home/vadi/.claude/jobs/3ae74264/tmp
B=/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/build-linux-release/test/functional_tests/PathfindBenchmark
busy() {
  read -r _ a b c d e f g _ < /proc/stat; t1=$((a+b+c+d+e+f+g)); i1=$((d+e)); sleep 3
  read -r _ a b c d e f g _ < /proc/stat; t2=$((a+b+c+d+e+f+g)); i2=$((d+e))
  echo $(( 100 - 100*(i2-i1)/(t2-t1) ))
}
export ADV_OUT="$(mktemp /home/vadi/.claude/jobs/3ae74264/tmp/advout.XXXX)"
export QTEST_FUNCTION_TIMEOUT=20000000
echo "METRIC machine_busy_pct_before $(busy)"
echo "METRIC loadavg_before $(cut -d' ' -f1 /proc/loadavg)"
MUDLET_BENCH_MAP="maps/$1" QT_QPA_PLATFORM=offscreen MUDLET_TEST_MODE=1 timeout 20000 "$B" "$2" 2>&1 | grep -a -E "^METRIC|^ADV|^FLOATCHECK|FAIL!|Totals|QWARN.*findPath"
echo "METRIC machine_busy_pct_after $(busy)"
echo "METRIC loadavg_after $(cut -d' ' -f1 /proc/loadavg)"
rm -f "$ADV_OUT"
