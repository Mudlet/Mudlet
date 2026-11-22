#!/usr/bin/env bash
# Runs Mudlet's busted specs in a headless, throwaway profile, or runs the same specs or a recorded
# game session against the merge base and this checkout and reports what behaves differently.
# Linux, macOS and Windows (an MSYS2 CLANG64 shell). Usage and workflow: ../SKILL.md, or --help.
# Written for bash 3.2, the one macOS ships.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
WS="$(cd "$HERE/../../../.." && pwd)"

usage() {
  cat <<'EOF'
verify-mudlet.sh specs [options] [SPEC...]
    Run specs (default: all of src/mudlet-lua/tests) in a fresh headless profile.
    A SPEC is a file or directory, or a name inside src/mudlet-lua/tests. Files from
    anywhere work, so an ad hoc probe can be a *_spec.lua in a temp directory.

verify-mudlet.sh diff specs [options] [SPEC...]
    Run the same specs on the merge base and on this checkout; report every test whose
    outcome differs. Differences are re-run once on both sides to drop flaky ones.

verify-mudlet.sh diff replay [options] [--log FILE] [--package FILE]...
    Play a recorded game session into both builds and diff the console (text and
    colour), what scripts sent, the events raised and the Lua errors. Without --log it
    replays the public svof benchmark session with svof installed.

Options:
  --binary PATH        Mudlet binary for this checkout (default: this checkout's build)
  --timeout SECONDS    cap on each Mudlet run (default 900, or MUDLET_LUA_TESTS_TIMEOUT)
  --offscreen          use Qt's offscreen platform instead of Xvfb or the native one
  --out DIR            keep logs and results here (default: a new temp directory)
  --verbose            print the whole Mudlet log, not just the summary
diff options:
  --base REF           compare against REF (default: merge base with <remote>/development)
  --base-binary PATH   Mudlet binary built from the base, instead of building one
  --base-preset NAME   CMake preset for building the base (default: <platform>-debug[-nosan])
  --specs-from base|head  whose spec files both sides run (default: base for a whole-suite
                       run, head when SPECs are named, so a new spec shows it bites)
  --no-recheck         report spec differences without re-running them
replay options:
  --log FILE           raw session bytes, as a game sent them (telnet codes and all)
  --package FILE       install this package first (repeatable)
  --home-files DIR     copy DIR's contents into the profile's home directory first
  --setup FILE         Lua to run in the profile before the packages are installed
  --settle MS          with --package or --setup: let the profile run this long after (default 4000)
  --plain              compare console text only, without colours (faster)
  --no-noise-check     run each side once, not twice (faster; timing noise then shows as changes)

Exit status: 0 no failures/differences, 1 failures or differences, 2 harness problem (and a
replay that did not complete), 3 Mudlet stopped before the specs finished (diff specs: also a
timeout), 124 a run hit --timeout.
EOF
}

die() {
  echo "verify-mudlet: $*" >&2
  exit 2
}

note() {
  echo "verify-mudlet: $*" >&2
}

case "$(uname -s)" in
  Linux*) OS=linux ;;
  Darwin*) OS=mac ;;
  MINGW* | MSYS* | CYGWIN*) OS=windows ;;
  *) die "unsupported platform: $(uname -s)" ;;
esac

# A path a native Windows program (Mudlet, Windows python) can read; unchanged elsewhere
native() {
  if [ "$OS" = windows ]; then cygpath -m "$1"; else printf '%s\n' "$1"; fi
}

resolve() {
  if command -v realpath > /dev/null 2>&1; then
    realpath "$1"
  else
    (cd "$(dirname "$1")" && printf '%s/%s\n' "$(pwd -P)" "$(basename "$1")")
  fi
}

# MSYS2's ln -s quietly deep-copies, or fails without Developer Mode, so copy outright there
link_into() {
  if [ "$OS" = windows ]; then cp -R "$1" "$2"; else ln -s "$1" "$2"; fi
}

COMMAND=""
DIFF_KIND=""
BINARY=""
BASE_REF=""
BASE_BINARY=""
BASE_PRESET=""
SPECS_FROM=""
RECHECK=true
TIMEOUT="${MUDLET_LUA_TESTS_TIMEOUT:-900}"
OFFSCREEN=false
OUT=""
VERBOSE=false
REPLAY_LOG=""
REPLAY_PACKAGES=()
REPLAY_HOME=""
REPLAY_SETUP=""
REPLAY_SETTLE=4000
REPLAY_PLAIN=false
NOISE_CHECK=true
SPECS=()

[ $# -gt 0 ] || { usage; exit 2; }
case "$1" in
  -h | --help) usage; exit 0 ;;
  specs) COMMAND=specs; shift ;;
  diff)
    COMMAND="diff"
    shift
    case "${1:-}" in
      specs | replay) DIFF_KIND="$1"; shift ;;
      *) die "diff needs 'specs' or 'replay'" ;;
    esac
    ;;
  *) die "unknown command '$1' (try --help)" ;;
esac

need_value() {
  [ $# -ge 2 ] && [ -n "$2" ] || die "$1 needs a value"
}

while [ $# -gt 0 ]; do
  case "$1" in
    --binary) need_value "$@"; BINARY="$2"; shift 2 ;;
    --timeout) need_value "$@"; TIMEOUT="$2"; shift 2 ;;
    --offscreen) OFFSCREEN=true; shift ;;
    --out) need_value "$@"; OUT="$2"; shift 2 ;;
    --verbose) VERBOSE=true; shift ;;
    --base) need_value "$@"; BASE_REF="$2"; shift 2 ;;
    --base-binary) need_value "$@"; BASE_BINARY="$2"; shift 2 ;;
    --base-preset) need_value "$@"; BASE_PRESET="$2"; shift 2 ;;
    --specs-from) need_value "$@"; SPECS_FROM="$2"; shift 2 ;;
    --no-recheck) RECHECK=false; shift ;;
    --log) need_value "$@"; REPLAY_LOG="$2"; shift 2 ;;
    --package) need_value "$@"; REPLAY_PACKAGES+=("$2"); shift 2 ;;
    --home-files) need_value "$@"; REPLAY_HOME="$2"; shift 2 ;;
    --setup) need_value "$@"; REPLAY_SETUP="$2"; shift 2 ;;
    --settle) need_value "$@"; REPLAY_SETTLE="$2"; shift 2 ;;
    --plain) REPLAY_PLAIN=true; shift ;;
    --no-noise-check) NOISE_CHECK=false; shift ;;
    -h | --help) usage; exit 0 ;;
    --) shift; SPECS+=("$@"); break ;;
    -*) die "unknown option '$1' (try --help)" ;;
    *) SPECS+=("$1"); shift ;;
  esac
done

[[ "$TIMEOUT" =~ ^[1-9][0-9]*$ ]] || die "--timeout must be a whole number of seconds, not '$TIMEOUT'"
[[ "$REPLAY_SETTLE" =~ ^[0-9]+$ ]] || die "--settle must be a whole number of milliseconds"
case "$SPECS_FROM" in "" | base | head) ;; *) die "--specs-from takes base or head" ;; esac
if [ "$COMMAND" = diff ] && [ "$DIFF_KIND" = replay ] && [ ${#SPECS[@]} -gt 0 ]; then
  die "diff replay takes no SPEC arguments"
fi

PYTHON=python3
command -v "$PYTHON" > /dev/null 2>&1 || PYTHON=python
command -v "$PYTHON" > /dev/null 2>&1 || die "python3 is needed for the test fixtures"

# Resolved now: everything below may run from another directory
absolute() {
  [ -e "$1" ] || die "no such file: $1"
  resolve "$1"
}
[ -z "$BINARY" ] || BINARY="$(absolute "$BINARY")"
[ -z "$BASE_BINARY" ] || BASE_BINARY="$(absolute "$BASE_BINARY")"
[ -z "$REPLAY_LOG" ] || REPLAY_LOG="$(absolute "$REPLAY_LOG")"
[ -z "$REPLAY_HOME" ] || REPLAY_HOME="$(absolute "$REPLAY_HOME")"
[ -z "$REPLAY_SETUP" ] || REPLAY_SETUP="$(absolute "$REPLAY_SETUP")"
for i in ${REPLAY_PACKAGES[@]+"${!REPLAY_PACKAGES[@]}"}; do
  REPLAY_PACKAGES[i]="$(absolute "${REPLAY_PACKAGES[i]}")"
done
RESOLVED_SPECS=()
for spec in ${SPECS[@]+"${SPECS[@]}"}; do
  if [ -e "$spec" ]; then
    RESOLVED_SPECS+=("$(resolve "$spec")")
  else
    # a name inside the tests directory, looked up per side later
    RESOLVED_SPECS+=("@$spec")
  fi
done

WORK="$(mktemp -d "${TMPDIR:-/tmp}/mudlet-verify-XXXXXX")"
if [ -z "$OUT" ]; then
  OUT="$WORK/out"
fi
mkdir -p "$OUT"
OUT="$(resolve "$OUT")"
FIXTURE_PIDS=()

stop_fixtures() {
  local pid
  for pid in ${FIXTURE_PIDS[@]+"${FIXTURE_PIDS[@]}"}; do
    # a plain SIGTERM does not reach a native Windows process from MSYS2
    kill -9 "$pid" 2> /dev/null || true
  done
  FIXTURE_PIDS=()
}

cleanup() {
  stop_fixtures
  # keep the results when they live in WORK; the shims (a hardlinked or cloned binary) go
  case "$OUT" in
    "$WORK"/*) find "$WORK" -mindepth 1 -maxdepth 1 ! -name out -exec rm -rf {} + 2> /dev/null || true ;;
    *) rm -rf "$WORK" ;;
  esac
}
trap cleanup EXIT

default_binaries() { # checkout -> candidate binaries, best first
  local root="$1" dir
  case "$OS" in
    linux)
      for dir in build-linux-debug-nosan build build-linux-release; do
        printf '%s\n' "$root/$dir/src/mudlet"
      done
      ;;
    mac)
      for dir in build-macos-debug-nosan build build-macos-release; do
        printf '%s\n' "$root/$dir/src/mudlet.app/Contents/MacOS/mudlet"
      done
      ;;
    windows)
      for dir in build build-windows-release; do
        printf '%s\n' "$root/$dir/src/mudlet.exe"
      done
      printf '%s\n' "$root/build-${MSYSTEM:-CLANG64}/release/mudlet.exe"
      ;;
  esac
}

find_binary() { # checkout
  local candidate
  while IFS= read -r candidate; do
    if [ -x "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done < <(default_binaries "$1")
  return 1
}

preset_binary() { # checkout preset
  local dir="$1/build-$2"
  case "$2" in linux-debug | macos-debug | windows-debug) dir="$1/build" ;; esac
  case "$OS" in
    linux) printf '%s\n' "$dir/src/mudlet" ;;
    mac) printf '%s\n' "$dir/src/mudlet.app/Contents/MacOS/mudlet" ;;
    windows) printf '%s\n' "$dir/src/mudlet.exe" ;;
  esac
}

# The LuaGlobal.lua loadGlobal() should run for this binary: the first existing candidate. If that one
# fails to load Mudlet falls through to the next, or to its compiled-in LUA_SOURCE_PATH; run_mudlet checks.
loaded_lua_global() { # binary
  local bindir candidate
  bindir="$(cd "$(dirname "$1")" && pwd)"
  for candidate in "$bindir/../Resources/mudlet-lua/lua/LuaGlobal.lua" \
                   "$bindir/mudlet-lua/lua/LuaGlobal.lua" \
                   "$bindir/../src/mudlet-lua/lua/LuaGlobal.lua" \
                   "$bindir/../../src/mudlet-lua/lua/LuaGlobal.lua" \
                   "$bindir/../../../src/mudlet-lua/lua/LuaGlobal.lua" \
                   "$bindir/../../../mudlet-lua/lua/LuaGlobal.lua"; do
    case "$candidate" in */Resources/*) [ "$OS" = mac ] || continue ;; esac
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done
  return 1
}

# Whether binary would run the Lua library under root/src/mudlet-lua. A macOS bundle, or a Windows
# shim, carries its own copy, which counts when it matches.
runs_lua_of() { # binary root
  local loaded wanted
  loaded="$(loaded_lua_global "$1")" || return 1
  wanted="$2/src/mudlet-lua/lua/LuaGlobal.lua"
  [ "$(resolve "$loaded")" != "$(resolve "$wanted")" ] || return 0
  case "$loaded" in */Resources/*) ;; *) [ "$OS" = windows ] || return 1 ;; esac
  [ "$(tree_sum "$(dirname "$loaded")")" = "$(tree_sum "$(dirname "$wanted")")" ]
}

# cksum, not diff: MSYS2 does not install diffutils by default
tree_sum() { # dir
  (cd "$1" && find . -type f -exec cksum {} + | LC_ALL=C sort -k3)
}

# Stages binary so that it runs root's Lua: Mudlet finds mudlet-lua relative to its executable.
# Prints the staged binary.
shim_binary() { # binary root dest
  local binary="$1" root="$2" dest="$3" bindir bundle
  bindir="$(cd "$(dirname "$binary")" && pwd)"
  mkdir -p "$dest"
  case "$OS" in
    linux)
      mkdir -p "$dest/build/src"
      # a hardlink is instant and costs no space; -p so a copy keeps the execute bit
      ln "$(resolve "$binary")" "$dest/build/src/mudlet" 2> /dev/null \
        || cp -p "$binary" "$dest/build/src/mudlet" || die "could not stage $binary - out of space?"
      ln -s "$root/src" "$dest/src"
      printf '%s\n' "$dest/build/src/mudlet"
      ;;
    mac)
      bundle="$(cd "$bindir/../.." && pwd)"
      # -c clones on APFS, so the copy is instant and takes no space
      cp -cR "$bundle" "$dest/mudlet.app" 2> /dev/null || cp -R "$bundle" "$dest/mudlet.app"
      rm -rf "$dest/mudlet.app/Contents/Resources/mudlet-lua/lua" "$dest/mudlet.app/Contents/MacOS/portable.txt"
      mkdir -p "$dest/mudlet.app/Contents/Resources/mudlet-lua"
      cp -R "$root/src/mudlet-lua/lua" "$dest/mudlet.app/Contents/Resources/mudlet-lua/lua"
      printf '%s\n' "$dest/mudlet.app/Contents/MacOS/$(basename "$binary")"
      ;;
    windows)
      mkdir -p "$dest/build/src" "$dest/src/mudlet-lua"
      # the DLLs and Qt plugin folders a build directory may carry beside the executable
      cp "$binary" "$dest/build/src/"
      find "$bindir" -maxdepth 1 \( -name '*.dll' -o -name 'qt.conf' \) -exec cp {} "$dest/build/src/" \;
      local plugins
      for plugins in platforms styles imageformats iconengines tls multimedia networkinformation sqldrivers texttospeech; do
        [ -d "$bindir/$plugins" ] && cp -R "$bindir/$plugins" "$dest/build/src/"
      done
      cp -R "$root/src/mudlet-lua/lua" "$dest/src/mudlet-lua/lua"
      printf '%s\n' "$dest/build/src/$(basename "$binary")"
      ;;
  esac
}

# Prints a binary that runs root's Lua: binary itself, or a shim of it. A portable.txt beside the
# binary would point its config at that folder instead of the throwaway one, and the shim leaves it out.
binary_for_lua() { # binary root label
  local staged
  if runs_lua_of "$1" "$2" && [ ! -f "$(dirname "$1")/portable.txt" ]; then
    printf '%s\n' "$1"
    return 0
  fi
  staged="$(shim_binary "$1" "$2" "$WORK/shim-$3")"
  runs_lua_of "$staged" "$2" || die "could not make $1 run the Lua in $2/src/mudlet-lua"
  printf '%s\n' "$staged"
}

# busted only takes a directory, and a few specs reach their fixtures and src/packages relative to
# their own file, so the specs to run are linked into a tree of the same shape
stage_specs() { # stage root, then specs ("@name" = inside root's tests directory)
  local stage="$1" root="$2"
  shift 2
  local tests="$stage/src/mudlet-lua/tests" source="$root/src/mudlet-lua/tests" spec entry
  mkdir -p "$tests"
  link_into "$root/src/packages" "$stage/src/packages"
  if [ $# -eq 0 ]; then
    for entry in "$source"/*; do
      link_into "$entry" "$tests/"
    done
  else
    for spec in "$@"; do
      case "$spec" in @*) spec="$source/${spec#@}" ;; esac
      [ -e "$spec" ] || die "no spec ${spec#"$source"/} in $source"
      if [ -d "$spec" ]; then
        for entry in "$spec"/*; do
          [ -e "$tests/$(basename "$entry")" ] || link_into "$entry" "$tests/"
        done
      else
        [ ! -e "$tests/$(basename "$spec")" ] || die "two specs named $(basename "$spec")"
        link_into "$spec" "$tests/"
      fi
    done
    [ -e "$tests/fixtures" ] || link_into "$source/fixtures" "$tests/"
  fi
  cp "$HERE/results_hook.lua" "$tests/!verify_results_spec.lua"
}

wait_for_file() { # file
  local _
  for _ in $(seq 1 100); do
    [ -s "$1" ] && return 0
    sleep 0.1
  done
  return 1
}

start_fixture() { # log, then env assignments and the script
  local log="$1"
  shift
  env "$@" > "$log" 2>&1 &
  FIXTURE_PIDS+=($!)
  disown $! 2> /dev/null || true
}

# CI's set per platform: Linux runs all four, macOS has no Discord, Windows only the HTTP server
start_fixtures() { # run dir
  local run="$1"
  local port_file="$run/http-port"
  start_fixture "$run/http.log" MUDLET_TEST_HTTP_PORT_FILE="$(native "$port_file")" "$PYTHON" "$(native "$WS/CI/http-fixture-server.py")"
  wait_for_file "$port_file" || { cat "$run/http.log"; die "http fixture failed"; }
  HTTP_PORT="$(tr -d '\r\n' < "$port_file")"
  if command -v curl > /dev/null 2>&1; then
    curl -fsS --max-time 10 "http://127.0.0.1:${HTTP_PORT}/fixture.txt" > /dev/null || die "http fixture is not serving"
  fi

  if [ "$OS" = linux ]; then
    # sockaddr_un caps the socket path at 108 characters, so not under $run
    DISCORD_RUNTIME="$(mktemp -d /tmp/mdxdg-XXXXXX)"
    start_fixture "$run/discord.log" "$PYTHON" "$WS/CI/discord-ipc-fixture.py" --runtime-dir "$DISCORD_RUNTIME" \
      --capture-file "$run/discord-frames.jsonl" --ready-file "$run/discord-ready"
    wait_for_file "$run/discord-ready" && [ -S "$DISCORD_RUNTIME/discord-ipc-0" ] || { cat "$run/discord.log"; die "discord fixture failed"; }
  fi

  if [ "$OS" != windows ]; then
    mkdir -p "$run/mmcp-peer" "$run/telnet-server"
    start_fixture "$run/mmcp.log" MUDLET_TEST_MMCP_DIR="$run/mmcp-peer" "$PYTHON" "$WS/CI/mmcp-peer.py"
    wait_for_file "$run/mmcp-peer/port" || { cat "$run/mmcp.log"; die "mmcp fixture failed"; }
    start_fixture "$run/telnet.log" MUDLET_TEST_TELNET_DIR="$run/telnet-server" "$PYTHON" "$WS/CI/telnet-fixture-server.py"
    wait_for_file "$run/telnet-server/port" || { cat "$run/telnet.log"; die "telnet fixture failed"; }
  fi
}

# Lua rock paths, read once against the real HOME before a run replaces it
ROCK_ENV=()
if command -v luarocks > /dev/null 2>&1; then
  case "$OS" in
    linux)
      eval "$(luarocks path --local --lua-version 5.1)"
      ROCK_ENV=("LUA_PATH=$LUA_PATH" "LUA_CPATH=$LUA_CPATH")
      ;;
    mac)
      if paths="$(luarocks --lua-version 5.1 path 2> /dev/null)"; then
        eval "$paths"
        ROCK_ENV=("LUA_PATH=$LUA_PATH" "LUA_CPATH=$LUA_CPATH")
      fi
      ;;
    windows)
      ROCK_ENV=("LUA_PATH=$(luarocks --lua-version 5.1 path --lr-path)" "LUA_CPATH=$(luarocks --lua-version 5.1 path --lr-cpath)")
      ;;
  esac
fi

bounded() { # seconds log command...
  local seconds="$1" log="$2"
  shift 2
  local timer=""
  command -v timeout > /dev/null 2>&1 && timer=timeout
  [ -n "$timer" ] || { command -v gtimeout > /dev/null 2>&1 && timer=gtimeout; }
  if [ -n "$timer" ]; then
    "$timer" -k 10 "$seconds" "$@" > "$log" 2>&1
    return
  fi
  "$@" > "$log" 2>&1 &
  local pid=$! rc=0 started=$SECONDS
  # the sleeps run in the background so a TERM ends them too, instead of leaving them behind
  (
    trap 'kill "$nap" 2> /dev/null; exit 0' TERM
    sleep "$seconds" & nap=$!; wait "$nap"
    kill -TERM "$pid" 2> /dev/null
    sleep 10 & nap=$!; wait "$nap"
    kill -KILL "$pid" 2> /dev/null
  ) > /dev/null 2>&1 &
  local watcher=$!
  wait "$pid" || rc=$?
  kill "$watcher" 2> /dev/null || true
  wait "$watcher" 2> /dev/null || true
  if [ "$rc" -ne 0 ] && [ $((SECONDS - started)) -ge "$seconds" ]; then
    rc=124
  fi
  return "$rc"
}

# Runs one Mudlet over the staged specs in a profile of its own. Leaves $dir/log.txt,
# $dir/results.tsv (+ .details), and sets RUN_STATUS: passed, failed, crashed, timeout or broken.
run_mudlet() { # label binary stage-root [extra env...]
  local label="$1" binary="$2" stage="$3"
  shift 3
  local dir="$OUT/$label" run="$WORK/run-$label"
  rm -rf "$dir" "$run"
  mkdir -p "$dir" "$run/xdg/mudlet/profiles" "$run/home"

  local expected
  expected="$(dirname "$(loaded_lua_global "$binary")")"
  if [ "$OS" = mac ] && [ -f "$binary" ]; then
    case "$(dirname "$binary")" in */Contents/MacOS) ;; *) note "$binary is not inside an .app bundle; macOS runs may misbehave" ;; esac
  fi
  if [ -f "$(dirname "$binary")/portable.txt" ]; then
    die "$(dirname "$binary")/portable.txt redirects that Mudlet's config away from the throwaway profile; move it aside"
  fi

  start_fixtures "$run"
  local envs=(
    AUTORUN_BUSTED_TESTS=true
    MUDLET_TEST_MODE=1
    MUDLET_TEST_REQUIRE_TTS_MOCK=1
    MUDLET_TEST_REQUIRE_HTTP_FIXTURE=1
    "MUDLET_TEST_HTTP_PORT=$HTTP_PORT"
    QUIT_MUDLET_AFTER_TESTS=true
    "TESTS_DIRECTORY=$(native "$stage/src/mudlet-lua/tests")"
    "MUDLET_TEST_FAILURE_MARKER=$(native "$run/busted-tests-failed")"
    "MUDLET_VERIFY_RESULTS=$(native "$dir/results.tsv")"
    "MUDLET_TEST_EXPECTED_LUA_PATH=$(native "$(resolve "$expected")")"
    # a pre-created profiles folder is what makes Mudlet take XDG_CONFIG_HOME over an existing config
    "XDG_CONFIG_HOME=$(native "$run/xdg")"
    # TriggerFlood_spec pends unless the match pool is on, and its sample would take lines from the pool
    "MUDLET_MATCH_THREADS=${MUDLET_MATCH_THREADS:-2}"
    "MUDLET_MATCH_SAMPLE_EVERY=${MUDLET_MATCH_SAMPLE_EVERY:-0}"
  )
  envs+=(${ROCK_ENV[@]+"${ROCK_ENV[@]}"})
  if [ "$OS" != windows ]; then
    envs+=(MUDLET_TEST_REQUIRE_MMCP_PEER=1 MUDLET_TEST_REQUIRE_TELNET_FIXTURE=1
      "MUDLET_TEST_MMCP_DIR=$run/mmcp-peer" "MUDLET_TEST_TELNET_DIR=$run/telnet-server")
  fi
  local launcher=() capper=() unit=""
  if [ "$OS" = linux ]; then
    set -a
    # shellcheck disable=SC1091
    source "$run/discord-ready"
    set +a
    envs+=(
      MUDLET_TEST_REQUIRE_MEDIA=1
      MUDLET_TEST_REQUIRE_DISCORD=1
      "MUDLET_TEST_DISCORD_RUNTIME_DIR=$MUDLET_TEST_DISCORD_RUNTIME_DIR"
      "XDG_RUNTIME_DIR=$MUDLET_TEST_DISCORD_RUNTIME_DIR"
      # a private HOME as well, for anything that reads it rather than XDG_CONFIG_HOME
      "HOME=$run/home"
      "LD_LIBRARY_PATH=$WS/3rdparty/discord/rpc/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
      # no spec needs a session bus, and libdbus leaks the autolaunch reply when it looks for one
      "DBUS_SESSION_BUS_ADDRESS=disabled:"
      # CI's, for an AddressSanitizer build; a build without one ignores them
      "ASAN_OPTIONS=${ASAN_OPTIONS:-detect_leaks=1:intercept_tls_get_addr=0}"
      "LSAN_OPTIONS=${LSAN_OPTIONS:-suppressions=$WS/asan-suppressions.txt:exitcode=1:log_threads=1}"
    )
    local cap="${MUDLET_VERIFY_MEMORY_MAX:-16G}"
    if [ "$cap" != off ] && systemd-run --user --scope -q true > /dev/null 2>&1; then
      # a spec for a C++ fix the binary lacks can reproduce the bug, and some bugs eat all the RAM
      unit="mudlet-verify-$$-$label.scope"
      capper=(systemd-run --user --scope -q "--unit=$unit" -p "MemoryMax=$cap" -p MemorySwapMax=0)
    fi
    if [ "$OFFSCREEN" = false ] && command -v xvfb-run > /dev/null 2>&1; then
      # Xvfb is X11 only: on a Wayland desktop Qt and GTK must be told, or Qt's GTK3 theme exits on gtk_init()
      envs+=(QT_QPA_PLATFORM=xcb GDK_BACKEND=x11)
      launcher=(xvfb-run --auto-servernum)
    else
      [ "$OFFSCREEN" = true ] || note "no xvfb-run, so falling back to Qt's offscreen platform"
      envs+=(QT_QPA_PLATFORM=offscreen)
    fi
  elif [ "$OFFSCREEN" = true ]; then
    envs+=(QT_QPA_PLATFORM=offscreen)
  fi
  envs+=("$@")

  local rc=0 started=$SECONDS
  (
    # an inherited value stops Mudlet naming its own, which Media_spec.lua checks
    unset QT_FFMPEG_DECODING_HW_DEVICE_TYPES QT_FFMPEG_ENCODING_HW_DEVICE_TYPES
    cd "$WS"
    bounded "$TIMEOUT" "$dir/log.txt" ${capper[@]+"${capper[@]}"} env "${envs[@]}" ${launcher[@]+"${launcher[@]}"} "$binary" --profile "Mudlet self-test" --mirror --offline
  ) || rc=$?
  stop_fixtures
  if [ -n "$unit" ]; then
    # systemd records the scope's result a moment after its last process exits
    local _
    for _ in 1 2 3 4 5 6 7 8 9 10; do
      case "$(systemctl --user show -p ActiveState --value "$unit" 2> /dev/null)" in active | deactivating) sleep 0.2 ;; *) break ;; esac
    done
    systemctl --user show -p Result --value "$unit" 2> /dev/null | grep -x "oom-kill" > "$dir/memory-events" || true
    systemctl --user reset-failed "$unit" 2> /dev/null || true
  fi
  [ -z "${DISCORD_RUNTIME:-}" ] || rm -rf "$DISCORD_RUNTIME"
  touch "$dir/results.tsv" "$dir/results.tsv.details"

  RUN_STATUS=passed
  # loadGlobal() walks on past a candidate that fails to run - down to the binary's compiled-in source
  # path - so a syntax error in the Lua under test hands the run to another copy. Anchored: --mirror
  # prefixes console output, so a spec that merely echoes the phrase cannot trip it.
  if grep -q "^TLuaInterpreter::loadGlobal() loading" "$dir/log.txt"; then
    note "$label: the Lua under test failed to load, so another copy ran instead - results are meaningless:"
    grep "^TLuaInterpreter::loadGlobal() loading" "$dir/log.txt" >&2
    RUN_STATUS=broken
  elif [ "$rc" -eq 124 ] || { [ "$rc" -eq 137 ] && [ $((SECONDS - started)) -ge "$TIMEOUT" ]; }; then
    RUN_STATUS=timeout
  elif ! grep -q "^[0-9][0-9]* success" "$dir/log.txt" && ! all_tests_ended "$dir/results.tsv"; then
    RUN_STATUS=crashed
  elif [ -e "$run/busted-tests-failed" ] || grep -Eq "^(failure|error)	" "$dir/results.tsv"; then
    RUN_STATUS=failed
  fi
  if [ "$RUN_STATUS" = passed ] && [ "$rc" -ne 0 ]; then
    # every spec green, yet Mudlet exited non-zero: a sanitizer report, or a crash on the way out
    note "$label: every spec passed but Mudlet exited $rc - see $dir/log.txt"
    RUN_STATUS=failed
  fi
  if [ "$RUN_STATUS" = passed ] || [ "$RUN_STATUS" = failed ]; then
    if grep -q "^=== verify-mudlet results hook failed" "$dir/results.tsv.details"; then
      note "$label: the results hook failed, so results.tsv is incomplete - see $dir/results.tsv.details"
      RUN_STATUS=broken
    elif [ ! -s "$dir/results.tsv" ]; then
      note "$label: no test reported a result - see $dir/log.txt"
      RUN_STATUS=broken
    fi
  fi
}

# Windows may capture no stdout from a GUI Mudlet, so the streamed results are the fallback evidence
# that the suite finished: some tests ran and none was left mid-flight
all_tests_ended() { # results.tsv
  [ "$OS" = windows ] || return 1
  awk -F'\t' '$1 == "start" { open++ } $1 != "start" && NF == 3 { open-- } END { exit !(NR > 0 && open <= 0) }' "$1"
}

count_status() { # results.tsv status
  awk -F'\t' -v s="$2" '$1 == s { n++ } END { print n + 0 }' "$1"
}

report_run() { # label [brief: skip the failure list, for diff sides whose differences print later]
  local dir="$OUT/$1"
  local results="$dir/results.tsv"
  printf '%s: %s - %s passed, %s failed, %s errors, %s pending\n' "$1" "$RUN_STATUS" \
    "$(count_status "$results" success)" "$(count_status "$results" failure)" \
    "$(count_status "$results" error)" "$(count_status "$results" pending)"
  case "$RUN_STATUS" in
    crashed)
      awk -F'\t' '$1 == "start" { open = $2 ": " $3 } $1 != "start" { open = ""; done = $2 ": " $3 }
        END { if (open != "") print "  stopped during: " open; else if (done != "") print "  stopped after: " done " (in a hook or between tests)" }' "$results"
      if [ -s "$dir/memory-events" ]; then
        echo "  killed for using more than ${MUDLET_VERIFY_MEMORY_MAX:-16G} of memory (MUDLET_VERIFY_MEMORY_MAX)"
      fi
      echo "  last lines of the log:"
      tail -n 15 "$dir/log.txt" | sed 's/^/    /'
      ;;
    timeout) echo "  killed by the ${TIMEOUT}s timeout; raise it with --timeout" ;;
  esac
  if [ -s "$results.details" ] && [ -z "${2:-}" ]; then
    echo "  failures (full text in $results.details):"
    awk '/^=== / { print "    " substr($0, 5); getline; print "      " $0 }' "$results.details" | head -n 60
  fi
  if [ "$VERBOSE" = true ]; then
    cat "$dir/log.txt"
  fi
  echo "  log: $dir/log.txt"
}

status_exit() { # RUN_STATUS -> exit code
  case "$1" in
    passed) echo 0 ;;
    failed) echo 1 ;;
    crashed) echo 3 ;;
    timeout) echo 124 ;;
    *) echo 2 ;;
  esac
}

head_binary() {
  if [ -n "$BINARY" ]; then
    [ -x "$BINARY" ] || die "$BINARY is not executable"
    printf '%s\n' "$BINARY"
    return
  fi
  find_binary "$WS" || die "no Mudlet build in $WS - build first (see the build-mudlet skill) or pass --binary"
}

head_side() {
  local binary
  binary="$(head_binary)"
  if ! runs_lua_of "$binary" "$WS"; then
    note "$binary does not run this checkout's mudlet-lua (another tree's build, or a stale bundle copy);"
    note "staging it with this checkout's Lua - its C++ is still whatever it was built from"
  fi
  warn_if_stale "$binary"
  binary_for_lua "$binary" "$WS" head
}

mtime() {
  stat -c %Y "$1" 2> /dev/null || stat -f %m "$1"
}

# A binary older than this checkout's C++ runs this checkout's Lua against older C++: a spec for a
# C++ fix then fails, or reproduces the bug the fix was for
warn_if_stale() { # binary
  local built newest path
  built="$(mtime "$1")"
  newest="$(git -C "$WS" log -1 --format=%ct -- src 3rdparty cmake CMakeLists.txt ':(exclude)src/mudlet-lua' 2> /dev/null || echo 0)"
  while IFS= read -r path; do
    [ -f "$WS/$path" ] && [ "$(mtime "$WS/$path")" -gt "$newest" ] && newest="$(mtime "$WS/$path")"
  done < <(git -C "$WS" diff --name-only HEAD -- src 3rdparty cmake ':(exclude)src/mudlet-lua')
  if [ "${newest:-0}" -gt "$built" ]; then
    note "WARNING: $1 is older than this checkout's latest C++ change - rebuild, or expect specs for newer C++ to fail"
  fi
}

cmd_specs() {
  local binary
  binary="$(head_side)"
  stage_specs "$WORK/stage-head" "$WS" ${RESOLVED_SPECS[@]+"${RESOLVED_SPECS[@]}"}
  run_mudlet head "$binary" "$WORK/stage-head"
  report_run head
  exit "$(status_exit "$RUN_STATUS")"
}

resolve_base() {
  local ref="$BASE_REF" remote url
  if [ -z "$ref" ]; then
    for remote in $(git -C "$WS" remote); do
      url="$(git -C "$WS" remote get-url "$remote")"
      case "$url" in *[/:]Mudlet/Mudlet | *[/:]Mudlet/Mudlet.git)
        ref="$remote/development"
        break
        ;;
      esac
    done
    [ -n "$ref" ] || ref=origin/development
    git -C "$WS" rev-parse -q --verify "$ref^{commit}" > /dev/null || die "no $ref to diff against; pass --base"
    BASE_SHA="$(git -C "$WS" merge-base HEAD "$ref")"
  else
    BASE_SHA="$(git -C "$WS" rev-parse --verify "$ref^{commit}")" || die "unknown --base $ref"
  fi
}

# Whether this checkout's changes since the base reach the compiled binary, or only files Mudlet reads
# from disk at startup. utf8_filenames.lua is compiled in as a Qt resource despite where it lives.
binary_changed() {
  local path
  while IFS= read -r path; do
    case "$path" in
      src/mudlet-lua/lua/utf8_filenames.lua) return 0 ;;
      src/mudlet-lua/lua/* | src/mudlet-lua/tests/*) ;;
      src/* | 3rdparty/* | cmake/* | translations/* | CMakeLists.txt | CMakePresets.json) return 0 ;;
    esac
  done < <({ git -C "$WS" diff --name-only "$BASE_SHA"; git -C "$WS" ls-files --others --exclude-standard; } | sort -u)
  return 1
}

default_preset() {
  case "$OS" in
    linux) echo linux-debug-nosan ;;
    mac) echo macos-debug-nosan ;;
    windows) echo windows-debug ;;
  esac
}

# The head build's Qt, so the base is built against the same one
head_cmake_args() {
  local dir cache
  dir="$(dirname "$1")"
  for _ in 1 2 3 4 5; do
    if [ -f "$dir/CMakeCache.txt" ]; then
      cache="$dir/CMakeCache.txt"
      break
    fi
    dir="$(dirname "$dir")"
  done
  [ -n "${cache:-}" ] || return 0
  sed -En 's/^(CMAKE_PREFIX_PATH|Qt6_DIR):[A-Z]*=(.+)$/-D\1=\2/p' "$cache"
}

# One reusable worktree for base builds, so a later run only rebuilds what changed
build_base() { # head binary
  local preset="${BASE_PRESET:-$(default_preset)}" tree lock
  tree="${MUDLET_VERIFY_BASE_TREE:-$(git -C "$WS" rev-parse --path-format=absolute --git-common-dir)/mudlet-verify-base}"
  lock="$tree.lock"
  mkdir "$lock" 2> /dev/null || die "another verify-mudlet is building the base in $tree (remove $lock if not)"
  # expanded now: lock is local, and a set -e exit runs the trap outside this function
  # shellcheck disable=SC2064
  trap "rmdir $(printf %q "$lock") 2> /dev/null; cleanup" EXIT
  if [ ! -d "$tree" ]; then
    note "creating the base worktree $tree"
    git -C "$WS" worktree add --detach "$tree" "$BASE_SHA" > /dev/null
  else
    [ -z "$(git -C "$tree" status --porcelain --untracked-files=no)" ] || die "$tree has local changes; clean it or remove it"
    git -C "$tree" checkout -q --detach "$BASE_SHA"
  fi
  git -C "$tree" submodule update --init --recursive -q
  local args=() gate=() jobs="${MUDLET_VERIFY_JOBS:-8}"
  while IFS= read -r arg; do args+=("$arg"); done < <(head_cmake_args "$1")
  ! command -v build-slot > /dev/null 2>&1 || gate=(build-slot)
  note "building the base ($(git -C "$WS" log -1 --format='%h %s' "$BASE_SHA")) with preset $preset - see $OUT/base-build.log"
  {
    (cd "$tree" && cmake --preset "$preset" ${args[@]+"${args[@]}"}) &&
      (cd "$tree" && ${gate[@]+"${gate[@]}"} cmake --build --preset "$preset" --target mudlet_executable -j "$jobs")
  } > "$OUT/base-build.log" 2>&1 || { tail -n 30 "$OUT/base-build.log" >&2; die "the base build failed"; }
  rmdir "$lock"
  trap cleanup EXIT
  BASE_ROOT="$tree"
  BASE_BINARY="$(preset_binary "$tree" "$preset")"
  [ -x "$BASE_BINARY" ] || die "the base build produced no $BASE_BINARY"
}

# Sets BASE_ROOT (a checkout of the base's sources) and BASE_SIDE_BINARY
prepare_base() { # head binary
  resolve_base
  note "base: $(git -C "$WS" log -1 --format='%h %s' "$BASE_SHA")"
  if [ -n "$BASE_BINARY" ]; then
    BASE_ROOT="$WORK/base-src"
    mkdir -p "$BASE_ROOT"
    git -C "$WS" archive "$BASE_SHA" src/mudlet-lua src/packages | tar -x -C "$BASE_ROOT"
  elif binary_changed; then
    build_base "$1"
  else
    note "only Lua changed since the base, so the base side reuses this binary with the base's Lua"
    BASE_ROOT="$WORK/base-src"
    mkdir -p "$BASE_ROOT"
    git -C "$WS" archive "$BASE_SHA" src/mudlet-lua src/packages | tar -x -C "$BASE_ROOT"
    BASE_BINARY="$1"
  fi
  BASE_SIDE_BINARY="$(binary_for_lua "$BASE_BINARY" "$BASE_ROOT" base)"
}

# One "<file>\t<test>\t<n>\t<status>" per test, n counting repeats of a name
outcomes() { # results.tsv
  awk -F'\t' '$1 != "start" && NF == 3 { key = $2 "\t" $3; seen[key]++; print key "\t" seen[key] "\t" $1 }' "$1" | LC_ALL=C sort
}

# Tests whose outcome differs: "<file>\t<test>\t<n>\t<base status>\t<head status>"
differences() { # base.tsv head.tsv
  outcomes "$1" > "$WORK/base.outcomes"
  outcomes "$2" > "$WORK/head.outcomes"
  awk -F'\t' '
    NR == FNR { base[$1 "\t" $2 "\t" $3] = $4; next }
    { key = $1 "\t" $2 "\t" $3; b = (key in base) ? base[key] : "absent"; delete base[key]
      if (b != $4) print key "\t" b "\t" $4 }
    END { for (key in base) print key "\t" base[key] "\tabsent" }' "$WORK/base.outcomes" "$WORK/head.outcomes" | LC_ALL=C sort
}

diff_specs_round() { # suffix, then specs
  local suffix="$1"
  shift
  local specs_root="$WS"
  [ "$SPECS_FROM" = base ] && specs_root="$BASE_ROOT"
  rm -rf "$WORK/stage-base$suffix" "$WORK/stage-head$suffix"
  stage_specs "$WORK/stage-base$suffix" "$specs_root" "$@"
  stage_specs "$WORK/stage-head$suffix" "$specs_root" "$@"
  run_mudlet "base$suffix" "$BASE_SIDE_BINARY" "$WORK/stage-base$suffix"
  BASE_STATUS=$RUN_STATUS
  report_run "base$suffix" brief
  run_mudlet "head$suffix" "$HEAD_SIDE_BINARY" "$WORK/stage-head$suffix"
  HEAD_STATUS=$RUN_STATUS
  report_run "head$suffix" brief
}

cmd_diff_specs() {
  [ -n "$SPECS_FROM" ] || { if [ ${#RESOLVED_SPECS[@]} -gt 0 ]; then SPECS_FROM="head"; else SPECS_FROM="base"; fi; }
  HEAD_SIDE_BINARY="$(head_side)"
  prepare_base "$(head_binary)"
  note "running ${SPECS_FROM}'s specs on both sides"
  diff_specs_round "" ${RESOLVED_SPECS[@]+"${RESOLVED_SPECS[@]}"}
  differences "$OUT/base/results.tsv" "$OUT/head/results.tsv" > "$OUT/differences.tsv"
  local first_base=$BASE_STATUS first_head=$HEAD_STATUS

  if [ "$RECHECK" = true ] && [ -s "$OUT/differences.tsv" ]; then
    # Re-run the spec files involved; a difference has to show up both times to count
    local files=() file
    while IFS= read -r file; do
      [ "$file" = "?" ] || [ "$file" = "!verify_results_spec.lua" ] || files+=("$file")
    done < <(cut -f1 "$OUT/differences.tsv" | sort -u)
    local recheck_specs=() source="$WS/src/mudlet-lua/tests"
    [ "$SPECS_FROM" = base ] && source="$BASE_ROOT/src/mudlet-lua/tests"
    for file in ${files[@]+"${files[@]}"}; do
      if [ -e "$source/$file" ]; then
        recheck_specs+=("@$file")
        echo "$file" >> "$WORK/rechecked.txt"
      else
        # a probe from outside the tests directory, named on the command line
        for spec in ${RESOLVED_SPECS[@]+"${RESOLVED_SPECS[@]}"}; do
          [ "$(basename "$spec")" = "$file" ] && { recheck_specs+=("$spec"); echo "$file" >> "$WORK/rechecked.txt"; }
        done
      fi
    done
    if [ ${#recheck_specs[@]} -gt 0 ]; then
      note "re-running ${#recheck_specs[@]} spec file(s) with differences to drop flaky ones"
      diff_specs_round "-recheck" "${recheck_specs[@]}"
      differences "$OUT/base-recheck/results.tsv" "$OUT/head-recheck/results.tsv" > "$WORK/recheck.tsv"
      # only a file that was re-run can be found flaky
      awk -F'\t' 'FILENAME == ARGV[1] { rerun[$0] = 1; next } FILENAME == ARGV[2] { again[$0] = 1; next }
        !($1 in rerun) || ($0 in again) { print; next } { print > "/dev/stderr" }' \
        "$WORK/rechecked.txt" "$WORK/recheck.tsv" "$OUT/differences.tsv" > "$WORK/confirmed.tsv" 2> "$OUT/flaky.tsv"
      mv "$WORK/confirmed.tsv" "$OUT/differences.tsv"
    fi
  fi

  echo
  if [ -s "$OUT/flaky.tsv" ]; then
    echo "$(wc -l < "$OUT/flaky.tsv" | tr -d ' ') difference(s) did not reproduce on a re-run (flaky): $OUT/flaky.tsv"
  fi
  if [ -s "$OUT/differences.tsv" ]; then
    echo "$(wc -l < "$OUT/differences.tsv" | tr -d ' ') test(s) behave differently on this checkout (base -> head):"
    awk -F'\t' '{ printf "  %s -> %s  %s: %s\n", $4, $5, $1, $2 }' "$OUT/differences.tsv" | head -n 100
  else
    echo "no test behaves differently between the base and this checkout"
  fi
  case "$first_base $first_head $BASE_STATUS $HEAD_STATUS" in
    *crashed* | *timeout* | *broken*)
      echo "a run did not finish (base: $first_base, head: $first_head; re-run: $BASE_STATUS, $HEAD_STATUS) - the comparison is partial"
      exit 3
      ;;
  esac
  [ ! -s "$OUT/differences.tsv" ] || exit 1
  if [ "$first_base" = passed ] && [ "$first_head" = failed ]; then
    echo "but Mudlet exited with an error on this checkout and not on the base - see $OUT/head/log.txt"
    exit 1
  fi
  exit 0
}

SVOF_URL="https://github.com/Mudlet/bench-data/releases/download/svof-1/SvofBench.mpackage"
SVOF_SHA256="1659af70b73c0e74ea5bb0a3cd53b7f5d6f2392f5acc6b1a0ba41c2fab16c980"

sha256_of() {
  if command -v sha256sum > /dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1; else shasum -a 256 "$1" | cut -d' ' -f1; fi
}

md5_of() {
  if command -v md5sum > /dev/null 2>&1; then md5sum < "$1" | cut -c1-32; else md5 -q "$1"; fi
}

# The default workload: svof, a 2,500-trigger curing system, playing a recorded Achaea session
svof_workload() {
  [ ${#REPLAY_PACKAGES[@]} -eq 0 ] && [ -z "$REPLAY_HOME" ] && [ -z "$REPLAY_SETUP" ] \
    || die "--package, --home-files and --setup need --log: the svof session brings its own"
  local cache="${SVOF_CACHE:-${XDG_CACHE_HOME:-$HOME/.cache}/svof-replay}"
  # the same cache test/svof-replay/run.sh fills
  local bundle="$cache/SvofBench-svof-1.mpackage" dir
  mkdir -p "$cache"
  if [ ! -f "$bundle" ]; then
    note "downloading the svof replay bundle to $cache"
    curl -fsSL -o "$bundle.part" "$SVOF_URL" || die "could not download $SVOF_URL"
    mv "$bundle.part" "$bundle"
  fi
  [ "$(sha256_of "$bundle")" = "$SVOF_SHA256" ] || die "$bundle does not match its published checksum; delete it"
  dir="$cache/$(md5_of "$bundle")"
  if [ ! -f "$dir/manifest.lua" ]; then
    rm -rf "$dir.tmp"
    mkdir -p "$dir.tmp"
    unzip -q "$bundle" -d "$dir.tmp" || die "could not unpack $bundle"
    rm -rf "$dir"
    mv "$dir.tmp" "$dir"
  fi
  REPLAY_LOG="$dir/corpus.txt"
  REPLAY_PACKAGES=("$dir/svof.mpackage")
  REPLAY_HOME="$WORK/svof-home"
  mkdir -p "$REPLAY_HOME"
  cp -R "$dir/svo" "$REPLAY_HOME/svo"
  # as a real Achaea profile: swallow the newline the game sends after a GA prompt
  REPLAY_SETUP="$WORK/svof-setup.lua"
  echo 'setConfig("fixUnnecessaryLinebreaks", true)' > "$REPLAY_SETUP"
}

lua_string() { # a Lua long string that cannot be closed early by the value
  local level="=" value="$1"
  while [[ "$value" == *"]$level]"* ]]; do level="$level="; done
  printf '[%s[%s]%s]' "$level" "$value" "$level"
}

replay_once() { # label binary
  local label="$1" dir="$OUT/$1" stage="$WORK/stage-$1" config="$WORK/replay-$1.lua"
  rm -rf "$stage"
  mkdir -p "$stage/src/mudlet-lua/tests" "$dir"
  cp "$HERE/replay.lua" "$stage/src/mudlet-lua/tests/replay_spec.lua"
  cp "$HERE/results_hook.lua" "$stage/src/mudlet-lua/tests/!verify_results_spec.lua"
  {
    echo "return {"
    echo "  out = $(lua_string "$(native "$dir")"),"
    echo "  log = $(lua_string "$(native "$REPLAY_LOG")"),"
    [ -z "$REPLAY_HOME" ] || echo "  home = $(lua_string "$(native "$REPLAY_HOME")"),"
    [ -z "$REPLAY_SETUP" ] || echo "  setup = $(lua_string "$(native "$REPLAY_SETUP")"),"
    echo "  packages = {"
    for package in ${REPLAY_PACKAGES[@]+"${REPLAY_PACKAGES[@]}"}; do
      echo "    $(lua_string "$(native "$package")"),"
    done
    echo "  },"
    echo "  settle = $(if [ ${#REPLAY_PACKAGES[@]} -gt 0 ] || [ -n "$REPLAY_SETUP" ]; then echo "$REPLAY_SETTLE"; else echo 0; fi),"
    # one socket read's worth at a time, fed back to back so the result does not hang on timer races
    echo "  chunk = 100000,"
    echo "  pump = 0,"
    echo "  after = 1000,"
    echo "  plain = $REPLAY_PLAIN,"
    echo "}"
  } > "$config"
  run_mudlet "$label" "$2" "$stage" "MUDLET_VERIFY_REPLAY_CONFIG=$(native "$config")"
  # --mirror copies the editor's error console to stdout as "<profile>.errors_<profile>| ...", the
  # only place a script's Lua errors leave the profile
  grep -aE '^\**[^|]*\.errors_[^|]*\| ' "$dir/log.txt" | grep -v '\*\*\* starting new session \*\*\*' | sed 's/^[^|]*| //' > "$dir/errors.txt" || true
  if [ "$RUN_STATUS" != passed ]; then
    report_run "$label"
    die "the $label replay did not complete"
  fi
}

STREAMS="console.txt sends.txt events.txt errors.txt"

cmd_diff_replay() {
  [ -n "$REPLAY_LOG" ] || svof_workload
  HEAD_SIDE_BINARY="$(head_side)"
  prepare_base "$(head_binary)"
  local runs=(base head)
  replay_once base "$BASE_SIDE_BINARY"
  replay_once head "$HEAD_SIDE_BINARY"
  if [ "$NOISE_CHECK" = true ]; then
    replay_once base-again "$BASE_SIDE_BINARY"
    replay_once head-again "$HEAD_SIDE_BINARY"
    runs+=(base-again head-again)
  fi

  local stream run changed=0 files text numbers
  echo
  for stream in $STREAMS; do
    files=()
    for run in "${runs[@]}"; do files+=("$OUT/$run/$stream"); done
    compare_stream "$OUT/$stream.noisy" "${files[@]}" > "$OUT/$stream.changes"
    text="$(awk -F'\t' '$5 == "text"' "$OUT/$stream.changes" | wc -l | tr -d ' ')"
    numbers="$(awk -F'\t' '$5 == "numbers"' "$OUT/$stream.changes" | wc -l | tr -d ' ')"
    if [ "$text" -eq 0 ] && [ "$numbers" -eq 0 ]; then
      echo "$stream: same ($(wc -l < "$OUT/head/$stream" | tr -d ' ') lines)"
      rm -f "$OUT/$stream.changes"
      continue
    fi
    [ "$text" -eq 0 ] || changed=1
    # timings a script prints land in the console; sends, events and errors carry none of their own
    [ "$stream" = console.txt ] || [ "$numbers" -eq 0 ] || changed=1
    echo "$stream: $text line(s) changed - $OUT/$stream.changes"
    show_changes text 40 "$OUT/$stream.changes"
    if [ "$numbers" -gt 0 ]; then
      echo "  and $numbers line(s) differ only in their numbers - timings, or a real numeric change:"
      show_changes numbers 5 "$OUT/$stream.changes"
    fi
  done
  if [ "$NOISE_CHECK" = true ]; then
    echo "(lines whose count varied between two runs of one build were compared with their numbers ignored: $OUT/*.noisy)"
  fi
  exit "$changed"
}

show_changes() { # kind max file
  LC_ALL=C awk -F'\t' -v kind="$1" -v max="$2" '$5 == kind && shown++ < max {
    printf "    base %d -> head %d  (base line %d, head line %d) %s\n", $1, $2, $3, $4, substr($6, 1, 160) }' "$3"
}

# Multisets of lines, as timers make order and some lines vary between runs. Files: base, head, then
# optionally base and head again. Prints "<base n>\t<head n>\t<first base line>\t<first head line>\t<text|numbers>\t<line>".
compare_stream() { # noisy-out file...
  local noisy="$1"
  shift
  : > "$noisy"
  LC_ALL=C awk -v runs="$#" -v noisy="$noisy" '
    # colour tags such as <192,192,192:0,0,0> are kept: a colour change is not timing noise
    function digits(s) { gsub(/[0-9]+([.][0-9]+)?/, "#", s); return s }
    function fold(s,   out) {
      out = ""
      while (match(s, /<[0-9]+,[0-9]+,[0-9]+(:[0-9]+,[0-9]+,[0-9]+)?>/)) {
        out = out digits(substr(s, 1, RSTART - 1)) substr(s, RSTART, RLENGTH)
        s = substr(s, RSTART + RLENGTH)
      }
      return out digits(s)
    }
    # by argument position, as an empty file has no first record to count
    BEGIN { for (i = 1; i < ARGC; i++) slot[ARGV[i]] = i }
    { f = slot[FILENAME] }
    { n[f, $0]++; seen[$0] = 1; if (!((f, $0) in at)) at[f, $0] = FNR }
    END {
      if (runs == 4) {
        for (k in seen) {
          if (n[1, k] + 0 != n[3, k] + 0 || n[2, k] + 0 != n[4, k] + 0) {
            if (!(fold(k) in vary)) print fold(k) > noisy
            vary[fold(k)] = 1
          }
        }
      }
      for (k in seen) {
        t = (fold(k) in vary) ? fold(k) : k
        keys[t] = 1
        for (i = 1; i <= runs; i++) {
          c[i, t] += n[i, k]
          if (((i, k) in at) && (!((i, t) in first) || at[i, k] < first[i, t])) first[i, t] = at[i, k]
        }
      }
      for (t in keys) {
        b = c[1, t] + 0; h = c[2, t] + 0
        if (b == h || (runs == 4 && (c[3, t] + 0 != b || c[4, t] + 0 != h))) continue
        changed[t] = 1; fb[fold(t)] += b; fh[fold(t)] += h
      }
      # sort key: where the line sits, so a base line and its head replacement print together
      for (t in changed) {
        print (first[1, t] ? first[1, t] "\t0" : first[2, t] "\t1") "\t" c[1, t] + 0 "\t" c[2, t] + 0 "\t" first[1, t] + 0 "\t" first[2, t] + 0 "\t" (fb[fold(t)] == fh[fold(t)] ? "numbers" : "text") "\t" t
      }
    }' "$@" | LC_ALL=C sort -t "$(printf '\t')" -k1,1n -k2,2n | cut -f3-
}

case "$COMMAND $DIFF_KIND" in
  "specs ") cmd_specs ;;
  "diff specs") cmd_diff_specs ;;
  "diff replay") cmd_diff_replay ;;
esac
