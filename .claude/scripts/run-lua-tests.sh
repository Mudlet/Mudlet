#!/bin/bash
# Runs the busted Lua specs the way CI's "(Linux) Run Lua tests" step does:
# starts the HTTP, Discord IPC, MMCP peer and telnet fixtures, then drives the
# built Mudlet through the "Mudlet self-test" profile under xvfb. --offline stops
# the profile connecting to its game server, which is also what leaves the telnet
# socket in the state feedTelnet() needs; the specs that do need a connection open
# one to the telnet fixture themselves.
#
# Usage: .claude/scripts/run-lua-tests.sh [path-to-mudlet-binary]
# Defaults to the linux-debug-nosan build. A binary built in another worktree
# works too: src/mudlet-lua is read from disk at startup, so a change confined
# to it needs no build of its own. Only the Lua is this worktree's - the C++,
# and everything compiled into the binary's Qt resources (src/packages/*, and
# mudlet-lua/lua/utf8_filenames.lua), still comes from whoever built it. Needs
# the rocks and apt packages that .claude/hooks/session-start.sh installs.
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
BINARY="$(cd "$(dirname "$BINARY")" && pwd)/$(basename "$BINARY")"

WS_GLOBAL="$WS/src/mudlet-lua/lua/LuaGlobal.lua"
[ -f "$WS_GLOBAL" ] || { echo "no mudlet-lua under $WS - is this a Mudlet checkout?"; exit 1; }

# Mudlet locates mudlet-lua relative to its own executable and takes the first
# candidate that exists, so a binary borrowed from another build tree would
# quietly run that tree's Lua library against this worktree's specs - green
# whatever the change under test did. Detect that and shim around it, rather
# than making the caller build here.
BINDIR="$(cd "$(dirname "$BINARY")" && pwd)"
loads_this_worktree() {
  local candidate
  # The Linux subset of loadGlobal()'s candidates, in its order. Omitted: the
  # macOS-only ../Resources entry, which would come first there, and the two
  # absolute tail entries LUA_DEFAULT_PATH and LUA_SOURCE_PATH. They sort after
  # these, so they cannot make this misjudge a Linux binary - but LUA_SOURCE_PATH
  # is the donor's own source tree, baked in at compile time, so it stays a live
  # fallback for the whole run. Hence the post-run check at the bottom.
  for candidate in "$BINDIR/mudlet-lua/lua/LuaGlobal.lua" \
                   "$BINDIR/../src/mudlet-lua/lua/LuaGlobal.lua" \
                   "$BINDIR/../../src/mudlet-lua/lua/LuaGlobal.lua" \
                   "$BINDIR/../../../src/mudlet-lua/lua/LuaGlobal.lua" \
                   "$BINDIR/../../../mudlet-lua/lua/LuaGlobal.lua"; do
    [ -f "$candidate" ] || continue
    # only the first existing candidate is ever loaded, so this one decides it
    [ "$(readlink -f "$candidate")" = "$(readlink -f "$WS_GLOBAL")" ]
    return
  done
  return 1
}

if ! loads_this_worktree; then
  echo "note: $BINARY was built elsewhere - running its C++ against this worktree's"
  echo "      Lua. A C++ change in this worktree is NOT under test."
  mkdir -p "$TMP/shim/build/src"
  # A hardlink rather than a symlink: Qt 6.12 derives applicationDirPath() from
  # argv[0], under which either would work, but a hardlink is also correct if it
  # ever goes back to resolving /proc/self/exe. readlink -f first because the
  # argument may itself be a symlink: hard-linking one stages a link straight back
  # into the donor tree, which voids that hedge and disagrees with the cp -p
  # fallback, since cp dereferences. -p because a bare cp applies the umask and can
  # drop the execute bit. Copy only when the donor is on another filesystem, where
  # a hardlink is impossible.
  ln "$(readlink -f "$BINARY")" "$TMP/shim/build/src/mudlet" 2>/dev/null \
    || cp -p "$BINARY" "$TMP/shim/build/src/mudlet" \
    || { echo "could not stage $BINARY into $TMP - out of space, or /tmp not writable"; exit 2; }
  ln -s "$WS/src" "$TMP/shim/src" || { echo "could not link $WS/src into $TMP"; exit 2; }
  BINARY="$TMP/shim/build/src/mudlet"
  BINDIR="$(cd "$(dirname "$BINARY")" && pwd)"
  loads_this_worktree || { echo "shim did not take: $BINARY still would not load $WS/src/mudlet-lua"; exit 2; }
  [ -x "$BINARY" ] || { echo "staged $BINARY is not executable"; exit 2; }
fi

FIXTURE_PIDS=()
cleanup() {
  for pid in "${FIXTURE_PIDS[@]:-}"; do
    kill "$pid" 2>/dev/null || true
  done
  # Nothing here is worth keeping: the fixture logs are cat'd before any early
  # exit and the run itself is tee'd to stdout. Leaving it behind costs real
  # disk - a shimmed run parks a ~250MB hardlink that also pins the donor's
  # inode, so a deleted worktree would stop reclaiming its build.
  rm -rf "$TMP" "${runtime_dir:-}"
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

# 4. silent recording game server (ephemeral port); never negotiates, so specs
# see the connected-but-unnegotiated state.
telnet_dir="$TMP/telnet-server"
mkdir -p "$telnet_dir"
MUDLET_TEST_TELNET_DIR="$telnet_dir" nohup python3 "$WS/CI/telnet-fixture-server.py" > "$TMP/telnet.log" 2>&1 &
FIXTURE_PIDS+=($!)
for _ in $(seq 1 50); do [ -s "$telnet_dir/port" ] && break; sleep 0.1; done
[ -s "$telnet_dir/port" ] || { echo "telnet fixture failed"; cat "$TMP/telnet.log"; exit 1; }

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
export MUDLET_TEST_REQUIRE_TELNET_FIXTURE=1
export MUDLET_TEST_REQUIRE_MEDIA=1
export MUDLET_TEST_REQUIRE_DISCORD=1
export MUDLET_TEST_HTTP_PORT="$HTTP_PORT"
export MUDLET_TEST_MMCP_DIR="$peer_dir"
export MUDLET_TEST_TELNET_DIR="$telnet_dir"
export XDG_RUNTIME_DIR="${MUDLET_TEST_DISCORD_RUNTIME_DIR:-$runtime_dir}"
export LD_LIBRARY_PATH="$WS/3rdparty/discord/rpc/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DBUS_SESSION_BUS_ADDRESS='disabled:'
export TESTS_DIRECTORY="${TESTS_DIRECTORY:-$WS/src/mudlet-lua/tests}"
export QUIT_MUDLET_AFTER_TESTS=true

# Mudlet exits 0 whatever the specs did, so a failing run is only visible
# through the marker file busted writes - the same signal CI's separate
# "Passed Lua tests" step checks. Keep it in this run's temp directory so
# concurrent runs cannot read each other's.
export MUDLET_TEST_FAILURE_MARKER="$TMP/busted-tests-failed"

# Names the library this run is meant to be exercising, so MudletBusted_spec.lua
# can check that it is the one loadGlobal() settled on rather than the donor's.
export MUDLET_TEST_EXPECTED_LUA_PATH="$(readlink -f "$WS/src/mudlet-lua/lua")"

cd "$WS"
rc=0
timeout 360 xvfb-run --auto-servernum "$BINARY" --profile "Mudlet self-test" --mirror --offline 2>&1 \
  | tee "$TMP/run.log" || rc=$?

# loadGlobal() walks on to its next candidate when one fails to run, so a syntax
# error anywhere in this worktree's mudlet-lua silently hands the whole library
# over to the donor's LUA_SOURCE_PATH and the suite passes against that instead.
# The warning it emits on the way past names the file and the Lua error, so treat
# it as fatal. MudletBusted_spec.lua backs this up with a positive check.
if grep -q "loadGlobal() loading" "$TMP/run.log"; then
  echo "This worktree's mudlet-lua failed to load, so the specs ran against the"
  echo "binary's own copy - the result above is meaningless. The failure was:"
  grep "loadGlobal() loading" "$TMP/run.log"
  rc=1
fi

if [ -e "$MUDLET_TEST_FAILURE_MARKER" ]; then
  echo "Lua tests failed - see the busted output above."
  rc=1
fi
exit $rc
