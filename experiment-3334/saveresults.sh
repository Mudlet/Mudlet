#!/bin/bash
set -e
T=/home/vadi/.claude/jobs/3ae74264/tmp
E=/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/experiment-3334
rm -rf $E/results8
cp -r $T/results8 $T/results9 $T/results10 $E/
rm -f $E/results8/*.part
cp $T/campaign9.sh $T/campaign10.sh $T/campaign11.sh $T/altsumm8.py $T/altsumm10.py $T/waitunit.sh $T/saveresults.sh $E/
python3 $T/altsumm10.py > $E/results10/SUMMARY.txt
python3 $T/altsumm8.py > $E/results8/SUMMARY.txt
ls $E
