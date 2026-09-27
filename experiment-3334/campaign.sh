#!/bin/bash
T=/home/vadi/.claude/jobs/3ae74264/tmp
mkdir -p $T/results
for m in achaea aetolia imperian lusternia starmourn; do
  [ -s $T/results/$m.txt ] || $T/runbench.sh $m.dat 300 > $T/results/$m.txt.part && mv $T/results/$m.txt.part $T/results/$m.txt
done
for m in million aetherspace; do
  [ -s $T/results/$m.txt ] || $T/runbench.sh $m.dat 100 > $T/results/$m.txt.part && mv $T/results/$m.txt.part $T/results/$m.txt
done
for mode in 0 1 2; do
  [ -s $T/results/aether-scen-$mode.txt ] || MUDLET_HEURISTIC_MODE=$mode $T/runbench.sh aetherspace.dat 1 benchFindPath > $T/results/aether-scen-$mode.txt.part && mv $T/results/aether-scen-$mode.txt.part $T/results/aether-scen-$mode.txt
done
echo done > $T/results/ALLDONE
