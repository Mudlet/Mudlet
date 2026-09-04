#!/bin/bash
# Ratchet for the libmudlet Qt Widgets audit (#8681, #9011).
#
# cmake/audit-core-widgets.sh measures how many mudlet_core files still depend on
# Qt Widgets, and docs/libmudlet-widgets-report.md plus
# cmake/core-widgets-baseline.txt record that measurement. Nothing ran the script,
# so the record rotted twice: once to a stale 402/147, and once when an
# include-hygiene change put `#include <QApplication>` back into XMLexport.cpp and
# took the count from 149 to 150 with nothing to notice.
#
# This runs as the CoreWidgetsAuditTest ctest case, so it rides the ctest run every
# CI job already performs rather than costing a job of its own.
#
# The ratchet direction needs care, at two levels. A count that rises is the
# obvious regression, but every way the measurement can break makes the count FALL:
# a renamed mudlet_SRCS variable, a file listed in CMakeLists.txt but absent from
# disk, and - one layer up - a pruned Qt or one that relocated classes out of
# QtWidgets, which shrinks the audit's own measuring instrument and quietly drops
# every user of the vanished classes. A guard that only watches for a rise reads
# all of those as progress and then tells the engineer to bank the phantom win. So
# this floors the denominator, floors the derived Qt sets, checks that every widget
# class the record depends on is still visible, fails on stale-list drift, and
# refuses to skip on CI.

set -uo pipefail

# The audit pins collation so its report is byte-identical across machines; the
# sort/comm pairs below decide whether two lists line up, so they need the same pin
# rather than the caller's locale.
export LC_ALL=C

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
AUDIT="${REPO_DIR}/cmake/audit-core-widgets.sh"
REPORT="${REPO_DIR}/docs/libmudlet-widgets-report.md"
BASELINE="${REPO_DIR}/cmake/core-widgets-baseline.txt"
SELF="${BASH_SOURCE[0]#"${REPO_DIR}/"}"

# ctest maps this to "skipped" via the SKIP_RETURN_CODE test property.
SKIP_EXIT=77

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

# Unchecked, a failed mktemp leaves WORK_DIR empty, every temp path below becomes
# absolute at /, and the run dies with "/live-report.md: Permission denied" while
# blaming the audit and the Qt install. set -e is deliberately off, so check it.
WORK_DIR="$(mktemp -d)" || fail "could not create a temporary directory (is TMPDIR=${TMPDIR:-/tmp} writable?)"
if [ -z "${WORK_DIR}" ] || [ ! -d "${WORK_DIR}" ]; then
  fail "mktemp -d produced no usable directory (is TMPDIR=${TMPDIR:-/tmp} writable?)"
fi
trap 'rm -rf "${WORK_DIR}"' EXIT

CANDIDATES="${WORK_DIR}/qt-candidates.txt"
: > "${CANDIDATES}"

# A contributor whose Qt cannot be located should not be blocked by a progress
# metric. CI is the opposite case: a skip there disables the ratchet permanently
# while every run stays green, which is the exact rot this guard exists to end.
# GitHub Actions always sets CI.
skip_or_fail() {
  if [ -n "${CI:-}" ]; then
    fail "$1

       Refusing to skip because CI is set. A skipped ratchet on CI is indistinguishable
       from a passing one, so it would silently stop guarding while staying green."
  fi
  echo "SKIPPED: $1"
  exit "${SKIP_EXIT}"
}

for required in "${AUDIT}" "${REPORT}" "${BASELINE}"; do
  [ -f "${required}" ] || fail "missing ${required#"${REPO_DIR}/"} - the audit and its committed record must stay together"
done

# Validated before the audit runs: a malformed baseline is its own diagnosis, and
# reporting it here stops it from surfacing later as a confusing "could not produce
# a report" from the audit's own baseline parser.
BASE="$(sed -n '1p' "${BASELINE}" | tr -d '\r')"
case "${BASE}" in
  '' | *[!0-9]*) fail "cmake/core-widgets-baseline.txt must have a bare integer on its first line, got '${BASE}'" ;;
esac

# The audit derives what counts as a QtWidgets header by subtracting the QtGui and
# QtCore header names from the QtWidgets ones, so all three directories have to be
# present; a partial Qt would silently inflate the count instead.
qt_include_usable() {
  [ -n "${1:-}" ] && [ -d "$1/QtWidgets" ] && [ -d "$1/QtGui" ] && [ -d "$1/QtCore" ]
}

qt_include_complaint() {
  local missing=""
  for sub in QtWidgets QtGui QtCore; do
    [ -d "$1/${sub}" ] || missing="${missing} ${sub}/"
  done
  [ -n "${missing}" ] && printf 'no%s under it' "${missing}" && return 0
  printf 'unusable'
}

# MUDLET_QT_INCLUDE_DIR is passed in by test/CMakeLists.txt from the very Qt the
# build was configured against, which is the authoritative answer. Falling through
# from it to whatever qtpaths happens to find swaps the measured Qt without saying
# so, and a different Qt can legitimately yield a different count - so say loudly
# when the passed-in value is rejected.
find_qt_include() {
  local candidate label
  for label in MUDLET_QT_INCLUDE_DIR QT_INCLUDE_DIR; do
    candidate="${!label:-}"
    [ -n "${candidate}" ] || continue
    printf '%s=%s\n' "${label}" "${candidate}" >> "${CANDIDATES}"
    if qt_include_usable "${candidate}"; then
      printf '%s\n' "${candidate}"
      return 0
    fi
    echo "WARNING: ignoring ${label}=${candidate} - $(qt_include_complaint "${candidate}")." >&2
    echo "         That is the Qt this build was configured against; anything found below is a substitute" >&2
    echo "         and may measure a different Qt from the one the committed report was generated with." >&2
  done
  local tool
  for tool in qtpaths6 qtpaths qmake6 qmake; do
    command -v "${tool}" > /dev/null 2>&1 || continue
    case "${tool}" in
      qtpaths*) candidate="$("${tool}" --query QT_INSTALL_HEADERS 2>/dev/null)" ;;
      qmake*) candidate="$("${tool}" -query QT_INSTALL_HEADERS 2>/dev/null)" ;;
    esac
    printf '%s=%s\n' "${tool}" "${candidate:-<no answer>}" >> "${CANDIDATES}"
    if qt_include_usable "${candidate}"; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done
  return 1
}

if ! QT_INCLUDE="$(find_qt_include)"; then
  tried="$(sed 's/^/           /' "${CANDIDATES}")"
  [ -n "${tried}" ] || tried="           (nothing - no MUDLET_QT_INCLUDE_DIR, and no qtpaths/qmake on PATH)"
  skip_or_fail "no Qt headers with QtWidgets/, QtGui/ and QtCore/ found, so the Widgets audit cannot run.
       Point at them with MUDLET_QT_INCLUDE_DIR=<dir containing QtWidgets/> to run this check.
       Tried:
${tried}
       On a macOS framework build the headers are QtWidgets.framework/Headers, which is not this
       layout; the include dir Qt lays down beside the frameworks is the one to point at."
fi

echo "Qt headers: ${QT_INCLUDE}"

# Carries the resolved Qt, so the record gets regenerated against the same Qt the
# guard just measured with rather than whatever the next shell happens to find.
REGENERATE="    bash cmake/audit-core-widgets.sh --qt-include ${QT_INCLUDE} --count > cmake/core-widgets-baseline.txt
    bash cmake/audit-core-widgets.sh --qt-include ${QT_INCLUDE} > docs/libmudlet-widgets-report.md"

LIVE_REPORT="${WORK_DIR}/live-report.md"
LIVE_CLASSES="${WORK_DIR}/live-classes.txt"
REPORT_ERR="${WORK_DIR}/report-stderr.txt"
COUNT_ERR="${WORK_DIR}/count-stderr.txt"
CLASSES_ERR="${WORK_DIR}/classes-stderr.txt"

# Each invocation gets its own stderr file. Sharing one meant the report run's
# warnings were overwritten by the --count run before anything could show them.
show_stderr() {
  [ -s "$1" ] || return 0
  echo "--- audit-core-widgets.sh stderr ---" >&2
  sed 's/^/    /' "$1" >&2
}

if ! bash "${AUDIT}" --qt-include "${QT_INCLUDE}" > "${LIVE_REPORT}" 2> "${REPORT_ERR}"; then
  show_stderr "${REPORT_ERR}"
  fail "cmake/audit-core-widgets.sh could not produce a report against ${QT_INCLUDE}"
fi

LIVE_COUNT="$(bash "${AUDIT}" --qt-include "${QT_INCLUDE}" --count 2> "${COUNT_ERR}")"
case "${LIVE_COUNT}" in
  '' | *[!0-9]*)
    show_stderr "${COUNT_ERR}"
    fail "cmake/audit-core-widgets.sh --count did not print a number (got '${LIVE_COUNT}')"
    ;;
esac

if ! bash "${AUDIT}" --qt-include "${QT_INCLUDE}" --classes > "${LIVE_CLASSES}" 2> "${CLASSES_ERR}"; then
  show_stderr "${CLASSES_ERR}"
  fail "cmake/audit-core-widgets.sh --classes could not list the derived QtWidgets classes"
fi

# The audit only writes to stderr when something is wrong with what it was asked to
# measure - today a file listed in src/CMakeLists.txt but absent from disk, or an
# entry in mudlet_SRCS/HDRS it cannot reduce to a bare filename. Either way it
# measured less than the whole target, and the missing files take their Qt Widgets
# dependencies out of the count with them. Deliberately not tied to one cause: any
# warning on a successful run means the measurement is incomplete.
for stream in "${REPORT_ERR}" "${COUNT_ERR}" "${CLASSES_ERR}"; do
  if [ -s "${stream}" ]; then
    show_stderr "${stream}"
    fail "the audit completed but warned about what it was measuring, so it did not see all of mudlet_core.
       Whatever it skipped took its Qt Widgets dependencies out of the count with it.
       Resolve the warning above before trusting this count."
  fi
done

# Offending files are the report's table rows: "| `path` | inc | sym | refs |".
# The Summary rows above them ("| Source files in `mudlet_core` | N |") do not open
# with a backtick, and the clean-file list uses "- `path`", so neither is picked up
# here.
offending_files() {
  # shellcheck disable=SC2016 # the backticks are literal Markdown, not a subshell
  sed -n 's/^| `\([^`]*\)` | .*/\1/p' "$1"
}

# Every distinct widget class named in the report's reference column, which is what
# the recorded count was actually built out of.
referenced_classes() {
  # shellcheck disable=SC2016 # the backticks are literal Markdown, not a subshell
  sed -n 's/^| `[^`]*` | [0-9]* | [0-9]* | \(.*\) |$/\1/p' "$1" \
    | tr ',' '\n' | sed 's/^[ \t]*//; s/[ \t]*$//' \
    | grep -E '^Q[A-Z][A-Za-z0-9_]*$' | sort -u
}

offender_count() {
  sed -n 's/^| Files depending on Qt Widgets | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1
}

# The denominator. A shrinking offender count is the refactor working; a shrinking
# TOTAL means the audit looked at less of mudlet_core than it used to, which hides
# offenders rather than fixing them.
total_count() {
  sed -n 's/^| Source files in .mudlet_core. | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1
}

# The measuring instrument itself.
qt_version_of() { sed -n 's/^| Qt version measured against | \(.*\) |$/\1/p' "$1" | head -1; }
header_set_of() { sed -n 's/^| QtWidgets headers seen | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1; }
class_set_of() { sed -n 's/^| QtWidgets classes seen | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1; }

LIVE_FILES="${WORK_DIR}/live-files.txt"
COMMITTED_FILES="${WORK_DIR}/committed-files.txt"
if ! offending_files "${LIVE_REPORT}" | sort > "${LIVE_FILES}"; then
  fail "could not parse the offending-file list out of the freshly generated report"
fi
if ! offending_files "${REPORT}" | sort > "${COMMITTED_FILES}"; then
  fail "could not parse the offending-file list out of docs/libmudlet-widgets-report.md"
fi

# Both harnesses in this repo fail silently when the setup is wrong, so prove the
# parser still understands the format before trusting anything it produced. A
# reformatted report would otherwise reduce this test to asserting nothing.
LIVE_PARSED="$(wc -l < "${LIVE_FILES}" | tr -d ' ')"
LIVE_SUMMARY="$(offender_count "${LIVE_REPORT}")"
LIVE_TOTAL="$(total_count "${LIVE_REPORT}")"
if [ "${LIVE_PARSED}" -ne "${LIVE_COUNT}" ] || [ "${LIVE_SUMMARY}" != "${LIVE_COUNT}" ]; then
  fail "parsed ${LIVE_PARSED} offending files and a '${LIVE_SUMMARY}' summary row out of a freshly generated
       report that counts ${LIVE_COUNT}. The report format and this test's parser have diverged - fix the
       parser in ${SELF} before trusting the guard again."
fi
case "${LIVE_TOTAL}" in
  '' | *[!0-9]*)
    fail "could not read the 'Source files in \`mudlet_core\`' total out of a freshly generated report (got '${LIVE_TOTAL}').
       The report format and this test's parser have diverged - fix the parser in ${SELF}."
    ;;
esac

COMMITTED_SUMMARY="$(offender_count "${REPORT}")"
COMMITTED_TOTAL="$(total_count "${REPORT}")"
COMMITTED_PARSED="$(wc -l < "${COMMITTED_FILES}" | tr -d ' ')"
COMMITTED_QTVER="$(qt_version_of "${REPORT}")"
COMMITTED_HDRS="$(header_set_of "${REPORT}")"
COMMITTED_CLS="$(class_set_of "${REPORT}")"
if [ -z "${COMMITTED_SUMMARY}" ] || [ -z "${COMMITTED_TOTAL}" ] \
   || [ -z "${COMMITTED_QTVER}" ] || [ -z "${COMMITTED_HDRS}" ] || [ -z "${COMMITTED_CLS}" ]; then
  fail "docs/libmudlet-widgets-report.md is missing a Summary row - it is not a generated report, or it predates
       the Qt-set rows this guard needs. Regenerate it:
${REGENERATE}"
fi
if [ "${COMMITTED_PARSED}" -ne "${COMMITTED_SUMMARY}" ]; then
  fail "docs/libmudlet-widgets-report.md lists ${COMMITTED_PARSED} offending files but its summary says ${COMMITTED_SUMMARY}.
       It has been hand-edited; it is a generated file. Regenerate it:
${REGENERATE}"
fi

LIVE_QTVER="$(qt_version_of "${LIVE_REPORT}")"
LIVE_HDRS="$(header_set_of "${LIVE_REPORT}")"
LIVE_CLS="$(class_set_of "${LIVE_REPORT}")"

echo "mudlet_core Qt Widgets audit: ${LIVE_COUNT} of ${LIVE_TOTAL} files (committed report ${COMMITTED_SUMMARY} of ${COMMITTED_TOTAL}, baseline ${BASE})"
echo "Qt ${LIVE_QTVER}: ${LIVE_HDRS} QtWidgets headers, ${LIVE_CLS} classes (report recorded Qt ${COMMITTED_QTVER}: ${COMMITTED_HDRS}/${COMMITTED_CLS})"

# The instrument check that does not depend on Qt version. Every widget class the
# recorded count was built out of has to still be visible; if one is not, its users
# silently left the count and the drop is phantom. This is what catches a Qt that
# relocated classes to QtGui - exactly what Qt6 did to QAction - which a size
# comparison cannot distinguish from ordinary version drift.
MISSING_CLASSES="${WORK_DIR}/missing-classes.txt"
COMMITTED_REFS="${WORK_DIR}/committed-refs.txt"
if ! referenced_classes "${REPORT}" > "${COMMITTED_REFS}"; then
  fail "could not read the widget-class references out of docs/libmudlet-widgets-report.md"
fi
if ! comm -23 "${COMMITTED_REFS}" <(sort "${LIVE_CLASSES}") > "${MISSING_CLASSES}"; then
  fail "could not compare the recorded widget classes against the ones this Qt exposes"
fi
if [ -s "${MISSING_CLASSES}" ]; then
  {
    echo "FAIL: $(wc -l < "${MISSING_CLASSES}" | tr -d ' ') widget class(es) the committed report counts are not in this Qt's QtWidgets set:"
    sed 's/^/    /' "${MISSING_CLASSES}"
    cat <<EOF

The audit derives its class list from the Qt it is pointed at, so a class that is
absent takes every file using it out of the count. The result looks like progress
and is not: ${LIVE_COUNT} of ${LIVE_TOTAL} against a recorded ${COMMITTED_SUMMARY} of ${COMMITTED_TOTAL}.

Qt headers used: ${QT_INCLUDE} (Qt ${LIVE_QTVER}, ${LIVE_CLS} classes)
Report recorded: Qt ${COMMITTED_QTVER}, ${COMMITTED_CLS} classes

Either the Qt install is incomplete, or these classes moved to another module the
way QAction and QShortcut moved to QtGui in Qt6. If they genuinely moved, they are
no longer a Widgets dependency and the record should be regenerated - deliberately,
noting the move in the commit message. Do not regenerate to silence a pruned Qt.
${REGENERATE}
EOF
  } >&2
  exit 1
fi

# Sizes are only comparable within one Qt version: both sets grow between releases
# (Qt 6.4.2 exposes 316/193 where 6.12.0 exposes 320/196), so a strict floor across
# versions would fail every older Qt. Within a version a drop means a pruned install.
if [ "${LIVE_QTVER}" = "${COMMITTED_QTVER}" ]; then
  if [ "${LIVE_HDRS}" -lt "${COMMITTED_HDRS}" ] || [ "${LIVE_CLS}" -lt "${COMMITTED_CLS}" ]; then
    fail "this Qt ${LIVE_QTVER} exposes ${LIVE_HDRS} QtWidgets headers and ${LIVE_CLS} classes, but the report recorded
       ${COMMITTED_HDRS} and ${COMMITTED_CLS} from the same Qt version. The measuring instrument has shrunk, so a lower
       offending count is not progress. Usual cause: a pruned or partial Qt install.
       Qt headers used: ${QT_INCLUDE}"
  fi
elif [ "${LIVE_HDRS}" -lt "${COMMITTED_HDRS}" ] || [ "${LIVE_CLS}" -lt "${COMMITTED_CLS}" ]; then
  echo "NOTE: this Qt (${LIVE_QTVER}) exposes fewer QtWidgets headers/classes than the Qt the report was"
  echo "      generated with (${COMMITTED_QTVER}): ${LIVE_HDRS}/${LIVE_CLS} against ${COMMITTED_HDRS}/${COMMITTED_CLS}. Sizes are not compared"
  echo "      across versions; the class-presence check above found nothing missing, so the count stands."
fi

if [ "${LIVE_TOTAL}" -lt "${COMMITTED_TOTAL}" ]; then
  fail "the audit now sees ${LIVE_TOTAL} source files in mudlet_core, but docs/libmudlet-widgets-report.md records ${COMMITTED_TOTAL}.
       Fewer offenders is progress; a smaller TOTAL means the audit measured less of mudlet_core than before,
       and every file it stopped looking at took its Qt Widgets dependencies out of the count with it.
       Usual cause: the mudlet_SRCS / mudlet_HDRS variables in src/CMakeLists.txt were renamed or restructured.
       The audit parses those two names literally, so the planned mudlet_core/mudlet_app split will land here -
       teach cmake/audit-core-widgets.sh the new names rather than regenerating against the smaller view.
       If files genuinely left the target, regenerate so future runs compare against the new total:
${REGENERATE}"
fi

NEW_FILES="${WORK_DIR}/new-files.txt"
GONE_FILES="${WORK_DIR}/gone-files.txt"
COMM_ERR="${WORK_DIR}/comm-stderr.txt"
if ! comm -23 "${LIVE_FILES}" "${COMMITTED_FILES}" > "${NEW_FILES}" 2> "${COMM_ERR}"; then
  sed 's/^/    /' "${COMM_ERR}" >&2
  fail "could not compare the live and committed offender lists"
fi
if ! comm -13 "${LIVE_FILES}" "${COMMITTED_FILES}" > "${GONE_FILES}" 2>> "${COMM_ERR}"; then
  sed 's/^/    /' "${COMM_ERR}" >&2
  fail "could not compare the live and committed offender lists"
fi

NEW_N="$(wc -l < "${NEW_FILES}" | tr -d ' ')"
GONE_N="$(wc -l < "${GONE_FILES}" | tr -d ' ')"

# Quote the row out of the freshly generated report so a finding says which widget
# classes were picked up, not merely which file.
detail_for() {
  awk -F' \\| ' -v want="| \`$1\`" '
    index($0, want) == 1 {
      refs=$4; sub(/ \|$/, "", refs)
      printf "(%s direct include(s), %s symbol reference(s): %s)", $2, $3, refs
      exit
    }
  ' "${LIVE_REPORT}"
}

# A file that only changed path appears in both sets at once, and calling that a
# new Widgets dependency sends the reader hunting for an include nobody added. But
# "one file cleaned, one file broken" produces the same two non-empty sets with no
# rename anywhere, and blessing that as churn would wave a real regression through:
# the count did not rise, so the baseline check never fires afterwards either.
# Only claim churn when the paths actually pair up by basename.
UNPAIRED_NEW="${WORK_DIR}/unpaired-new.txt"
if [ "${NEW_N}" -gt 0 ] && [ "${GONE_N}" -gt 0 ]; then
  sed 's|.*/||' "${GONE_FILES}" | sort -u > "${WORK_DIR}/gone-basenames.txt"
  : > "${UNPAIRED_NEW}"
  while IFS= read -r f; do
    grep -qxF "${f##*/}" "${WORK_DIR}/gone-basenames.txt" || printf '%s\n' "${f}" >> "${UNPAIRED_NEW}"
  done < "${NEW_FILES}"
else
  cp "${NEW_FILES}" "${UNPAIRED_NEW}" 2>/dev/null || : > "${UNPAIRED_NEW}"
fi

if [ "${NEW_N}" -gt 0 ] && [ "${GONE_N}" -gt 0 ] && [ ! -s "${UNPAIRED_NEW}" ] \
   && [ "${LIVE_COUNT}" -le "${COMMITTED_SUMMARY}" ]; then
  {
    echo "FAIL: docs/libmudlet-widgets-report.md is out of date - ${NEW_N} offending file(s) changed path."
    echo "      Every new path matches a disappeared one by filename and the count did not rise"
    echo "      (${LIVE_COUNT} now, ${COMMITTED_SUMMARY} in the report), so this is a move rather than a new dependency."
    echo
    echo "      offending under a path the report does not list:"
    while IFS= read -r f; do echo "          ${f}  $(detail_for "${f}")"; done < "${NEW_FILES}"
    echo "      listed in the report but no longer found:"
    sed 's/^/          /' "${GONE_FILES}"
    cat <<EOF

The libmudlet refactor moves these files into a future mudlet_app target, so path
churn in this set is expected work. Regenerate the record:
${REGENERATE}
EOF
  } >&2
  exit 1
fi

if [ "${NEW_N}" -gt 0 ]; then
  {
    echo "FAIL: ${NEW_N} file(s) are offending that docs/libmudlet-widgets-report.md does not record:"
    while IFS= read -r newFile; do
      echo "    ${newFile}  $(detail_for "${newFile}")"
    done < "${NEW_FILES}"
    if [ "${GONE_N}" -gt 0 ]; then
      echo
      echo "  ${GONE_N} file(s) the report lists are no longer offending:"
      sed 's/^/      /' "${GONE_FILES}"
      echo
      echo "  The two sets do not pair up by filename, so this is not a rename. Treat the files above as"
      echo "  new dependencies: the count alone will not catch them, because a file cleaned elsewhere has"
      echo "  already paid for them and the baseline check will not fire."
    fi
    cat <<EOF

mudlet_core is being made buildable with Qt Widgets absent (#8681, #9011), so a
new Widgets dependency there is a step backwards. Usually a QtGui class does the
job - QGuiApplication for QApplication, QGuiApplication::clipboard() for
QApplication::clipboard() - and a dead include left behind by an include-hygiene
pass just needs deleting.

If the dependency is genuinely unavoidable, record it so the count ratchets from
its new value, and say why in the commit message:
${REGENERATE}
EOF
  } >&2
  exit 1
fi

if [ "${LIVE_COUNT}" -gt "${BASE}" ]; then
  fail "the audit counts ${LIVE_COUNT} offending files but cmake/core-widgets-baseline.txt says ${BASE}.
       No file is newly offending, so the committed artefacts are simply out of date. Regenerate them:
${REGENERATE}"
fi

if [ "${LIVE_COUNT}" -lt "${BASE}" ] || [ "${LIVE_COUNT}" -ne "${COMMITTED_SUMMARY}" ] || [ "${LIVE_TOTAL}" -ne "${COMMITTED_TOTAL}" ]; then
  # An improvement must never break somebody's build, so this only reports.
  echo "NOTE: the record is behind - now ${LIVE_COUNT} of ${LIVE_TOTAL}, report ${COMMITTED_SUMMARY} of ${COMMITTED_TOTAL}, baseline ${BASE}."
  echo "      Every widget class the report counts is still visible in this Qt, so the drop is real."
  echo "      Lock the gain in so it cannot be given back:"
  echo "${REGENERATE}"
fi

echo "All checks passed"
