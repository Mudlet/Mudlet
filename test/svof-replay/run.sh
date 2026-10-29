#!/bin/bash
# One run of the svof replay benchmark: a private HOME with svof installed into a fresh profile, a
# log server replaying an Achaea session over loopback, Mudlet on a private Xvfb. Its inputs all come
# from SVOF_BENCH, a SvofBench.mpackage built by make-package.lua. See README.md.
# usage: SVOF_BENCH=<SvofBench.mpackage> run.sh <label> <mudlet-exe> [timeout-s=400]
set -u
P="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ $# -lt 2 ] || [[ "$1" == */* ]] || [ -z "$1" ]; then
    echo "usage: SVOF_BENCH=<SvofBench.mpackage> run.sh <label> <mudlet-exe> [timeout-s=400]" >&2
    exit 2
fi
if [ ! -f "${SVOF_BENCH:-}" ]; then
    echo "run.sh: set SVOF_BENCH to a SvofBench.mpackage (see README.md)" >&2
    exit 2
fi

# One benchmark at a time on this machine, whoever started it
lock=${PERF_BENCH_LOCK:-$HOME/.cache/perf-bench.lock}
if [ -z "${PERF_BENCH_LOCKED:-}" ]; then
    mkdir -p "$(dirname "$lock")"
    exec 9>"$lock"
    if ! flock -w "${PERF_BENCH_LOCK_WAIT:-1800}" 9; then
        echo "run.sh: lock $lock still held" >&2
        exit 75
    fi
    export PERF_BENCH_LOCKED=1
fi

label=$1
exe=$(readlink -f "$2")
timeout=${3:-400}

# Unpacked once per package, by its checksum
bench=$(readlink -f "$SVOF_BENCH")
cache=${SVOF_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/svof-replay}/$(md5sum < "$bench" | cut -c1-32)
if [ ! -f "$cache/manifest.lua" ]; then
    rm -rf "$cache.tmp"
    mkdir -p "$cache.tmp"
    if ! unzip -q "$bench" -d "$cache.tmp"; then
        echo "run.sh: cannot unpack $bench" >&2
        exit 2
    fi
    rm -rf "$cache"
    mv "$cache.tmp" "$cache"
fi
read -r corpus want < <(lua5.1 -e "local m = dofile([==[$cache/manifest.lua]==]) print(m.corpus, m.md5)")
corpus="$cache/${corpus:-}"
if [ -z "${want:-}" ] || [ ! -f "$corpus" ]; then
    echo "run.sh: cannot read the corpus named in $cache/manifest.lua" >&2
    exit 2
fi
if [ "$(md5sum < "$corpus" | cut -c1-32)" != "$want" ]; then
    echo "run.sh: $corpus does not match the package's manifest" >&2
    exit 2
fi

runs=${SVOF_RUNS:-${TMPDIR:-/tmp}/svof-replay}
run="$runs/$label"
rm -rf "$run"
prof="$run/home/.config/mudlet/profiles/SvofBench"
mkdir -p "$prof/current"

for _ in 1 2 3 4 5 6 7 8 9 10; do
    port=$((5500 + RANDOM % 100))
    ss -ltn 2>/dev/null | grep -q ":$port " || break
done
for _ in 1 2 3 4 5 6 7 8 9 10; do
    disp=$((100 + RANDOM % 800))
    [ -e "/tmp/.X${disp}-lock" ] || break
done

printf '[General]\ntelnetHandlerAsked=true\ntelnetHandlerEnabled=false\ntelnetHandlerDontAsk=true\n' > "$run/home/.config/mudlet/Mudlet.ini"
printf '%s' "$port" > "$prof/port"
printf '127.0.0.1' > "$prof/url"
result="$run/result.txt"
cat > "$prof/current/2030-01-01#00-00-00.xml" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE MudletPackage>
<MudletPackage version="1.001">
	<ScriptPackage>
		<Script isActive="yes" isFolder="no">
			<name>bench</name>
			<packageName></packageName>
			<script>_G.BENCH_CFG = { mode = "socket", bundle = [==[$cache]==], result = [==[$result]==], label = [==[$label]==], port = $port, settle = ${SVOF_SETTLE:-4.0} }
dofile([==[$P/bench.lua]==])</script>
			<eventHandlerList />
		</Script>
	</ScriptPackage>
</MudletPackage>
EOF

mpid= srv= xpid=
trap 'kill $mpid $srv $xpid 2>/dev/null' EXIT
trap 'exit 1' INT TERM
# Give a process up to $2 half-seconds to exit by itself, then kill it
stop() {
    for ((i = 0; i < $2; i++)); do
        kill -0 "$1" 2>/dev/null || break
        sleep 0.5
    done
    kill "$1" 2>/dev/null
    wait "$1" 2>/dev/null
}

t0=$(date +%s.%N)
if [ "${SVOF_QPA:-xcb}" = offscreen ]; then
    qpa=(QT_QPA_PLATFORM=offscreen)
else
    Xvfb ":$disp" -screen 0 1600x1000x24 -nolisten tcp > "$run/xvfb.log" 2>&1 9>&- &
    xpid=$!
    qpa=(QT_QPA_PLATFORM=xcb DISPLAY=":$disp")
fi
lua5.1 "$P/logserver.lua" "$corpus" "$port" "$run/logserver.log" 9>&- &
srv=$!
sleep 0.7
# Keep Mudlet off the desktop's Wayland session, where it does not paint and the run looks faster
env -u WAYLAND_DISPLAY HOME="$run/home" XDG_CONFIG_HOME="$run/home/.config" XDG_DATA_HOME="$run/home/.local/share" \
    XDG_CACHE_HOME="$run/home/.cache" "${qpa[@]}" LD_PRELOAD="${MUDLET_PRELOAD:-}" \
    "$exe" --profile=SvofBench > "$run/mudlet.out" 2>&1 9>&- &
mpid=$!

for ((i = 0; i < timeout * 2; i++)); do
    [ -f "$result" ] && break
    kill -0 $mpid 2>/dev/null || break
    sleep 0.5
done
hwm=$(awk '/^VmHWM/{print $2}' /proc/$mpid/status 2>/dev/null)
stop "$mpid" 10
mpid=
# The log server writes CLIENT_LINES once it sees Mudlet's connection close
stop "$srv" 6
srv=
[ -n "$xpid" ] && stop "$xpid" 0
xpid=
t1=$(date +%s.%N)

if [ ! -f "$result" ]; then
    echo "NO RESULT after ${timeout}s"
    tail -5 "$run/mudlet.out"
    cat "$result.dbg" 2>/dev/null
    exit 1
fi
cat "$result"
echo "client_sends $(awk '/CLIENT_LINES/{print $3}' "$run/logserver.log")"
echo "maxrss_kb ${hwm:-unknown}"
echo "wall_s $(echo "$t1 - $t0" | bc -l | cut -c1-6)"
if ! grep -qx "gate pass" "$result"; then
    echo "run.sh: FAIL, $(grep '^gate ' "$result")" >&2
    cat "$result.dbg" >&2 2>/dev/null
    exit 1
fi
