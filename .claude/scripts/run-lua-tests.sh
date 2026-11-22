#!/bin/bash
# Usage: .claude/scripts/run-lua-tests.sh [path-to-mudlet-binary]
# Kept for existing callers; the runner is the verify-mudlet skill's script, which also works on
# macOS and Windows. TESTS_DIRECTORY and MUDLET_LUA_TESTS_TIMEOUT are honoured as before.
set -euo pipefail
WS="$(cd "$(dirname "$0")/../.." && pwd)"
args=(specs --verbose)
[ $# -eq 0 ] || args+=(--binary "$1")
[ -z "${TESTS_DIRECTORY:-}" ] || args+=("$TESTS_DIRECTORY")
exec "$WS/.agents/skills/verify-mudlet/scripts/verify-mudlet.sh" "${args[@]}"
