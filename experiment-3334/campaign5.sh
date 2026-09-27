#!/bin/bash
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results4
mkdir -p $R
run() { # name map function [env...]
  local name=$1 map=$2 fn=$3; shift 3
  [ -s $R/$name.txt ] && return
  env "$@" $T/runbench2.sh $map $fn > $R/$name.txt.part && mv $R/$name.txt.part $R/$name.txt
}
for m in sendar achaea aetolia imperian lusternia starmourn; do
  run $m $m.dat verifyHeuristics MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5
done
run aetherspace aetherspace.dat verifyHeuristics MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=5 MUDLET_BENCH_SCENARIOS=1
run aetherspace-teleport aetherspace.dat verifyHeuristics MUDLET_BENCH_PAIRS=50 MUDLET_BENCH_REPS=3 MUDLET_BENCH_MODES=0,5 MUDLET_BENCH_SCENARIOS=1 MUDLET_BENCH_MUTATE=teleport
echo done > $R/ALLDONE
