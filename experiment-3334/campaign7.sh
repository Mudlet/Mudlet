#!/bin/bash
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results6
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
run() { # name map function [env...]
  local name=$1 map=$2 fn=$3; shift 3
  [ -s $R/$name.txt ] && return
  { gate; env "$@" $T/runbench2.sh $map $fn; } > $R/$name.txt.part && mv $R/$name.txt.part $R/$name.txt
}
SMALL="sendar achaea aetolia imperian lusternia starmourn"

# 1. main comparison, K=8 farthest
for m in $SMALL; do
  run main-$m $m.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,1,5,6,7 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5
done
run main-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,1,5,6,7 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1

# 2. exact admissibility / consistency audit
for m in $SMALL; do
  run audit-$m $m.dat auditHeuristic MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_AUDIT_GOALS=30
done
run audit-aetherspace aetherspace.dat auditHeuristic MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_AUDIT_GOALS=3
run audit-aetherspace-teleport aetherspace.dat auditHeuristic MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_AUDIT_GOALS=3 MUDLET_BENCH_MUTATE=teleport

# 3. teleport
run teleport-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,5,6 MUDLET_BENCH_PAIRS=50 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1 MUDLET_BENCH_MUTATE=teleport

# 4. variants
for K in 2 4 16; do
  for m in $SMALL; do
    run k$K-$m $m.dat verifyHeuristics MUDLET_ALT_K=$K MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=3
  done
  run k$K-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=$K MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3
done
for m in $SMALL; do
  run fwdonly-$m $m.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_ALT_FWD_ONLY=1 MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=3
  run random-$m $m.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_ALT_SELECT=random MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=3
done
run fwdonly-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_ALT_FWD_ONLY=1 MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3
run random-aetherspace aetherspace.dat verifyHeuristics MUDLET_ALT_K=8 MUDLET_ALT_SELECT=random MUDLET_BENCH_MODES=0,6 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3

# 5. mapping loop
for m in $SMALL; do
  run mapping-$m $m.dat mappingLoop MUDLET_ALT_K=8 MUDLET_MAPPING_CYCLES=10
done
run mapping-aetherspace aetherspace.dat mappingLoop MUDLET_ALT_K=8 MUDLET_MAPPING_CYCLES=3

# 6. adversarial + float
run adversarial sendar.dat adversarial MUDLET_ALT_K=8
run floatcheck sendar.dat floatCheck MUDLET_ALT_K=8
echo done > $R/ALLDONE
