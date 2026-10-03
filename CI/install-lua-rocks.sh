#!/bin/bash
# Installs the --local Lua rocks that the Linux and macOS builds generate translation
# statistics with, bundle into the macOS app, and run the tests and changelog with.
#
# The macOS jobs cache the result keyed on this file's hash, so any change here - a
# rock, a version or a build flag - installs every rock afresh rather than being
# masked by what the cache already holds.

set -e

LUAROCKS="luarocks --lua-version 5.1 install --local"

if [ "$(uname)" = "Darwin" ]; then
  # Homebrew's prefix differs between x86_64 and arm64, and some of its libraries are keg-only
  : "${HOMEBREW_PREFIX:?HOMEBREW_PREFIX must be set}"
  $LUAROCKS lua-yajl YAJL_DIR="${HOMEBREW_PREFIX}/opt/yajl"
  $LUAROCKS lua-zip ZIP_DIR="${HOMEBREW_PREFIX}/opt/libzip"
  $LUAROCKS LuaSQL-SQLite3 2.6.1 SQLITE_DIR="${HOMEBREW_PREFIX}/opt/sqlite"
  $LUAROCKS lrexlib-pcre2 PCRE2_DIR="${HOMEBREW_PREFIX}/opt/pcre2"
else
  $LUAROCKS lua-yajl
  $LUAROCKS lua-zip
  $LUAROCKS LuaSQL-SQLite3 2.6.1
  $LUAROCKS lrexlib-pcre2
fi

$LUAROCKS LuaFileSystem
$LUAROCKS luautf8
$LUAROCKS lpeg

# CI changelog generation dependencies
$LUAROCKS argparse
$LUAROCKS lunajson

# Lua-based tests
$LUAROCKS busted
