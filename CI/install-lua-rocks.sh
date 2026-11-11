#!/bin/bash
# Installs the --local Lua rocks that the Linux and macOS builds generate translation
# statistics with, bundle into the macOS app, and run the tests and changelog with.
#
#   install-lua-rocks.sh lua-yajl   lua-yajl alone, needed before the build
#   install-lua-rocks.sh            every other rock, needed after it
#   install-lua-rocks.sh --check    exit 0 only if every rock loads
#
# The macOS jobs cache the result keyed on this file's hash, so any change here - a
# rock, a version or a build flag - installs every rock afresh rather than being
# masked by what the cache already holds.

set -e

LUAROCKS="luarocks --lua-version 5.1 install --local"

if [ "$1" = "--check" ]; then
  eval "$(luarocks path --local --lua-version 5.1)"
  exec lua -e 'for _, m in ipairs({"yajl", "brimworks.zip", "luasql.sqlite3", "rex_pcre2", "lfs",
                                     "lua-utf8", "lpeg", "argparse", "lunajson", "busted"}) do
                 require(m)
               end'
fi

if [ "$(uname)" = "Darwin" ]; then
  # Homebrew's prefix differs between x86_64 and arm64, and some of its libraries are keg-only
  : "${HOMEBREW_PREFIX:?must be set}"
fi

if [ "$1" = "lua-yajl" ]; then
  if [ "$(uname)" = "Darwin" ]; then
    $LUAROCKS lua-yajl YAJL_DIR="${HOMEBREW_PREFIX}/opt/yajl"
  else
    $LUAROCKS lua-yajl
  fi
  exit 0
fi

$LUAROCKS LuaFileSystem
$LUAROCKS luautf8
$LUAROCKS lpeg

if [ "$(uname)" = "Darwin" ]; then
  $LUAROCKS lua-zip ZIP_DIR="${HOMEBREW_PREFIX}/opt/libzip"
  $LUAROCKS LuaSQL-SQLite3 2.6.1 SQLITE_DIR="${HOMEBREW_PREFIX}/opt/sqlite"
  $LUAROCKS lrexlib-pcre2 PCRE2_DIR="${HOMEBREW_PREFIX}/opt/pcre2"
else
  $LUAROCKS lua-zip
  $LUAROCKS LuaSQL-SQLite3 2.6.1
  $LUAROCKS lrexlib-pcre2
fi

# CI changelog generation dependencies
$LUAROCKS argparse
$LUAROCKS lunajson

# Lua-based tests
$LUAROCKS busted
