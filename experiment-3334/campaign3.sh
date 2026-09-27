#!/bin/bash
T=/home/vadi/.claude/jobs/3ae74264/tmp
export QTEST_FUNCTION_TIMEOUT=6000000
for m in sendar achaea aetolia imperian lusternia starmourn; do
  [ -s $T/results2/$m.txt ] || $T/runbench.sh $m.dat 300 > $T/results2/$m.txt.part && mv $T/results2/$m.txt.part $T/results2/$m.txt
done
[ -s $T/results2/aetherspace.txt ] || $T/runbench.sh aetherspace.dat 100 > $T/results2/aetherspace.txt.part && mv $T/results2/aetherspace.txt.part $T/results2/aetherspace.txt
for mode in 3 4; do
  [ -s $T/results2/aether-scen-$mode.txt ] || MUDLET_HEURISTIC_MODE=$mode $T/runbench.sh aetherspace.dat 1 benchFindPath > $T/results2/aether-scen-$mode.txt.part && mv $T/results2/aether-scen-$mode.txt.part $T/results2/aether-scen-$mode.txt
done
echo done > $T/results2/ALLDONE
