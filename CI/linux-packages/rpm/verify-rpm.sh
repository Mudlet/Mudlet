#!/usr/bin/env bash
# Runs in a clean base image: installs /out/*.rpm through dnf and checks that
# nothing Mudlet needs at runtime is missing.
set -euo pipefail

RPM_FILE="$(find /out -name '*.rpm' | head -n1)"
[[ -n "$RPM_FILE" ]] || { echo "no .rpm in /out" >&2; exit 1; }

# libssh reaches Mudlet only through Qt Multimedia's ffmpeg plugin, so install it
# by name for the buffer_free check below
dnf install -y --setopt=install_weak_deps=False "$RPM_FILE" findutils grep libssh

[[ -x /usr/bin/mudlet ]] || { echo "the package installs no /usr/bin/mudlet" >&2; exit 1; }
missing="$(ldd /usr/bin/mudlet | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
  echo "mudlet has unresolved libraries:" >&2
  echo "$missing" >&2
  exit 1
fi

# Every module the bundled Lua scripts require() must load from the package and
# its Requires, so a new runtime dependency fails here rather than at launch.
# The C++ side requires the first list, which no script mentions.
cd /usr/share/mudlet/lua
missing=""
for m in $({ printf '%s\n' lfs zip rex_pcre2 luasql.sqlite3 lua-utf8 yajl lpeg lcf.workshop.base
            grep -rhoE "require ?\(? *[\"'][A-Za-z0-9_.]+" . | sed -E "s/.*[\"']//"; } | sort -u); do
  case "$m" in string|table|math|os|io|coroutine|debug|utf8|package|bit|jit) continue ;; esac
  rel="$(printf '%s' "$m" | tr '.' '/')"
  if [[ -f "$rel.lua" || -f "$rel/init.lua" ]]; then continue; fi
  lua-5.1 -e "require('$m')" >/dev/null 2>&1 || missing="$missing $m"
done
if [[ -n "$missing" ]]; then
  echo "the package does not provide these Lua modules:$missing" >&2
  exit 1
fi

# require() alone misses the buffer_free clash, so load libssh first as Mudlet does
ssh_library="$(ldconfig -p | awk '$1 == "libssh.so.4" {print $NF; exit}')"
[[ -n "$ssh_library" ]] || { echo "libssh.so.4 is not installed" >&2; exit 1; }
LD_PRELOAD="$ssh_library" lua-5.1 - <<'LUA'
local rex = require('rex_pcre2')
local path = '/opt/mudlet/lua'
local result, matches, replacements = rex.gsub(path, [[\\]], '/')
assert(result == path and matches == 0 and replacements == 0)
result, matches, replacements = rex.gsub([[C:\games\mudlet]], [[\\]], '/')
assert(result == 'C:/games/mudlet' and matches == 2 and replacements == 2)
LUA
echo "==> verified $RPM_FILE"
