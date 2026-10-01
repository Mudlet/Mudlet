#!/bin/bash
# final mode 8 with the measured tightness rule (MUDLET_GEO_TIGHT=0.9) on every map
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results13
mkdir -p $R
source <(sed -n '/^busy()/,/^}/p;/^gate()/,/^}/p;/^run()/,/^}/p' $T/campaign12.sh)
G="MUDLET_SKIP_SCALE_PASSES=1 MUDLET_GEO=1 MUDLET_GEO_TIGHT=0.9 MUDLET_ALT_K=8"
MAPS="sendar.dat achaea.dat aetolia.dat imperian.dat lusternia.dat starmourn.dat"
for f in $T/maps/extra/*.dat $T/maps/extra/*.json; do
  b=$(basename "$f")
  [ "$b" = torilmud.dat ] && continue
  MAPS="$MAPS extra/$b"
done
for map in $MAPS; do
  m=$(basename "${map%.*}")
  run main-$m $map verifyHeuristics $G MUDLET_BENCH_MODES=0,8 MUDLET_BENCH_PAIRS=300 MUDLET_BENCH_REPS=5
  run audit-$m $map auditHeuristic $G MUDLET_BENCH_MODES=0,8 MUDLET_AUDIT_GOALS=30
  run mapping-$m $map mappingLoop $G MUDLET_MAPPING_MODE=8 MUDLET_MAPPING_CYCLES=10
done
run main-aetherspace aetherspace.dat verifyHeuristics $G MUDLET_BENCH_MODES=0,8 MUDLET_BENCH_PAIRS=100 MUDLET_BENCH_REPS=3 MUDLET_BENCH_SCENARIOS=1
run teleport-aetherspace aetherspace.dat verifyHeuristics $G MUDLET_BENCH_MODES=0,8 MUDLET_BENCH_PAIRS=50 MUDLET_BENCH_REPS=3 MUDLET_BENCH_MUTATE=teleport
run audit-aetherspace aetherspace.dat auditHeuristic $G MUDLET_BENCH_MODES=0,8 MUDLET_AUDIT_GOALS=3
run audit-aetherspace-teleport aetherspace.dat auditHeuristic $G MUDLET_BENCH_MODES=0,8 MUDLET_AUDIT_GOALS=3 MUDLET_BENCH_MUTATE=teleport
run mapping-aetherspace aetherspace.dat mappingLoop $G MUDLET_MAPPING_MODE=8 MUDLET_MAPPING_CYCLES=4
echo done > $R/ALLDONE
