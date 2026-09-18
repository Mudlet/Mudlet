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
# Known gap - what counts as being *in* the target: the file list is parsed from the
# set(mudlet_SRCS|HDRS ...) and list(APPEND|PREPEND|INSERT ...) blocks, plus
# add_library() and target_sources() calls naming the library target. Three kinds of
# target member still sit outside that:
#   - mudlet_UIS and mudlet_RCCS, which are target members but not C++ sources.
#   - anything reaching the target through a variable this parser does not know.
#   - anything added from a nested add_subdirectory(), which is not followed, or
#     spelled with an uppercase command name, which CMake allows and Mudlet does not
#     use.
# So this audit and CMakeListsConsistencyTest (which counts bare basenames
# anywhere in the file) do not agree about membership, and a file that joins
# mudlet_core by one of those routes is scanned by nobody. An entry *inside* the
# parsed blocks that cannot be reduced to a bare filename aborts the run, as does
# a target_sources() whose target cannot be resolved: the count is already wrong
# at that point, so it is not reported at all.
#
# Usage:
#   cmake/audit-core-widgets.sh                 # print the Markdown report, exit 0
#   cmake/audit-core-widgets.sh --enforce       # CI guard: exit 1 if count > baseline
#   cmake/audit-core-widgets.sh --summary       # one-line count, exit 0
#   cmake/audit-core-widgets.sh --count         # bare offending-file count, exit 0
#
# Options:
#   --qt-include DIR   Qt headers dir (the one containing QtWidgets/). Overrides
#                      auto-detection.
#   --baseline FILE    baseline file for --enforce, also recorded in the report
#                      (default: cmake/core-widgets-baseline.txt).
#   --src DIR          Mudlet src/ dir (default: derived from this script's location).
#   -h, --help         show this help.
#
# Nothing gates on this count yet. Steps 3-10 of the refactor legitimately move
# files between the core and app targets, so an intermediate step can correctly
# raise it, and --enforce only becomes a CI gate once the count reaches 0 (#9516).
# Until then the committed report and baseline are regenerated in each libmudlet PR
# and drift shows up in the diff - which is why this script must never report a
# wrong number quietly.
#
# Regenerate the committed artefacts - baseline first, so the report records it.
# Redirect to a temporary file and move it into place: a redirect truncates its target
# before this script runs, so any fatal error would leave the baseline empty and every
# later run would then abort on it.
#   bash cmake/audit-core-widgets.sh --count > baseline.tmp && mv baseline.tmp cmake/core-widgets-baseline.txt
#   bash cmake/audit-core-widgets.sh > report.tmp && mv report.tmp docs/libmudlet-widgets-report.md

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
  sed -n '2,62p' "$0" | sed 's/^# \{0,1\}//'
}

while [ $# -gt 0 ]; do
  case "$1" in
    --enforce) MODE=enforce ;;
    --summary) MODE=summary ;;
    --count) MODE=count ;;
    --qt-include) shift; [ -n "${1:-}" ] || { err "--qt-include needs a directory"; exit 2; }; QT_INCLUDE="$1" ;;
    --qt-include=*) QT_INCLUDE="${1#*=}"; [ -n "$QT_INCLUDE" ] || { err "--qt-include needs a directory"; exit 2; } ;;
    --baseline) shift; [ -n "${1:-}" ] || { err "--baseline needs a file"; exit 2; }; BASELINE_FILE="$1" ;;
    --baseline=*) BASELINE_FILE="${1#*=}"; [ -n "$BASELINE_FILE" ] || { err "--baseline needs a file"; exit 2; } ;;
    --src) shift; [ -n "${1:-}" ] || { err "--src needs a directory"; exit 2; }; SRC_DIR="$1" ;;
    --src=*) SRC_DIR="${1#*=}"; [ -n "$SRC_DIR" ] || { err "--src needs a directory"; exit 2; } ;;
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

QT_VER=$(sed -n 's/^#define QTCORE_VERSION_STR  *"\([^"]*\)".*/\1/p' "$QTINC/QtCore/qtcoreversion.h" 2>/dev/null | head -1)
[ -n "$QT_VER" ] || QT_VER="unknown"

# A header counts as QtWidgets only if its filename is absent from QtGui/ and
# QtCore/ - that filters Qt6 compatibility forwarders such as qaction.h and
# qshortcut.h whose classes moved to QtGui.
TMPDIR_AUDIT=$(mktemp -d "${TMPDIR:-/tmp}/core-widgets.XXXXXX") || { err "mktemp failed"; exit 2; }
trap 'rm -rf "$TMPDIR_AUDIT"' EXIT
# sh resumes after the handler returns unless it exits, and the count would then be
# taken from files the handler just deleted - reading as zero, the goal number.
trap 'rm -rf "$TMPDIR_AUDIT"; trap - EXIT; exit 2' INT TERM
SET_HEADERS="$TMPDIR_AUDIT/headers.txt"
SET_CLASSES="$TMPDIR_AUDIT/classes.txt"
FILE_LIST="$TMPDIR_AUDIT/files.txt"
FILE_LIST_RAW="$TMPDIR_AUDIT/files-raw.txt"
PARSE_WARNINGS="$TMPDIR_AUDIT/parse-warnings.txt"

lower_module_names="$TMPDIR_AUDIT/lower.txt"
widgets_sorted="$TMPDIR_AUDIT/widgets_all.txt"

# Checked one module at a time, and without hiding ls's own error: an unreadable
# QtGui still leaves a non-empty union from QtCore, and the QtWidgets forwarders
# whose classes moved to QtGui would then be miscounted as Widgets dependencies.
for module in QtGui QtCore; do
  if ! ls "$QTINC/$module" > "$TMPDIR_AUDIT/$module.txt"; then
    err "cannot read $QTINC/$module - partial, broken or unreadable Qt install; aborting."
    exit 2
  fi
  if [ ! -s "$TMPDIR_AUDIT/$module.txt" ]; then
    err "no headers found in $QTINC/$module (partial or broken Qt install)."
    err "cannot separate QtWidgets headers from forwarders relocated to QtGui; aborting."
    exit 2
  fi
done
sort -u "$TMPDIR_AUDIT/QtGui.txt" "$TMPDIR_AUDIT/QtCore.txt" > "$lower_module_names"
ls "$QW" | sort > "$widgets_sorted"

comm -23 "$widgets_sorted" "$lower_module_names" \
  | grep -vE '^[0-9]+\.[0-9]+\.[0-9]+$' > "$SET_HEADERS" # drop the versioned private-headers subdir

if [ ! -s "$SET_HEADERS" ]; then
  err "derived an empty QtWidgets header set from $QW - aborting"
  exit 2
fi

# SET_CLASSES drives the symbol signal - the half of the audit that catches a file
# using QApplication with no direct include of it. Every class missing from this
# set silently takes its users out of the offending count, and a lower count
# reads as progress, so a pruned or stubbed Qt looks like the goal being reached.
# Emptiness is too weak a check to catch that: a set with one class left passes
# it. These four have been in QtWidgets since Qt 4 and are used throughout
# Mudlet, so their absence means a broken instrument rather than a clean tree.
grep -E '^Q[A-Z]' "$SET_HEADERS" > "$SET_CLASSES"
missing_canary=""
for class in QWidget QApplication QLabel QSizePolicy; do
  grep -qx "$class" "$SET_CLASSES" || missing_canary="$missing_canary $class"
done
if [ -n "$missing_canary" ]; then
  err "the QtWidgets class set derived from $QW is missing:$missing_canary"
  err "that set is the audit's own instrument, so the count would be silently too low; aborting."
  exit 2
fi

# Auto-detection takes the first qtpaths/qmake on PATH, so the count can come
# from a different Qt than the reader assumes. Announced on stderr rather than
# in the report, which has to stay byte-identical across Qt versions.
err "measuring against Qt $QT_VER at $QTINC"

# mudlet_core's file list lives in the mudlet_SRCS / mudlet_HDRS variables of
# src/CMakeLists.txt; both the set(...) blocks and later list(APPEND ...) lines
# contribute, and target_sources() attaches files to the library target directly -
# which is the idiom the planned mudlet_core/mudlet_app split will use - so the awk
# below must catch all three forms. Comments are stripped and CRLF tolerated first
# so a ")" or a filename inside a "#" comment - or a Windows-style checkout - cannot
# silently truncate or pad the parsed list. Any of the blocks may span several
# lines, so each is tracked rather than assumed to be single-line.
# Target names such as LIB_MUDLET_TARGET live in the root CMakeLists.txt; without it
# a target_sources() naming one is unresolvable and the run aborts on correct input.
ROOT_CMAKE="$SRC_DIR/../CMakeLists.txt"
set --
[ -f "$ROOT_CMAKE" ] && set -- "$ROOT_CMAKE"
set -- "$@" "$CMAKE_FILE"

awk -v cmakefile="$CMAKE_FILE" '
  BEGIN { srcprefix = "${CMAKE_CURRENT_SOURCE_DIR}/"; listdirprefix = "${CMAKE_CURRENT_LIST_DIR}/" }
  function resolveTarget(name,   v) {
    if (name !~ /^[$][{][A-Za-z_][A-Za-z0-9_]*[}]$/) return name
    v = substr(name, 3, length(name) - 3)
    if (v in var) return var[v]
    # Only reached when the root CMakeLists.txt was unreadable.
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
    # The root file is read for its set() lines alone; only src/ contributes files.
    if (FILENAME != cmakefile) next
    if ($0 ~ /^[ \t]*set\([ \t]*mudlet_(SRCS|HDRS)([ \t]|$)/) inblk=1
    if ($0 ~ /list\([ \t]*(APPEND|PREPEND|INSERT)[ \t]+mudlet_(SRCS|HDRS)([ \t]|\)|$)/) inapp=1
    # Only the library target: target_sources() on the executable adds files that
    # are not part of mudlet_core, so counting them would inflate the denominator.
    # CMake allows the target name on a line of its own after "target_sources(",
    # hence tgtpending carrying the lookup across to the next line.
    if (match($0, /(target_sources|add_library)[ \t]*\(/)) {
      trest = substr($0, RSTART + RLENGTH)
      sub(/^[ \t]+/, "", trest); gsub(/"/, "", trest)
      if (trest == "") tgtpending=1
      else { split(trest, tname, /[ \t)]/); targetSeen(tname[1]) }
    } else if (tgtpending) {
      trest = $0; sub(/^[ \t]+/, "", trest); gsub(/"/, "", trest)
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
        else if (index(tok, listdirprefix) == 1) tok=substr(tok, length(listdirprefix) + 1)
        if (tok ~ /\.(cpp|mm|h)$/ && tok !~ /[${}<>*?]/) { print tok; continue }
        # An entry naming a source in a form this parser cannot reduce to a path
        # under src/ - one routed through some other variable, or a generator
        # expression like $<$<BOOL:${USE_X}>:Foo.cpp> - is silently scanned by
        # nobody, so the file joins mudlet_core with its Qt Widgets use uncounted.
        if (a[i] ~ /\.(cpp|mm|h)([^A-Za-z0-9_]|$)/)
          printf "audit-core-widgets: not a bare filename, so nothing scanned it: %s\n", a[i] > "/dev/stderr"
      }
    }
    if ($0 ~ /\)/) { inblk=0; inapp=0; intgt=0; tgtpending=0 }
  }
' "$@" 2> "$PARSE_WARNINGS" > "$FILE_LIST_RAW" || { err "parsing $CMAKE_FILE failed"; exit 2; }
sort -uf "$FILE_LIST_RAW" > "$FILE_LIST" || { err "sorting the parsed file list failed"; exit 2; }

# Each warning means a file reached mudlet_core by a route the parser could not
# follow, so nothing scanned it and the count below is already too low. That is a
# wrong number, not a caveat, so it is fatal in every mode - including the ones
# that regenerate the committed baseline and report.
if [ -s "$PARSE_WARNINGS" ]; then
  cat "$PARSE_WARNINGS" >&2
  err "entries in $CMAKE_FILE could not be resolved to a file, so nothing scanned them."
  err "the count would be silently too low; teach the parser this spelling before trusting it."
  exit 2
fi

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

# A file listed in CMakeLists.txt but absent from disk means the list is stale or
# the parser drifted, so counting the survivors would under-report.
if [ "$missing" -gt 0 ]; then
  err "$missing file(s) listed in $CMAKE_FILE are missing from disk; aborting."
  exit 2
fi

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
  }
  # Whichever of "//" and "/*" comes first wins, so a "/*" inside a line comment cannot
  # open a block and a "//" inside a block comment cannot close one. Getting that wrong
  # swallows the rest of the file, dropping an offending file from the count.
  function stripComments(l,   a, b, e, rest) {
    while (1) {
      a = index(l, "//"); b = index(l, "/*")
      if (a > 0 && (b == 0 || a < b)) return substr(l, 1, a - 1)
      if (b == 0) return l
      rest = substr(l, b + 2)
      e = index(rest, "*/")
      if (e == 0) { incmt = 1; return substr(l, 1, b - 1) }
      l = substr(l, 1, b - 1) " " substr(rest, e + 2)
    }
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
    # A string can hold "/*" or "//" - a glob like "*/*.lua", or a URL - which would
    # otherwise open a phantom comment and hide the rest of the file. Include lines keep
    # their quotes: the header name is inside them.
    if (line !~ /^[ \t]*#[ \t]*include[ \t]*[<"]/) gsub(/"([^"\\]|\\.)*"/, "\"\"", line)
    line = stripComments(line)
    if (line ~ /^[ \t]*#[ \t]*include[ \t]*[<"]/) {
      s=line; sub(/^[^<"]*[<"]/,"",s); sub(/[>"].*$/,"",s)
      if (s=="QtWidgets") { inc[FILENAME]++; mark(FILENAME, "QtWidgets") }
      # Stripped before being re-added, so an include that already spells the
      # module out does not end up recorded as QtWidgets/QtWidgets/QWidget.
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

# Every file the parser found has to come back with a row. A signal or a failure
# part-way through leaves fewer, and a shortfall only ever removes offenders - so it
# reads as progress towards the goal of zero.
WANTED=$(wc -l < "$EXISTING" | tr -d ' ')
if [ "$TOTAL" != "$WANTED" ]; then
  err "scanned $TOTAL of the $WANTED files parsed from $CMAKE_FILE; the count would be too low. Aborting."
  exit 2
fi
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
    # A failed write must not exit 0: the documented way to refresh the baseline
    # is "--count > cmake/core-widgets-baseline.txt", and the shell truncates that
    # file before this runs, so a silent failure leaves it empty.
    printf 'mudlet_core Qt Widgets audit: %s of %s files depend on Qt Widgets\n' "$OFFENDING" "$TOTAL" || exit 2
    exit 0
    ;;

  count)
    printf '%s\n' "$OFFENDING" || exit 2
    exit 0
    ;;

  enforce)
    BASE=$(read_baseline) || exit 2
    printf 'mudlet_core Qt Widgets audit: %s offending files (baseline %s).\n' "$OFFENDING" "$BASE" || exit 2
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
    BASE="n/a"
    if [ -f "$BASELINE_FILE" ]; then BASE=$(read_baseline) || exit 2; fi
    # Assembled in full before anything reaches stdout, because the documented
    # recipe redirects stdout straight over the committed report: a write that
    # fails midway would otherwise leave a truncated file behind and exit 0.
    {
    cat <<EOF
<!-- GENERATED FILE - do not edit by hand. -->
<!-- Regenerate: bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md -->

# libmudlet: Qt Widgets dependency audit (\`mudlet_core\`)

Measures how many source files in the \`mudlet_core\` static-library target
(\`src/CMakeLists.txt\`) still depend on Qt Widgets. Part of the libmudlet
refactor (#8681, #9011): the goal is to drive this count to **0** so \`mudlet_core\`
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

**Regenerate** - baseline first, so the report records it. Write through a temporary
file: a redirect truncates its target before the audit runs, so a fatal error would
leave the baseline empty and every later run would abort on it.

\`\`\`sh
bash cmake/audit-core-widgets.sh --count > baseline.tmp && mv baseline.tmp cmake/core-widgets-baseline.txt
bash cmake/audit-core-widgets.sh > report.tmp && mv report.tmp docs/libmudlet-widgets-report.md
\`\`\`

Nothing gates on this count yet: steps 3-10 of the refactor legitimately move files
between the core and app targets, so an intermediate step can correctly raise it.
\`--enforce\` becomes a CI gate once the count reaches **0**. Until then this report
and the baseline are regenerated in each libmudlet PR, so drift shows up in the diff.

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
    } > "$TMPDIR_AUDIT/report.md" || { err "failed to assemble the report"; exit 2; }
    cat "$TMPDIR_AUDIT/report.md" || { err "failed to write the report to stdout"; exit 2; }
    exit 0
    ;;
esac
