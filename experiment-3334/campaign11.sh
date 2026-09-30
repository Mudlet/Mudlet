#!/bin/bash
# mode 8: max(per-SCC ALT, Chebyshev in sealed areas), landmarks skipped where Chebyshev is tight
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results10
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
G="MUDLET_SKIP_SCALE_PASSES=1 MUDLET_GEO=1 MUDLET_GEO_SKIP=0.5"
SMALL="sendar achaea aetolia imperian lusternia starmourn"
run adversarial sendar.dat adversarial $G MUDLET_ALT_K=8
for m in $SMALL; do
  run main-$m $m.dat verifyHeuristics $G MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,6,8 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5
done
run main-aetherspace aetherspace.dat verifyHeuristics $G MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,6,8 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1
run teleport-aetherspace aetherspace.dat verifyHeuristics $G MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,6,8 MUDLET_BENCH_PAIRS=50 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1 MUDLET_BENCH_MUTATE=teleport
for m in $SMALL; do
  run audit-$m $m.dat auditHeuristic $G MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,6,8 MUDLET_AUDIT_GOALS=30
done
run audit-aetherspace aetherspace.dat auditHeuristic $G MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,8 MUDLET_AUDIT_GOALS=3
run audit-aetherspace-teleport aetherspace.dat auditHeuristic $G MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,8 MUDLET_AUDIT_GOALS=3 MUDLET_BENCH_MUTATE=teleport
for m in $SMALL; do
  run mapping-$m $m.dat mappingLoop $G MUDLET_ALT_K=8 MUDLET_MAPPING_MODE=8 MUDLET_MAPPING_CYCLES=10
done
run mapping-aetherspace aetherspace.dat mappingLoop $G MUDLET_ALT_K=8 MUDLET_MAPPING_MODE=8 MUDLET_MAPPING_CYCLES=4
echo done > $R/ALLDONE
