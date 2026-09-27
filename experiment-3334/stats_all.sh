#!/bin/bash
cd /home/vadi/.claude/jobs/3ae74264/tmp
for m in "$@"; do
  echo "== $m"
  ./runbench2.sh "$m.dat" mapStats | grep -E "stats_|FAIL" | sed 's/METRIC stats_//' | tr '\n' ' '
  echo
done
