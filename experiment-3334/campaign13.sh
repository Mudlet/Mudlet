#!/bin/bash
# Procedural Realms: does the landmark skip cause the regression?
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results12
mkdir -p $R
source <(sed -n '/^busy()/,/^}/p;/^gate()/,/^}/p;/^run()/,/^}/p' $T/campaign12.sh)
B="MUDLET_SKIP_SCALE_PASSES=1 MUDLET_GEO=1 MUDLET_ALT_K=8 MUDLET_BENCH_MODES=0,6,8 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5"
run pr-noskip extra/proceduralrealms.dat verifyHeuristics $B
run pr-skip1 extra/proceduralrealms.dat verifyHeuristics $B MUDLET_GEO_SKIP=1.0
run pr-skip05 extra/proceduralrealms.dat verifyHeuristics $B MUDLET_GEO_SKIP=0.5
run pr-mapping-noskip extra/proceduralrealms.dat mappingLoop MUDLET_SKIP_SCALE_PASSES=1 MUDLET_GEO=1 MUDLET_ALT_K=8 MUDLET_MAPPING_MODE=8 MUDLET_MAPPING_CYCLES=10
echo done > $R/ALLDONE
