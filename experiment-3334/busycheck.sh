#!/bin/bash
read -r _ a b c d e f g _ < /proc/stat; t1=$((a+b+c+d+e+f+g)); i1=$((d+e)); sleep 3
read -r _ a b c d e f g _ < /proc/stat; t2=$((a+b+c+d+e+f+g)); i2=$((d+e))
echo "total_delta=$((t2-t1)) idle_delta=$((i2-i1)) busy=$(( 100 - 100*(i2-i1)/(t2-t1) ))"; cat /proc/loadavg
