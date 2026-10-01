#!/bin/bash
set -e
T=/home/vadi/.claude/jobs/3ae74264/tmp
E=/home/vadi/Programs/Mudlet/.claude/worktrees/heuristic-3334/experiment-3334
rm -f $T/results11/*.part $T/results12/*.part $T/results13/*.part
cp -r $T/results11 $T/results12 $T/results13 $E/
cp $T/campaign12.sh $T/campaign13.sh $T/campaign14.sh $T/altsumm11.py $T/finalsumm.py $T/commenttable.py $T/comment_assemble.py $T/comment_body.md $T/saveresults2.sh $E/
python3 $T/finalsumm.py > $E/results13/SUMMARY.txt
python3 $T/altsumm11.py > $E/results11/SUMMARY.txt
echo saved
