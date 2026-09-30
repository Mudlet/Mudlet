#!/bin/bash
# mapping loop without the experiment-only scale passes, modes interleaved
T=/home/vadi/.claude/jobs/3ae74264/tmp
R=$T/results9
mkdir -p $R
busy() {
  read -r _ a b c d e f g _ < /proc/stat; t1=$((a+b+c+d+e+f+g)); i1=$((d+e)); sleep 3
  read -r _ a b c d e f g _ < /proc/stat; t2=$((a+b+c+d+e+f+g)); i2=$((d+e))
  echo $(( 100 - 100*(i2-i1)/(t2-t1) ))
}
gate() {
  local waited=0 quiet=0
  while [ $waited -lt 3600 ]; do
    if [ "$(busy)" -lt 15 ]; then quiet=$((quiet+1)); else quiet=0; fi
    [ $quiet -ge 2 ] && { echo "METRIC gate_waited_s $waited"; return; }
    sleep 10; waited=$((waited+13))
  done
  echo "METRIC gate_gave_up 1"
}
run() {
  local name=$1 map=$2 fn=$3; shift 3
  [ -s $R/$name.txt ] && return
  { gate; env "$@" $T/runbench2.sh $map $fn; } > $R/$name.txt.part && mv $R/$name.txt.part $R/$name.txt
}
SMALL="sendar achaea aetolia imperian lusternia starmourn"
for K in 8 4; do
  for m in $SMALL; do
    run mapping-k$K-$m $m.dat mappingLoop MUDLET_SKIP_SCALE_PASSES=1 MUDLET_ALT_K=$K MUDLET_MAPPING_CYCLES=10
  done
  run mapping-k$K-aetherspace aetherspace.dat mappingLoop MUDLET_SKIP_SCALE_PASSES=1 MUDLET_ALT_K=$K MUDLET_MAPPING_CYCLES=4
done
echo done > $R/ALLDONE
