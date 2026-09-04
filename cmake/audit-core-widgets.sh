#!/bin/sh
# audit-core-widgets.sh - Qt Widgets dependency audit for the `mudlet_core` target.
#
# Part of the libmudlet refactor (issues #8681 and #9011). The
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
#      like VarUnit.h (uses QTreeWidgetItem with no direct include).
#
# The QtWidgets header/symbol sets are derived from the installed Qt's own module
# layout (headers present in QtWidgets/ but not in QtGui/ or QtCore/, so Qt6
# relocations like QAction/QShortcut -> QtGui are handled automatically). The
# resulting offending-file count is stable across Qt 6.x versions because Mudlet
# only uses widget classes whose module membership is unchanged.
#
# Known gap - what counts as being *in* the target: the file list is parsed from
# the set(mudlet_SRCS|HDRS ...) and list(APPEND mudlet_SRCS|HDRS ...) blocks, plus
# target_sources() calls naming the library target. Two kinds of target member
# still sit outside that:
#   - mudlet_UIS and mudlet_RCCS, which are target members but not C++ sources.
#   - anything reaching the target through a variable this parser does not know.
# So this audit and CMakeListsConsistencyTest (which counts bare basenames
# anywhere in the file) do not agree about membership, and a file that joins
# mudlet_core by one of those routes is scanned by nobody. An entry *inside* the
# parsed blocks that cannot be reduced to a bare filename is warned about rather
# than skipped, as is a target_sources() whose target cannot be resolved, which
# covers the generator-expression, quoted-path and unknown-target cases.
#
# Usage:
#   cmake/audit-core-widgets.sh                 # print the Markdown report, exit 0
#   cmake/audit-core-widgets.sh --summary       # one-line count, exit 0
#   cmake/audit-core-widgets.sh --count         # bare offending-file count, exit 0
#   cmake/audit-core-widgets.sh --classes       # the derived QtWidgets class names, exit 0
#
# Options:
#   --qt-include DIR   Qt headers dir (the one containing QtWidgets/). Overrides
#                      auto-detection.
#   --baseline FILE    baseline file to record in the report (default:
#                      cmake/core-widgets-baseline.txt).
#   --src DIR          Mudlet src/ dir (default: derived from this script's location).
#   -h, --help         show this help.
#
# Nothing gates on this count yet. Steps 3-10 of the refactor legitimately move
# files between the core and app targets, so an intermediate step can correctly
# raise it, and the audit only becomes a CI gate once the count reaches 0 (#9516).
# Until then the committed report and baseline are regenerated in each libmudlet PR
# and drift shows up in the diff - which is why this script must never report a
# wrong number quietly.
#
# Regenerate the committed artefacts - baseline first, so the report records it:
#   bash cmake/audit-core-widgets.sh --count > cmake/core-widgets-baseline.txt
#   bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md

set -u

# Pin collation so the generated report is byte-identical across machines.
LC_ALL=C
export LC_ALL

PROG=$(basename "$0")
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
REPO_ROOT=$(dirname -- "$SCRIPT_DIR")

MODE=report
QT_INCLUDE=""
SRC_DIR="$REPO_ROOT/src"
BASELINE_FILE="$SCRIPT_DIR/core-widgets-baseline.txt"

err() { printf '%s: %s\n' "$PROG" "$1" >&2; }

usage() {
  sed -n '2,61p' "$0" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
  case "$1" in
    --summary) MODE=summary ;;
    --count) MODE=count ;;
    --classes) MODE=classes ;;
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

qt_query() {
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
    "$HOME"/Qt/*/gcc_64/include \
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

# Recorded in the report so a later run can tell "this Qt sees fewer widget
# classes than the one that generated the record" apart from "this is simply a
# different Qt". Both derived sets grow between Qt releases, so their sizes are
# only comparable within one version.
QT_VER=$(sed -n 's/^#define QTCORE_VERSION_STR  *"\([^"]*\)".*/\1/p' "$QTINC/QtCore/qtcoreversion.h" 2>/dev/null | head -1)
[ -n "$QT_VER" ] || QT_VER="unknown"

# A header counts as QtWidgets only if its filename is absent from QtGui/ and
# QtCore/ - that filters Qt6 compatibility forwarders such as qaction.h and
# qshortcut.h whose classes moved to QtGui.
TMPDIR_AUDIT=$(mktemp -d "${TMPDIR:-/tmp}/core-widgets.XXXXXX") || { err "mktemp failed"; exit 2; }
trap 'rm -rf "$TMPDIR_AUDIT"' EXIT INT TERM
SET_HEADERS="$TMPDIR_AUDIT/headers.txt"
SET_CLASSES="$TMPDIR_AUDIT/classes.txt"
FILE_LIST="$TMPDIR_AUDIT/files.txt"

lower_module_names="$TMPDIR_AUDIT/lower.txt"
widgets_sorted="$TMPDIR_AUDIT/widgets_all.txt"
{ ls "$QTINC/QtGui" 2>/dev/null; ls "$QTINC/QtCore" 2>/dev/null; } | sort -u > "$lower_module_names"
ls "$QW" | sort > "$widgets_sorted"

# A partial/broken Qt (QtWidgets present, QtGui/QtCore missing) would leave the
# filter set empty, so every QtWidgets forwarder header (qaction.h, qshortcut.h,
# ... whose classes actually live in QtGui) would be miscounted as a Widgets
# dependency. Refuse to run rather than emit a silently inflated count.
if [ ! -s "$lower_module_names" ]; then
  err "no QtGui/QtCore headers found under $QTINC (partial or broken Qt install)."
  err "cannot separate QtWidgets headers from forwarders relocated to QtGui; aborting."
  exit 2
fi

comm -23 "$widgets_sorted" "$lower_module_names" \
  | grep -vE '^[0-9]+\.[0-9]+\.[0-9]+$' > "$SET_HEADERS" # drop the versioned private-headers subdir

if [ ! -s "$SET_HEADERS" ]; then
  err "derived an empty QtWidgets header set from $QW - aborting"
  exit 2
fi

# SET_CLASSES drives the symbol signal - the half of the audit that catches a file
# using QApplication with no direct include of it. An empty set silently demotes
# the audit to a plain include scan, and the resulting lower count reads as
# progress, so it gets the same check SET_HEADERS gets above.
grep -E '^Q[A-Z]' "$SET_HEADERS" > "$SET_CLASSES"
if [ ! -s "$SET_CLASSES" ]; then
  err "derived no QtWidgets class names from $QW (no CamelCase class headers found)."
  err "the symbol half of the audit would be dead and the count silently too low; aborting."
  exit 2
fi

# The sizes of the two derived sets are the audit's own measuring instrument. A
# pruned Qt, or one that relocated classes out of QtWidgets, shrinks them - and
# every class that drops out silently takes its users out of the offending count,
# which reads as progress. Recording them lets the guard notice.
HEADER_SET_SIZE=$(wc -l < "$SET_HEADERS" | tr -d ' ')
CLASS_SET_SIZE=$(wc -l < "$SET_CLASSES" | tr -d ' ')

# mudlet_core's file list lives in the mudlet_SRCS / mudlet_HDRS variables of
# src/CMakeLists.txt; both the set(...) blocks and later list(APPEND ...) lines
# contribute, and target_sources() attaches files to the library target directly -
# which is the idiom the planned mudlet_core/mudlet_app split will use - so the awk
# below must catch all three forms. Comments are stripped and CRLF tolerated first
# so a ")" or a filename inside a "#" comment - or a Windows-style checkout - cannot
# silently truncate or pad the parsed list. Any of the blocks may span several
# lines, so each is tracked rather than assumed to be single-line.
awk '
  BEGIN { srcprefix = "${CMAKE_CURRENT_SOURCE_DIR}/" }
  # LIB_MUDLET_TARGET is set in the parent CMakeLists.txt, so it is the one name
  # the single-line set() harvest below cannot resolve for itself.
  function resolveTarget(name,   v) {
    if (name !~ /^[$][{][A-Za-z_][A-Za-z0-9_]*[}]$/) return name
    v = substr(name, 3, length(name) - 3)
    if (v in var) return var[v]
    if (v == "LIB_MUDLET_TARGET") return "mudlet_core"
    return ""
  }
  function targetSeen(name,   t) {
    tgtpending = 0
    t = resolveTarget(name)
    if (t == "mudlet_core") { intgt = 1; return }
    if (t != "") return
    printf "audit-core-widgets: cannot tell which target this target_sources() adds to, so nothing scanned its files: %s\n", (name == "" ? "(no target named)" : name) > "/dev/stderr"
  }
  {
    sub(/\r$/, "")               # tolerate CRLF checkouts
    sub(/#.*/, "")               # a comment cannot contribute or terminate a block
    if (match($0, /^[ \t]*set\([ \t]*/)) {
      vrest = substr($0, RSTART + RLENGTH)
      if (match(vrest, /^[A-Za-z_][A-Za-z0-9_]*/)) {
        vname = substr(vrest, 1, RLENGTH)
        vval = substr(vrest, RLENGTH + 1)
        sub(/\)[ \t]*$/, "", vval)
        sub(/^[ \t]+/, "", vval); sub(/[ \t]+$/, "", vval); gsub(/"/, "", vval)
        if (vval != "" && vval !~ /[ \t${}]/) var[vname] = vval
      }
    }
    if ($0 ~ /^[ \t]*set\(mudlet_(SRCS|HDRS)([ \t]|$)/) inblk=1
    if ($0 ~ /list\(APPEND[ \t]+mudlet_(SRCS|HDRS)([ \t]|\)|$)/) inapp=1
    # Only the library target. target_sources() on the executable adds files that
    # are not part of mudlet_core, so counting them would inflate the denominator.
    # CMake allows the target name on a line of its own after "target_sources(", and
    # that spelling used to match nothing here: the file joined mudlet_core with its
    # widget use uncounted and the run stayed silent, so the name is carried across
    # lines now.
    if (match($0, /target_sources[ \t]*\(/)) {
      trest = substr($0, RSTART + RLENGTH)
      sub(/^[ \t]+/, "", trest)
      if (trest == "") tgtpending=1
      else { split(trest, tname, /[ \t)]/); targetSeen(tname[1]) }
    } else if (tgtpending) {
      trest = $0; sub(/^[ \t]+/, "", trest)
      if (trest != "") { split(trest, tname, /[ \t)]/); targetSeen(tname[1]) }
    }
    if (inblk || inapp || intgt) {
      line=$0; gsub(/[()]/," ",line)
      n=split(line,a,/[ \t]+/)
      for (i=1;i<=n;i++) {
        if (a[i] == "") continue
        # The target_sources() entries naming C++ files are written
        # "${CMAKE_CURRENT_SOURCE_DIR}/sparkleupdater.h". Unquoting and dropping that
        # one prefix is what keeps them out of the missing-from-disk report below;
        # leaving either in place would invent a phantom missing file, which is worse
        # than not parsing the call at all.
        tok=a[i]; gsub(/"/,"",tok)
        if (index(tok, srcprefix) == 1) tok=substr(tok, length(srcprefix) + 1)
        if (tok ~ /\.(cpp|mm|h)$/ && tok !~ /[${}<>*?]/) { print tok; continue }
        # An entry naming a source in a form this parser cannot reduce to a path
        # under src/ - one routed through some other variable, or a generator
        # expression like $<$<BOOL:${USE_X}>:Foo.cpp> - is silently scanned by
        # nobody, so the file joins mudlet_core with its Qt Widgets use uncounted.
        # Warn rather than skip: the ctest guard turns a warning here into a failure.
        if (a[i] ~ /\.(cpp|mm|h)([^A-Za-z0-9_]|$)/)
          printf "audit-core-widgets: not a bare filename, so nothing scanned it: %s\n", a[i] > "/dev/stderr"
      }
    }
    if ($0 ~ /\)/) { inblk=0; inapp=0; intgt=0; tgtpending=0 }
  }
' "$CMAKE_FILE" | sort -uf > "$FILE_LIST"

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

# Per-file output, tab-separated: includes, symbols, path, distinct widget
# refs. Comments and double-quoted strings are stripped first so mentions of
# widget classes in prose or log text don't count as dependencies.
RESULTS="$TMPDIR_AUDIT/results.txt"
( cd "$SRC_DIR" && awk -v setfile="$SET_HEADERS" -v classfile="$SET_CLASSES" '
  BEGIN{
    while ((getline l < setfile) > 0)  W[l]=1
    while ((getline c < classfile) > 0) C[c]=1
    # Match a complete /* ... */ comment whose body may itself contain "*"
    # (e.g. a one-line "/** ... */"). The old /\*[^*]*\*\// stopped at the
    # first inner "*", failed to strip such comments, then mistook them for an
    # unterminated block and silently swallowed the rest of the file.
    blockcmt = "/\\*([^*]|\\*+[^*/])*\\*+/"
  }
  FNR==1 {
    if (nf && incmt) badcmt=order[nf]
    order[++nf]=FILENAME; incmt=0
  }
  {
    line=$0
    sub(/\r$/,"",line)
    if (incmt) {
      p=index(line, "*/")
      if (p==0) next
      line=substr(line, p+2); incmt=0
    }
    gsub(blockcmt,"",line)
    if (line ~ /\/\*/) { sub(/\/\*.*$/,"",line); incmt=1 }
    sub(/\/\/.*$/,"",line)
    if (line ~ /^[ \t]*#[ \t]*include[ \t]*[<"]/) {
      s=line; sub(/^[^<"]*[<"]/,"",s); sub(/[>"].*$/,"",s)
      if (s=="QtWidgets") { inc[FILENAME]++; mark(FILENAME, "QtWidgets") }
      # The prefix is stripped before being re-added: an include that already spells
      # it out was recorded as QtWidgets/QtWidgets/QWidget, which no reader or
      # parser expects and regenerating only reproduced.
      else if (s ~ /^QtWidgets\//) { inc[FILENAME]++; sub(/^QtWidgets\//, "", s); mark(FILENAME, "QtWidgets/" s) }
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
    if (incmt) badcmt=order[nf]
    if (badcmt != "") {
      printf "audit-core-widgets: unterminated block comment in %s - scan aborted\n", badcmt > "/dev/stderr"
      exit 3
    }
    for (k=1;k<=nf;k++) {
      f=order[k]
      printf "%d\t%d\t%s\t%s\n", inc[f]+0, sym[f]+0, f, refs[f]
    }
  }
' $(cat "$EXISTING") ) > "$RESULTS"
awk_status=$?
if [ "$awk_status" -ne 0 ]; then
  err "source scan failed (awk exit $awk_status) - see message above; not emitting a count"
  exit 2
fi

TOTAL=$(wc -l < "$RESULTS" | tr -d ' ')
OFFENDING=$(awk -F'\t' '$1+$2>0' "$RESULTS" | wc -l | tr -d ' ')
CLEAN=$((TOTAL - OFFENDING))

read_baseline() {
  [ -f "$BASELINE_FILE" ] || { err "baseline file not found: $BASELINE_FILE"; return 1; }
  # Strict parse of the first line only. The old "tr -cd 0-9" pooled every digit
  # in the file - a "# updated 2026-07-20" comment turned into a bogus baseline
  # like 20260720158 that would out-rank any real count and pass forever.
  base=$(sed -n '1p' "$BASELINE_FILE" | tr -d '\r')
  case "$base" in
    ''|*[!0-9]*)
      err "baseline file must have a bare integer on its first line: $BASELINE_FILE"
      return 1 ;;
  esac
  printf '%s\n' "$base"
}

case "$MODE" in
  summary)
    printf 'mudlet_core Qt Widgets audit: %s of %s files depend on Qt Widgets\n' "$OFFENDING" "$TOTAL"
    exit 0
    ;;

  count)
    printf '%s\n' "$OFFENDING"
    exit 0
    ;;

  classes)
    cat "$SET_CLASSES"
    exit 0
    ;;

  report)
    BASE="n/a"
    if [ -f "$BASELINE_FILE" ]; then BASE=$(read_baseline) || exit 2; fi
    cat <<EOF
<!-- GENERATED FILE - do not edit by hand. -->
<!-- Regenerate: bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md -->

# libmudlet: Qt Widgets dependency audit (\`mudlet_core\`)

Measures how many source files in the \`mudlet_core\` static-library target
(\`src/CMakeLists.txt\`) still depend on Qt Widgets. Part of the libmudlet
refactor (#8681, #9011): the goal is to drive this count to **0** so \`mudlet_core\`
can build with Qt Widgets absent.

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

**Regenerate** - baseline first, so the report records it:

\`\`\`sh
bash cmake/audit-core-widgets.sh --count > cmake/core-widgets-baseline.txt
bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md
\`\`\`

Nothing gates on this count yet: steps 3-10 of the refactor legitimately move files
between the core and app targets, so an intermediate step can correctly raise it.
The audit becomes a CI gate once the count reaches **0**. Until then this report and
the baseline are regenerated in each libmudlet PR, so drift shows up in the diff.

## Summary

| Metric | Count |
| --- | ---: |
| Source files in \`mudlet_core\` | ${TOTAL} |
| Qt version measured against | ${QT_VER} |
| QtWidgets headers seen | ${HEADER_SET_SIZE} |
| QtWidgets classes seen | ${CLASS_SET_SIZE} |
| Files depending on Qt Widgets | ${OFFENDING} |
| Clean files | ${CLEAN} |
| Committed baseline | ${BASE:-n/a} |

## Offending files

Sorted by total references (includes + symbols), then by path. \`Inc\` = direct
QtWidgets header includes; \`Sym\` = QtWidgets class-symbol references.

| File | Inc | Sym | Widget references |
| --- | ---: | ---: | --- |
EOF
    # Secondary case-insensitive path sort keeps regeneration deterministic.
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
