#!/usr/bin/env bash
# Runs in a clean base image: installs /out/*.deb through apt and checks that
# nothing Mudlet needs at runtime is missing.
set -euo pipefail

DEB="$(find /out -name '*.deb' | head -n1)"
[[ -n "$DEB" ]] || { echo "no .deb in /out" >&2; exit 1; }

apt-get update
apt-get install -y --no-install-recommends "$DEB"

missing="$(ldd /usr/bin/mudlet | grep 'not found' || true)"
if [[ -n "$missing" ]]; then
  echo "mudlet has unresolved libraries:" >&2
  echo "$missing" >&2
  exit 1
fi

# Every module the bundled Lua scripts require() must load from the package and
# its Depends, so a new runtime dependency fails here rather than at launch
cd /usr/share/mudlet/lua
missing=""
for m in $(grep -rhoE "require ?\(? *[\"'][A-Za-z0-9_.]+" . | sed -E "s/.*[\"']//" | sort -u); do
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
ssh_library="$(ldd /usr/bin/mudlet | awk '$1 ~ /^libssh\.so/ && $2 == "=>" {print $3; exit}')"
LD_PRELOAD="$ssh_library" lua5.1 - <<'LUA'
local rex = require('rex_pcre2')
local path = '/opt/mudlet/lua'
local result, matches, replacements = rex.gsub(path, [[\\]], '/')
assert(result == path and matches == 0 and replacements == 0)
result, matches, replacements = rex.gsub([[C:\games\mudlet]], [[\\]], '/')
assert(result == 'C:/games/mudlet' and matches == 2 and replacements == 2)
LUA
echo "==> verified $DEB"
