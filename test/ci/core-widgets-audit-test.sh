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
# The ratchet direction needs care. Every way the measurement can break - a renamed
# mudlet_SRCS variable, a degraded Qt header set, a file listed in CMakeLists.txt
# but absent from disk - makes the offender count go DOWN, and a guard that only
# watches for a rise reads all three as progress, then tells the engineer to commit
# the broken number as the new baseline. So this also floors the denominator, fails
# on any stale-list drift the audit reports, and refuses to skip on CI.

set -uo pipefail

# The audit pins collation so its report is byte-identical across machines; the
# sort/comm pair below decides whether two file lists line up, so it needs the
# same pin rather than the caller's locale.
export LC_ALL=C

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
AUDIT="${REPO_DIR}/cmake/audit-core-widgets.sh"
REPORT="${REPO_DIR}/docs/libmudlet-widgets-report.md"
BASELINE="${REPO_DIR}/cmake/core-widgets-baseline.txt"
SELF="${BASH_SOURCE[0]#"${REPO_DIR}/"}"

# ctest maps this to "skipped" via the SKIP_RETURN_CODE test property.
SKIP_EXIT=77

REGENERATE="    bash cmake/audit-core-widgets.sh --count > cmake/core-widgets-baseline.txt
    bash cmake/audit-core-widgets.sh > docs/libmudlet-widgets-report.md"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

fail() {
  echo "FAIL: $*" >&2
  exit 1
}

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

# MUDLET_QT_INCLUDE_DIR is passed in by test/CMakeLists.txt from the very Qt the
# build was configured against, which is the reliable answer. The probes after it
# only matter when this script is run by hand outside ctest.
find_qt_include() {
  local candidate
  for candidate in "${MUDLET_QT_INCLUDE_DIR:-}" "${QT_INCLUDE_DIR:-}"; do
    if qt_include_usable "${candidate}"; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done
  local tool
  for tool in qtpaths6 qtpaths qmake6 qmake; do
    command -v "${tool}" > /dev/null 2>&1 || continue
    case "${tool}" in
      qtpaths*) candidate="$("${tool}" --query QT_INSTALL_HEADERS 2>/dev/null)" ;;
      qmake*) candidate="$("${tool}" -query QT_INSTALL_HEADERS 2>/dev/null)" ;;
    esac
    if qt_include_usable "${candidate}"; then
      printf '%s\n' "${candidate}"
      return 0
    fi
  done
  return 1
}

QT_INCLUDE="$(find_qt_include)" || skip_or_fail "no Qt headers with QtWidgets/, QtGui/ and QtCore/ found, so the Widgets audit cannot run.
       Point at them with MUDLET_QT_INCLUDE_DIR=<dir containing QtWidgets/> to run this check."

echo "Qt headers: ${QT_INCLUDE}"

LIVE_REPORT="${WORK_DIR}/live-report.md"
REPORT_ERR="${WORK_DIR}/report-stderr.txt"
COUNT_ERR="${WORK_DIR}/count-stderr.txt"

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

# On a successful run the audit's only stderr output is its "listed but not found
# on disk (skipped)" warning, which it otherwise acts on only under --enforce. That
# means src/CMakeLists.txt names a file that is not there - a state that breaks the
# real build too - and every such file quietly shrinks the audit's view of
# mudlet_core. Surface it and stop rather than measuring the survivors.
for stream in "${REPORT_ERR}" "${COUNT_ERR}"; do
  if [ -s "${stream}" ]; then
    show_stderr "${stream}"
    fail "the audit completed but warned about the file list above, so it measured only part of mudlet_core.
       Every file it skipped takes its Qt Widgets dependencies out of the count with it.
       Fix src/CMakeLists.txt so every file it lists exists on disk."
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

offender_count() {
  sed -n 's/^| Files depending on Qt Widgets | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1
}

# The denominator. A shrinking offender count is the refactor working; a shrinking
# TOTAL means the audit looked at less of mudlet_core than it used to, which hides
# offenders rather than fixing them.
total_count() {
  sed -n 's/^| Source files in .mudlet_core. | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1
}

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
if [ -z "${COMMITTED_SUMMARY}" ] || [ -z "${COMMITTED_TOTAL}" ]; then
  fail "docs/libmudlet-widgets-report.md is missing a Summary row - it is not a generated report.
       Regenerate it:
${REGENERATE}"
fi
if [ "${COMMITTED_PARSED}" -ne "${COMMITTED_SUMMARY}" ]; then
  fail "docs/libmudlet-widgets-report.md lists ${COMMITTED_PARSED} offending files but its summary says ${COMMITTED_SUMMARY}.
       It has been hand-edited; it is a generated file. Regenerate it:
${REGENERATE}"
fi

echo "mudlet_core Qt Widgets audit: ${LIVE_COUNT} of ${LIVE_TOTAL} files (committed report ${COMMITTED_SUMMARY} of ${COMMITTED_TOTAL}, baseline ${BASE})"

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

# A file that merely changed path appears in both sets at once. Reporting that as
# a brand-new Widgets dependency, on the line under one saying the count did not
# move, sends the reader hunting for an include that was never added. Failing is
# still right - the report is genuinely stale - so only the wording differs.
if [ "${NEW_N}" -gt 0 ] && [ "${GONE_N}" -gt 0 ] && [ "${LIVE_COUNT}" -le "${COMMITTED_SUMMARY}" ]; then
  {
    echo "FAIL: docs/libmudlet-widgets-report.md is out of date - ${NEW_N} offending file(s) moved or were renamed."
    echo "      The Qt Widgets count did not rise (${LIVE_COUNT} now, ${COMMITTED_SUMMARY} in the report), so this is"
    echo "      path churn rather than a new dependency."
    echo
    echo "      offending under a path the report does not list:"
    sed 's/^/          /' "${NEW_FILES}"
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
    echo "FAIL: ${NEW_N} file(s) gained a Qt Widgets dependency that docs/libmudlet-widgets-report.md does not record:"
    while IFS= read -r newFile; do
      # Quote the row out of the freshly generated report so the message says
      # which widget classes were picked up, not merely which file. Naming the
      # file matters more than the detail, so fall back to the bare name.
      detail="$(awk -F' \\| ' -v want="| \`${newFile}\`" '
        index($0, want) == 1 {
          refs=$4; sub(/ \|$/, "", refs)
          printf "(%s direct include(s), %s symbol reference(s): %s)", $2, $3, refs
          exit
        }
      ' "${LIVE_REPORT}")"
      echo "    ${newFile}  ${detail}"
    done < "${NEW_FILES}"
    if [ "${GONE_N}" -gt 0 ]; then
      echo
      echo "  ${GONE_N} file(s) in the report are no longer offending, so some of the above may be renames:"
      sed 's/^/      /' "${GONE_FILES}"
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
  echo "      Lock the gain in so it cannot be given back:"
  echo "${REGENERATE}"
fi

echo "All checks passed"
