#!/bin/bash
T=/home/vadi/.claude/jobs/3ae74264/tmp
export QTEST_FUNCTION_TIMEOUT=3000000
$T/runbench.sh aetherspace.dat 100 > $T/results/aetherspace2.txt.part && mv $T/results/aetherspace2.txt.part $T/results/aetherspace2.txt
echo done > $T/results/ALLDONE2
