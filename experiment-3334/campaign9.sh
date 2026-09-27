#!/bin/bash
# per-SCC ALT rerun
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results8
mkdir -p $R
busy() {
  read -r _ a b c d e f g _ < /proc/stat; t1=$((a+b+c+d+e+f+g)); i1=$((d+e)); sleep 3
  read -r _ a b c d e f g _ < /proc/stat; t2=$((a+b+c+d+e+f+g)); i2=$((d+e))
  echo $(( 100 - 100*(i2-i1)/(t2-t1) ))
}
gate() {
  local waited=0 quiet=0
  while [ $waited -lt 3600 ]; do
    if [ "$(busy)" -lt 15 ]; then quiet=$((quiet+1)); else quiet=0; fi
    [ $quiet -ge 2 ] && { echo "METRIC gate_waited_s $waited"; return; }
    sleep 10; waited=$((waited+13))
  done
  echo "METRIC gate_gave_up 1"
}
run() {
  local name=$1 map=$2 fn=$3; shift 3
  [ -s $R/$name.txt ] && return
  { gate; env "$@" $T/runbench2.sh $map $fn; } > $R/$name.txt.part && mv $R/$name.txt.part $R/$name.txt
}
SMALL="sendar achaea aetolia imperian lusternia starmourn"
run stats-aetherspace aetherspace.dat mapStats
for m in $SMALL; do
  run main-$m $m.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,1,5,6 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5
done
run main-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,1,5,6 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1
for m in $SMALL; do
  run audit-$m $m.dat auditHeuristic MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_AUDIT_GOALS=30
done
run audit-aetherspace aetherspace.dat auditHeuristic MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_AUDIT_GOALS=3
run audit-aetherspace-teleport aetherspace.dat auditHeuristic MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_AUDIT_GOALS=3 MUDLET_BENCH_MUTATE=teleport
run teleport-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_BENCH_PAIRS=50 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1 MUDLET_BENCH_MUTATE=teleport
for K in 4 16; do
  for m in $SMALL; do
    run k$K-$m $m.dat verifyHeuristics MUDLET_ALT_K=$K MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=3
  done
  run k$K-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=$K MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3
done
for m in $SMALL; do
  run mapping-$m $m.dat mappingLoop MUDLET_ALT_K=8 MUDLET_MAPPING_CYCLES=10
done
run mapping-aetherspace aetherspace.dat mappingLoop MUDLET_ALT_K=8 MUDLET_MAPPING_CYCLES=3
echo done > $R/ALLDONE
