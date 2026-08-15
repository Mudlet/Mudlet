#!/bin/bash
# Runs the busted Lua specs the way CI's "(Linux) Run Lua tests" step does:
# starts the HTTP, Discord IPC and MMCP peer fixtures, then drives the built
# Mudlet through the "Mudlet self-test" profile under xvfb.
#
# Usage: .claude/scripts/run-lua-tests.sh [path-to-mudlet-binary]
# Defaults to the linux-debug-nosan build. Needs the rocks and apt packages
# that .claude/hooks/session-start.sh installs.
set -euo pipefail

WS="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${1:-$WS/build-linux-debug-nosan/src/mudlet}"
TMP="$(mktemp -d /tmp/mudlet-luatests-XXXX)"

[ -x "$BINARY" ] || { echo "no mudlet binary at $BINARY - build first"; exit 1; }

# fixtures from a previous run would answer on stale ports and fail the MMCP
# specs with peers the tests never scripted
pkill -f 'CI/http-fixture-server.py' 2>/dev/null || true
pkill -f 'CI/discord-ipc-fixture.py' 2>/dev/null || true
pkill -f 'CI/mmcp-peer.py' 2>/dev/null || true
sleep 1

cleanup() {
  pkill -f 'CI/http-fixture-server.py' 2>/dev/null || true
  pkill -f 'CI/discord-ipc-fixture.py' 2>/dev/null || true
  pkill -f 'CI/mmcp-peer.py' 2>/dev/null || true
}
trap cleanup EXIT

# the binary must sit in src/ so LuaGlobal.lua loads relative to it
cp "$BINARY" "$WS/src/mudlet"

port_file="$TMP/http-port"
MUDLET_TEST_HTTP_PORT_FILE="$port_file" nohup python3 "$WS/CI/http-fixture-server.py" > "$TMP/http.log" 2>&1 &
for _ in $(seq 1 50); do [ -s "$port_file" ] && break; sleep 0.1; done
[ -s "$port_file" ] || { echo "http fixture failed"; cat "$TMP/http.log"; exit 1; }
HTTP_PORT="$(cat "$port_file")"
curl -fsS "http://127.0.0.1:${HTTP_PORT}/fixture.txt" > /dev/null

# short runtime dir of its own: the discord-ipc-0 socket path has to fit into
# sockaddr_un's 108-character sun_path
runtime_dir="$(mktemp -d /tmp/mdxdg-XXXX)"
ready_file="$TMP/discord-ready"
nohup python3 "$WS/CI/discord-ipc-fixture.py" \
  --runtime-dir "$runtime_dir" \
  --capture-file "$TMP/discord-frames.jsonl" \
  --ready-file "$ready_file" > "$TMP/discord.log" 2>&1 &
for _ in $(seq 1 50); do [ -s "$ready_file" ] && break; sleep 0.1; done
[ -s "$ready_file" ] && [ -S "$runtime_dir/discord-ipc-0" ] || { echo "discord fixture failed"; cat "$TMP/discord.log"; exit 1; }
set -a; source "$ready_file"; set +a

peer_dir="$TMP/mmcp-peer"
mkdir -p "$peer_dir"
MUDLET_TEST_MMCP_DIR="$peer_dir" nohup python3 "$WS/CI/mmcp-peer.py" > "$TMP/mmcp.log" 2>&1 &
for _ in $(seq 1 50); do [ -s "$peer_dir/port" ] && break; sleep 0.1; done
[ -s "$peer_dir/port" ] || { echo "mmcp fixture failed"; cat "$TMP/mmcp.log"; exit 1; }

eval "$(luarocks path --local --lua-version 5.1)"
export LUA_PATH LUA_CPATH

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

cd "$WS"
rc=0
timeout 360 xvfb-run --auto-servernum "$WS/src/mudlet" --profile "Mudlet self-test" --mirror || rc=$?
exit $rc
