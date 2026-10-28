#!/bin/bash
# Temporary: launch a Mudlet bundle, report whether it survives startup, and backtrace it if it crashes.
# usage: run-mudlet.sh <label> <Mudlet.app> [shim.dylib]
label="$1"; app="$2"; shim="$3"
binary="$app/Contents/MacOS/mudlet"
envs=(CI=1 MallocScribble=1)
[ -n "$shim" ] && envs+=(DYLD_INSERT_LIBRARIES="$shim")
echo "::group::$label - launch"
log=$(mktemp)
env "${envs[@]}" "$binary" > "$log" 2>&1 &
pid=$!
alive=no
for _ in $(seq 1 25); do
  sleep 1
  kill -0 "$pid" 2>/dev/null || break
done
if kill -0 "$pid" 2>/dev/null; then
  alive=yes
  kill "$pid"; sleep 2; kill -9 "$pid" 2>/dev/null
  wait "$pid" 2>/dev/null
  code="still running after 25s"
else
  wait "$pid"; code="exited with $?"
fi
grep -v '^$' "$log" | tail -25
echo "::endgroup::"
echo "RESULT|$label|$code"
if [ "$alive" = no ]; then
  echo "::group::$label - lldb backtrace"
  lldb_cmds=(-o "settings set target.env-vars ${envs[*]}" -o "run" -k "thread backtrace -c 24" -k "kill" -o "quit")
  perl -e 'alarm shift; exec @ARGV' 90 lldb --batch "${lldb_cmds[@]}" -- "$binary" 2>&1 | grep -E "stop reason|frame #|uncached_colorspace|error" | head -40
  echo "::endgroup::"
fi
