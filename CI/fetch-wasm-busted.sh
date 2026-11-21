#!/bin/bash
# Puts busted and the pure-Lua modules it needs into the directory given as the only argument, laid out for a
# package.path of "<dir>/?.lua;<dir>/?/init.lua", for CI/wasm-headless-lua-tests.js to run the specs in a browser:
#
#   CI/fetch-wasm-busted.sh "$HOME/wasm-busted"
#
# Each source is pinned and checked against its sha256. busted's one C module, luasystem, is replaced by
# CI/wasm-busted-shims/system.lua; lua-term, cliargs and dkjson only serve the busted command line, which
# the run-tests package does not use.

set -euo pipefail

if [ $# -ne 1 ] || [ -z "$1" ]; then
  echo "usage: $0 <directory>" >&2
  exit 2
fi

sha256() {
  if command -v sha256sum > /dev/null; then
    sha256sum "$1"
  else
    shasum -a 256 "$1"
  fi | cut -d' ' -f1
}

mkdir -p "$1"
OUT="$(cd "$1" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

# fetch <name> <url> <sha256>: downloads into $WORK, verifies it and unpacks it into $WORK/<name>
fetch() {
  local archive="$WORK/$1.tar.gz"
  # Bounded, as a server that accepts and then sends nothing would hold curl until the job's timeout
  curl -fsSL --retry 3 --connect-timeout 30 --speed-limit 1024 --speed-time 60 -o "$archive" "$2"
  if [ "$(sha256 "$archive")" != "$3" ]; then
    echo "checksum mismatch for $2" >&2
    exit 1
  fi
  mkdir "$WORK/$1"
  tar -xf "$archive" -C "$WORK/$1" --strip-components=1
}

fetch busted https://github.com/lunarmodules/busted/archive/refs/tags/v2.2.0.tar.gz \
  befca10f573bb476fa1db2e3149150d65f802a71d34d1682679e640665f2dc2b
fetch luassert https://github.com/lunarmodules/luassert/archive/refs/tags/v1.9.0.tar.gz \
  1db0fabf1bd87392860375b89a8a37d17b687325c988be0df8c42e7e96e7ed73
fetch say https://github.com/lunarmodules/say/archive/refs/tags/v1.4.1.tar.gz \
  ce07547ca49ef42cc799e2a30b3c65ce77039978e32e7961799a252d61a56486
fetch penlight https://github.com/lunarmodules/Penlight/archive/refs/tags/1.14.0.tar.gz \
  2387431c0e83c4189cccb35b989141a3280d735cb5d42bacf3451af9869bebf7
fetch mediator https://github.com/Olivine-Labs/mediator_lua/archive/refs/tags/v1.1.2-0.tar.gz \
  faf5859fd2081be4e9e4fb8873a2dc65f7eff3fd93d6dd14da65f8e123fcff9b

rm -rf "$OUT/busted" "$OUT/luassert" "$OUT/say" "$OUT/pl" "$OUT/mediator.lua"
cp -R "$WORK/busted/busted" "$OUT/busted"
cp -R "$WORK/luassert/src" "$OUT/luassert"
mkdir "$OUT/say"
cp "$WORK/say/src/say/init.lua" "$OUT/say/init.lua"
cp -R "$WORK/penlight/lua/pl" "$OUT/pl"
cp "$WORK/mediator/src/mediator.lua" "$OUT/mediator.lua"
