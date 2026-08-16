#!/bin/bash
# Runs the busted Lua specs the way CI's "(Linux) Run Lua tests" step does:
# starts the HTTP, Discord IPC and MMCP peer fixtures, then drives the built
# Mudlet through the "Mudlet self-test" profile under xvfb. --offline stops the
# profile connecting to its game server, which is also what leaves the telnet
# socket in the state feedTelnet() needs.
#
# Usage: .claude/scripts/run-lua-tests.sh [path-to-mudlet-binary]
# Defaults to the linux-debug-nosan build. Needs the rocks and apt packages
# that .claude/hooks/session-start.sh installs.
#
# Safe to run concurrently (e.g. one run per worktree): every fixture binds an
# ephemeral port handed over through this run's private temp directory, only
# this run's fixture processes are cleaned up, and the profile tree lives in a
# per-run HOME so simultaneous Mudlets never share saved state.
set -euo pipefail

WS="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${1:-$WS/build-linux-debug-nosan/src/mudlet}"
TMP="$(mktemp -d /tmp/mudlet-luatests-XXXX)"

[ -x "$BINARY" ] || { echo "no mudlet binary at $BINARY - build first"; exit 1; }

# The binary is run in place; it locates mudlet-lua relative to itself, which
# works for any build tree inside the repository (build*/src/mudlet).
BINDIR="$(cd "$(dirname "$BINARY")" && pwd)"
if [ ! -f "$BINDIR/../../src/mudlet-lua/lua/LuaGlobal.lua" ] && [ ! -f "$BINDIR/mudlet-lua/lua/LuaGlobal.lua" ]; then
  echo "LuaGlobal.lua is not reachable relative to $BINARY - run a build tree that sits inside the repository"
  exit 1
fi

FIXTURE_PIDS=()
cleanup() {
  for pid in "${FIXTURE_PIDS[@]:-}"; do
    kill "$pid" 2>/dev/null || true
  done
}
trap cleanup EXIT

# 1. HTTP fixture server
port_file="$TMP/http-port"
MUDLET_TEST_HTTP_PORT_FILE="$port_file" nohup python3 "$WS/CI/http-fixture-server.py" > "$TMP/http.log" 2>&1 &
FIXTURE_PIDS+=($!)
for _ in $(seq 1 50); do [ -s "$port_file" ] && break; sleep 0.1; done
[ -s "$port_file" ] || { echo "http fixture failed"; cat "$TMP/http.log"; exit 1; }
HTTP_PORT="$(cat "$port_file")"
curl -fsS "http://127.0.0.1:${HTTP_PORT}/fixture.txt" > /dev/null

# 2. fake Discord IPC server. A short runtime dir of its own: the whole
# discord-ipc-0 socket path has to fit into sockaddr_un's 108-character
# sun_path, so it cannot live under $TMP.
runtime_dir="$(mktemp -d /tmp/mdxdg-XXXX)"
ready_file="$TMP/discord-ready"
nohup python3 "$WS/CI/discord-ipc-fixture.py" \
  --runtime-dir "$runtime_dir" \
  --capture-file "$TMP/discord-frames.jsonl" \
  --ready-file "$ready_file" > "$TMP/discord.log" 2>&1 &
FIXTURE_PIDS+=($!)
for _ in $(seq 1 50); do [ -s "$ready_file" ] && break; sleep 0.1; done
[ -s "$ready_file" ] && [ -S "$runtime_dir/discord-ipc-0" ] || { echo "discord fixture failed"; cat "$TMP/discord.log"; exit 1; }
set -a; source "$ready_file"; set +a

# 3. MMCP peer fixture (ephemeral port, so parallel runs never collide)
peer_dir="$TMP/mmcp-peer"
mkdir -p "$peer_dir"
MUDLET_TEST_MMCP_DIR="$peer_dir" nohup python3 "$WS/CI/mmcp-peer.py" > "$TMP/mmcp.log" 2>&1 &
FIXTURE_PIDS+=($!)
for _ in $(seq 1 50); do [ -s "$peer_dir/port" ] && break; sleep 0.1; done
[ -s "$peer_dir/port" ] || { echo "mmcp fixture failed"; cat "$TMP/mmcp.log"; exit 1; }

# absolute rock paths, resolved against the real HOME before it is replaced
eval "$(luarocks path --local --lua-version 5.1)"
export LUA_PATH LUA_CPATH

# A private HOME gives this run its own "Mudlet self-test" profile tree, so
# concurrent runs (and leftovers from aborted ones) never share saved state -
# the same clean slate a fresh CI runner provides.
export HOME="$TMP/home"
mkdir -p "$HOME"

export AUTORUN_BUSTED_TESTS=true
export MUDLET_TEST_MODE=1
export MUDLET_TEST_REQUIRE_TTS_MOCK=1
export MUDLET_TEST_REQUIRE_HTTP_FIXTURE=1
export MUDLET_TEST_REQUIRE_MMCP_PEER=1
export MUDLET_TEST_REQUIRE_MEDIA=1
export MUDLET_TEST_REQUIRE_DISCORD=1
export MUDLET_TEST_HTTP_PORT="$HTTP_PORT"
export MUDLET_TEST_MMCP_DIR="$peer_dir"
export XDG_RUNTIME_DIR="${MUDLET_TEST_DISCORD_RUNTIME_DIR:-$runtime_dir}"
export LD_LIBRARY_PATH="$WS/3rdparty/discord/rpc/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DBUS_SESSION_BUS_ADDRESS='disabled:'
export TESTS_DIRECTORY="$WS/src/mudlet-lua/tests"
export QUIT_MUDLET_AFTER_TESTS=true

# Mudlet exits 0 whatever the specs did, so a failing run is only visible
# through the marker file busted writes - the same signal CI's separate
# "Passed Lua tests" step checks. Keep it in this run's temp directory so
# concurrent runs cannot read each other's.
export MUDLET_TEST_FAILURE_MARKER="$TMP/busted-tests-failed"

cd "$WS"
rc=0
timeout 360 xvfb-run --auto-servernum "$BINARY" --profile "Mudlet self-test" --mirror --offline || rc=$?
if [ -e "$MUDLET_TEST_FAILURE_MARKER" ]; then
  echo "Lua tests failed - see the busted output above."
  rc=1
fi
exit $rc
