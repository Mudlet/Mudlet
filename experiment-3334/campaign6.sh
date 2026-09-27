#!/bin/bash
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results5
mkdir -p $R
busy() {
  read -r _ a b c d e f g _ < /proc/stat; t1=$((a+b+c+d+e+f+g)); i1=$((d+e)); sleep 3
  read -r _ a b c d e f g _ < /proc/stat; t2=$((a+b+c+d+e+f+g)); i2=$((d+e))
  echo $(( 100 - 100*(i2-i1)/(t2-t1) ))
}
gate() { # two consecutive quiet windows, give up after 60 min
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
for m in sendar achaea aetolia imperian lusternia starmourn; do
  run $m $m.dat verifyHeuristics MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5
done
run aetherspace aetherspace.dat verifyHeuristics MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1
run aetherspace-teleport aetherspace.dat verifyHeuristics MUDLET_BENCH_PAIRS=50 MUDLET_BENCH_REPS=3 MUDLET_BENCH_MODES=0,5 MUDLET_BENCH_SCENARIOS=1 MUDLET_BENCH_MUTATE=teleport
echo done > $R/ALLDONE
