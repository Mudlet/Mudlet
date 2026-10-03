#!/usr/bin/env bash
# Runs in a clean base image: installs /out/*.deb via apt and checks nothing needed at runtime is missing.
set -euo pipefail

DEB="$(find /out -name '*.deb' | head -n1)"
[[ -n "$DEB" ]] || { echo "no .deb in /out" >&2; exit 1; }

apt-get update
# libssh reaches Mudlet only via Qt Multimedia's ffmpeg plugin; install it for the buffer_free check below
apt-get install -y --no-install-recommends "$DEB" libssh-4

[[ -x /usr/bin/mudlet ]] || { echo "the package installs no /usr/bin/mudlet" >&2; exit 1; }

# Check the installed payload too: mkdeb.sh only sees what cmake --install staged (#10871)
development_files="$(dpkg -L mudlet | grep -E '^/usr/include(/|$)|/cmake/|\.(a|cmake|h|hpp|la|pc)$' || true)"
if [[ -n "$development_files" ]]; then
  echo "the package ships development files:" >&2
  echo "$development_files" >&2
  exit 1
fi

missing="$(ldd /usr/bin/mudlet | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
  echo "mudlet has unresolved libraries:" >&2
  echo "$missing" >&2
  exit 1
fi

# Every module the bundled Lua require()s must load from the package and its Depends, so a new
# runtime dependency fails here, not at launch. The first list is required from C++ only.
cd /usr/share/mudlet/lua
missing=""
for m in $({ printf '%s\n' lfs zip rex_pcre2 luasql.sqlite3 lua-utf8 yajl lpeg lcf.workshop.base
            grep -rhoE "require ?\(? *[\"'][A-Za-z0-9_.]+" . | sed -E "s/.*[\"']//"; } | sort -u); do
  case "$m" in string|table|math|os|io|coroutine|debug|utf8|package|bit|jit) continue ;; esac
  rel="$(printf '%s' "$m" | tr '.' '/')"
  if [[ -f "$rel.lua" || -f "$rel/init.lua" ]]; then continue; fi
  lua5.1 -e "require('$m')" >/dev/null 2>&1 || missing="$missing $m"
done
if [[ -n "$missing" ]]; then
  echo "the package does not provide these Lua modules:$missing" >&2
  exit 1
fi

# require() alone misses the buffer_free clash, so load libssh first as Mudlet does
ssh_library="$(ldconfig -p | awk '$1 == "libssh.so.4" {print $NF; exit}')"
[[ -n "$ssh_library" ]] || { echo "libssh.so.4 is not installed" >&2; exit 1; }
LD_PRELOAD="$ssh_library" lua5.1 - <<'LUA'
local rex = require('rex_pcre2')
local path = '/opt/mudlet/lua'
local result, matches, replacements = rex.gsub(path, [[\\]], '/')
assert(result == path and matches == 0 and replacements == 0)
result, matches, replacements = rex.gsub([[C:\games\mudlet]], [[\\]], '/')
assert(result == 'C:/games/mudlet' and matches == 2 and replacements == 2)
LUA
echo "==> verified $DEB"
