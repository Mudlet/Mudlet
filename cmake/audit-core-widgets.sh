#!/bin/sh
# audit-core-widgets.sh - Qt Widgets dependency audit for the `mudlet_core` target.
#
# Part of the libmudlet refactor (see .claude/libmudlet/PLAN.md, phase 0.1). The
# long-term goal is that the `mudlet_core` static library (src/CMakeLists.txt)
# builds with Qt Widgets absent. This script is the measurement for that goal:
# "how many source files in mudlet_core still depend on Qt Widgets?".
#
# What counts as a dependency (two signals, either one flags a file):
#   1. A direct #include of a QtWidgets-module header - both the bare
#      `#include <QWidget>` / `#include <qwidget.h>` forms and the
#      module-qualified `#include <QtWidgets/QWidget>` / `#include <QtWidgets>`
#      forms.
#   2. A reference to a QtWidgets class symbol in the code (e.g. QApplication,
#      QSizePolicy) even when the header arrives transitively via another
#      include. These symbols are exactly what fails to compile once Qt Widgets
#      is gone, so they must be measured too - a pure include scan misses files
#      like XMLexport.cpp (uses QApplication with no direct include).
#
# The QtWidgets header/symbol sets are derived from the installed Qt's own module
# layout (headers present in QtWidgets/ but not in QtGui/ or QtCore/, so Qt6
# relocations like QAction/QShortcut -> QtGui are handled automatically). The
# resulting offending-file count is stable across Qt 6.x versions because Mudlet
# only uses widget classes whose module membership is unchanged.
#
# Usage:
#   cmake/audit-core-widgets.sh                 # print the Markdown report, exit 0
#   cmake/audit-core-widgets.sh --enforce       # CI guard: exit 1 if count > baseline
#   cmake/audit-core-widgets.sh --summary       # one-line count, exit 0
#
# Options:
#   --qt-include DIR   Qt headers dir (the one containing QtWidgets/). Overrides
#                      auto-detection.
#   --baseline FILE    baseline file for --enforce (default: cmake/core-widgets-baseline.txt).
#   --src DIR          Mudlet src/ dir (default: derived from this script's location).
#   -h, --help         show this help.
#
# Regenerate the committed report:
#   bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md

set -u

PROG=$(basename "$0")
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(dirname -- "$SCRIPT_DIR")

MODE=report
QT_INCLUDE=""
SRC_DIR="$REPO_ROOT/src"
BASELINE_FILE="$SCRIPT_DIR/core-widgets-baseline.txt"

err() { printf '%s: %s\n' "$PROG" "$1" >&2; }

usage() {
  sed -n '2,39p' "$0" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
  case "$1" in
    --enforce) MODE=enforce ;;
    --summary) MODE=summary ;;
    --qt-include) shift; QT_INCLUDE="${1:-}" ;;
    --qt-include=*) QT_INCLUDE="${1#*=}" ;;
    --baseline) shift; BASELINE_FILE="${1:-}" ;;
    --baseline=*) BASELINE_FILE="${1#*=}" ;;
    --src) shift; SRC_DIR="${1:-}" ;;
    --src=*) SRC_DIR="${1#*=}" ;;
    -h|--help) usage; exit 0 ;;
    *) err "unknown argument: $1"; usage >&2; exit 2 ;;
  esac
  shift
done

CMAKE_FILE="$SRC_DIR/CMakeLists.txt"
if [ ! -f "$CMAKE_FILE" ]; then
  err "cannot find $CMAKE_FILE (pass --src DIR)"
  exit 2
fi

# ---------------------------------------------------------------------------
# Locate the Qt headers directory that holds QtWidgets/.
# ---------------------------------------------------------------------------
qt_query() {
  # $1 = tool name; echoes QT_INSTALL_HEADERS if the tool resolves it
  command -v "$1" >/dev/null 2>&1 || return 1
  case "$1" in
    qtpaths*) "$1" --query QT_INSTALL_HEADERS 2>/dev/null ;;
    qmake*)   "$1" -query QT_INSTALL_HEADERS 2>/dev/null ;;
  esac
}

detect_qt_include() {
  if [ -n "$QT_INCLUDE" ]; then printf '%s\n' "$QT_INCLUDE"; return 0; fi
  if [ -n "${QT_INCLUDE_DIR:-}" ]; then printf '%s\n' "$QT_INCLUDE_DIR"; return 0; fi
  for tool in qtpaths6 qtpaths qmake6 qmake; do
    dir=$(qt_query "$tool") || continue
    [ -n "$dir" ] && [ -d "$dir/QtWidgets" ] && { printf '%s\n' "$dir"; return 0; }
  done
  for dir in \
    /home/vadi/Programs/Qt/6.12.0/gcc_64/include \
    /usr/include/qt6 \
    /usr/include/x86_64-linux-gnu/qt6 \
    /usr/local/include/qt6 \
    /opt/homebrew/include; do
    [ -d "$dir/QtWidgets" ] && { printf '%s\n' "$dir"; return 0; }
  done
  return 1
}

QTINC=$(detect_qt_include) || {
  err "could not locate Qt headers with a QtWidgets/ subdirectory."
  err "point at it with --qt-include DIR or the QT_INCLUDE_DIR environment variable."
  exit 2
}
QW="$QTINC/QtWidgets"
if [ ! -d "$QW" ]; then
  err "no QtWidgets/ under $QTINC"
  exit 2
fi

# ---------------------------------------------------------------------------
# Derive the QtWidgets header-name set (W) and class-name set (C).
# A header belongs to QtWidgets only if its filename is not also present in
# QtGui/ or QtCore/ (that filters Qt6 compatibility forwarders such as
# qaction.h/qshortcut.h that now live in QtGui). Class names are the CamelCase
# forwarding headers (QWidget, QDialog, ...).
# ---------------------------------------------------------------------------
TMPDIR_AUDIT=$(mktemp -d "${TMPDIR:-/tmp}/core-widgets.XXXXXX") || { err "mktemp failed"; exit 2; }
trap 'rm -rf "$TMPDIR_AUDIT"' EXIT INT TERM
SET_HEADERS="$TMPDIR_AUDIT/headers.txt"
SET_CLASSES="$TMPDIR_AUDIT/classes.txt"
FILE_LIST="$TMPDIR_AUDIT/files.txt"

lower_module_names="$TMPDIR_AUDIT/lower.txt"
widgets_sorted="$TMPDIR_AUDIT/widgets_all.txt"
{ ls "$QTINC/QtGui" 2>/dev/null; ls "$QTINC/QtCore" 2>/dev/null; } | sort -u > "$lower_module_names"
ls "$QW" | sort > "$widgets_sorted"
# QtWidgets entries minus lower-module names, minus the versioned subdir.
comm -23 "$widgets_sorted" "$lower_module_names" \
  | grep -vE '^[0-9]+\.[0-9]+\.[0-9]+$' > "$SET_HEADERS"
grep -E '^Q[A-Z]' "$SET_HEADERS" > "$SET_CLASSES"

if [ ! -s "$SET_HEADERS" ]; then
  err "derived an empty QtWidgets header set from $QW - aborting"
  exit 2
fi

# ---------------------------------------------------------------------------
# Enumerate the source files of the mudlet_core target by parsing the
# mudlet_SRCS / mudlet_HDRS lists (both the set(...) blocks and the
# list(APPEND ...) additions) out of src/CMakeLists.txt.
# ---------------------------------------------------------------------------
awk '
  { if ($0 ~ /^[ \t]*set\(mudlet_(SRCS|HDRS)/) inblk=1
    line=$0; gsub(/[()]/," ",line)
    isappend = ($0 ~ /list\(APPEND[ \t]+mudlet_(SRCS|HDRS)/)
    if (inblk || isappend) {
      n=split(line,a,/[ \t]+/)
      for (i=1;i<=n;i++) if (a[i] ~ /\.(cpp|mm|h)$/) print a[i]
    }
    if (inblk && $0 ~ /\)/) inblk=0
  }
' "$CMAKE_FILE" | sort -uf > "$FILE_LIST"

# Keep only files that exist (paths are relative to src/).
EXISTING="$TMPDIR_AUDIT/existing.txt"
: > "$EXISTING"
missing=0
while IFS= read -r f; do
  if [ -f "$SRC_DIR/$f" ]; then
    printf '%s\n' "$f" >> "$EXISTING"
  else
    missing=$((missing + 1))
    err "listed but not found on disk (skipped): $f"
  fi
done < "$FILE_LIST"

if [ ! -s "$EXISTING" ]; then
  err "no source files parsed from $CMAKE_FILE"
  exit 2
fi

# ---------------------------------------------------------------------------
# Scan every file once. Emits, tab-separated, per file:
#   includes <TAB> symbols <TAB> path <TAB> distinct widget refs
# Comments (// and /* */) and double-quoted strings are stripped before
# matching to keep noise out.
# ---------------------------------------------------------------------------
RESULTS="$TMPDIR_AUDIT/results.txt"
( cd "$SRC_DIR" && awk -v setfile="$SET_HEADERS" -v classfile="$SET_CLASSES" '
  BEGIN{
    while ((getline l < setfile) > 0)  W[l]=1
    while ((getline c < classfile) > 0) C[c]=1
  }
  FNR==1 { order[++nf]=FILENAME; incmt=0 }
  {
    line=$0
    if (incmt) { if (line ~ /\*\//) { sub(/^.*\*\//,"",line); incmt=0 } else next }
    gsub(/\/\*[^*]*\*\//,"",line)
    if (line ~ /\/\*/) { sub(/\/\*.*$/,"",line); incmt=1 }
    sub(/\/\/.*$/,"",line)
    if (line ~ /^[ \t]*#[ \t]*include[ \t]*[<"]/) {
      s=line; sub(/^[^<"]*[<"]/,"",s); sub(/[>"].*$/,"",s)
      if (s=="QtWidgets" || s ~ /^QtWidgets\//) { inc[FILENAME]++; mark(FILENAME, "QtWidgets/" s) }
      else if (s ~ /\//) { }
      else if (s in W) { inc[FILENAME]++; mark(FILENAME, s) }
      next
    }
    gsub(/"[^"]*"/,"",line)
    n=split(line, t, /[^A-Za-z0-9_]+/)
    for (i=1;i<=n;i++) if (t[i] in C) { sym[FILENAME]++; mark(FILENAME, t[i]) }
  }
  function mark(f, tok,   key) {
    key = f SUBSEP tok
    if (!(key in seen)) { seen[key]=1; refs[f] = refs[f] (refs[f]==""?"":", ") tok }
  }
  END {
    for (k=1;k<=nf;k++) {
      f=order[k]
      printf "%d\t%d\t%s\t%s\n", inc[f]+0, sym[f]+0, f, refs[f]
    }
  }
' $(cat "$EXISTING") ) > "$RESULTS"

TOTAL=$(wc -l < "$RESULTS" | tr -d ' ')
OFFENDING=$(awk -F'\t' '$1+$2>0' "$RESULTS" | wc -l | tr -d ' ')
CLEAN=$((TOTAL - OFFENDING))

# ---------------------------------------------------------------------------
# Output.
# ---------------------------------------------------------------------------
read_baseline() {
  [ -f "$BASELINE_FILE" ] || { err "baseline file not found: $BASELINE_FILE"; return 1; }
  base=$(tr -cd '0-9' < "$BASELINE_FILE")
  [ -n "$base" ] || { err "baseline file has no integer: $BASELINE_FILE"; return 1; }
  printf '%s\n' "$base"
}

case "$MODE" in
  summary)
    printf 'mudlet_core Qt Widgets audit: %s of %s files depend on Qt Widgets\n' "$OFFENDING" "$TOTAL"
    exit 0
    ;;

  enforce)
    BASE=$(read_baseline) || exit 2
    printf 'mudlet_core Qt Widgets audit: %s offending files (baseline %s).\n' "$OFFENDING" "$BASE"
    if [ "$OFFENDING" -gt "$BASE" ]; then
      err "regression: $OFFENDING > baseline $BASE. New/changed files pulled Qt Widgets into mudlet_core."
      err "Offenders above baseline - inspect recent changes; run without --enforce for the full report."
      exit 1
    fi
    if [ "$OFFENDING" -lt "$BASE" ]; then
      printf 'Improvement: below baseline. Lower %s to %s to lock in the gain.\n' "$BASELINE_FILE" "$OFFENDING"
    fi
    exit 0
    ;;

  report)
    BASE=""
    if [ -f "$BASELINE_FILE" ]; then BASE=$(tr -cd '0-9' < "$BASELINE_FILE"); fi
    cat <<EOF
<!-- GENERATED FILE - do not edit by hand. -->
<!-- Regenerate: bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md -->

# libmudlet: Qt Widgets dependency audit (\`mudlet_core\`)

Measures how many source files in the \`mudlet_core\` static-library target
(\`src/CMakeLists.txt\`) still depend on Qt Widgets. Part of the libmudlet
refactor (phase 0.1): the goal is to drive this count to **0** so \`mudlet_core\`
can build with Qt Widgets absent, after which this audit becomes an enforcing CI
guard (\`--enforce\`).

A file is counted as depending on Qt Widgets if it either:

- directly \`#include\`s a QtWidgets-module header (bare \`<QWidget>\`,
  \`<qwidget.h>\`, or module-qualified \`<QtWidgets/...>\` / \`<QtWidgets>\`), or
- references a QtWidgets class symbol (e.g. \`QApplication\`, \`QSizePolicy\`) even
  when the header arrives transitively - those symbols are what break once Qt
  Widgets is removed.

The QtWidgets header and class sets are derived from the installed Qt's module
layout (headers in \`QtWidgets/\` that are not also in \`QtGui/\`/\`QtCore/\`), so
Qt6 relocations such as \`QAction\`/\`QShortcut\` -> QtGui are excluded
automatically. The offending-file count is stable across Qt 6.x releases.

Genuine widget classes (the \`dlg*\`, \`T*\`-widget, and \`mudlet\` UI files) are
included in the count; the refactor plan moves those wholesale to a future
\`mudlet_app\` target rather than de-widgeting them, so the count drops through a
mix of moving and refactoring.

**Regenerate:** \`bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md\`
**CI guard:** \`bash cmake/audit-core-widgets.sh --enforce\` (baseline in \`cmake/core-widgets-baseline.txt\`)

## Summary

| Metric | Count |
| --- | ---: |
| Source files in \`mudlet_core\` | ${TOTAL} |
| Files depending on Qt Widgets | ${OFFENDING} |
| Clean files | ${CLEAN} |
| Committed baseline | ${BASE:-n/a} |

## Offending files

Sorted by total references (includes + symbols), then by path. \`Inc\` = direct
QtWidgets header includes; \`Sym\` = QtWidgets class-symbol references.

| File | Inc | Sym | Widget references |
| --- | ---: | ---: | --- |
EOF
    # Offending rows: refs desc, then path asc (case-insensitive), deterministic.
    awk -F'\t' '$1+$2>0 { printf "%d\t%d\t%d\t%s\t%s\n", $1+$2, $1, $2, $3, $4 }' "$RESULTS" \
      | sort -t "$(printf '\t')" -k1,1nr -k4,4f \
      | awk -F'\t' '{ printf "| `%s` | %d | %d | %s |\n", $4, $2, $3, ($5==""?"-":$5) }'

    cat <<EOF

## Clean files (${CLEAN})

<details>
<summary>Files with no Qt Widgets dependency</summary>

EOF
    awk -F'\t' '$1+$2==0 { print $3 }' "$RESULTS" | sort -f | sed 's/^/- `/; s/$/`/'
    cat <<EOF

</details>
EOF
    exit 0
    ;;
esac
