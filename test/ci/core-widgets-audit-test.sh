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
# CI job already performs rather than costing a job of its own. It fails only when
# a file gains a Qt Widgets dependency it does not have in the committed report -
# a count that drops is the refactor working and merely asks for a regeneration.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
AUDIT="${REPO_DIR}/cmake/audit-core-widgets.sh"
REPORT="${REPO_DIR}/docs/libmudlet-widgets-report.md"
BASELINE="${REPO_DIR}/cmake/core-widgets-baseline.txt"

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

skip() {
  echo "SKIPPED: $*"
  exit "${SKIP_EXIT}"
}

for required in "${AUDIT}" "${REPORT}" "${BASELINE}"; do
  [ -f "${required}" ] || fail "missing ${required#"${REPO_DIR}/"} - the audit and its committed record must stay together"
done

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

QT_INCLUDE="$(find_qt_include)" || skip "no Qt headers with QtWidgets/, QtGui/ and QtCore/ found, so the Widgets audit cannot run.
       Point at them with MUDLET_QT_INCLUDE_DIR=<dir containing QtWidgets/> to run this check."

echo "Qt headers: ${QT_INCLUDE}"

LIVE_REPORT="${WORK_DIR}/live-report.md"
AUDIT_ERR="${WORK_DIR}/audit-stderr.txt"

if ! bash "${AUDIT}" --qt-include "${QT_INCLUDE}" > "${LIVE_REPORT}" 2> "${AUDIT_ERR}"; then
  echo "--- audit-core-widgets.sh stderr ---" >&2
  sed 's/^/    /' "${AUDIT_ERR}" >&2
  fail "cmake/audit-core-widgets.sh could not produce a report against ${QT_INCLUDE}"
fi

LIVE_COUNT="$(bash "${AUDIT}" --qt-include "${QT_INCLUDE}" --count 2> "${AUDIT_ERR}")"
case "${LIVE_COUNT}" in
  '' | *[!0-9]*)
    sed 's/^/    /' "${AUDIT_ERR}" >&2
    fail "cmake/audit-core-widgets.sh --count did not print a number (got '${LIVE_COUNT}')"
    ;;
esac

# Offending files are the report's table rows: "| `path` | inc | sym | refs |".
# The Summary rows above them ("| Source files in `mudlet_core` | N |") do not open
# with a backtick, and the clean-file list uses "- `path`", so neither is picked up
# here.
offending_files() {
  # shellcheck disable=SC2016 # the backticks are literal Markdown, not a subshell
  sed -n 's/^| `\([^`]*\)` | .*/\1/p' "$1"
}

summary_count() {
  sed -n 's/^| Files depending on Qt Widgets | \([0-9][0-9]*\) |.*/\1/p' "$1" | head -1
}

LIVE_FILES="${WORK_DIR}/live-files.txt"
COMMITTED_FILES="${WORK_DIR}/committed-files.txt"
offending_files "${LIVE_REPORT}" | sort > "${LIVE_FILES}"
offending_files "${REPORT}" | sort > "${COMMITTED_FILES}"

# Both harnesses in this repo fail silently when the setup is wrong, so prove the
# parser still understands the format before trusting anything it produced. A
# reformatted report would otherwise reduce this test to asserting nothing.
LIVE_PARSED="$(wc -l < "${LIVE_FILES}" | tr -d ' ')"
if [ "${LIVE_PARSED}" -ne "${LIVE_COUNT}" ]; then
  fail "parsed ${LIVE_PARSED} offending files out of a freshly generated report that counts ${LIVE_COUNT}.
       The report format and this test's parser have diverged - fix the parser in ${BASH_SOURCE[0]#"${REPO_DIR}/"}
       before trusting the guard again."
fi

COMMITTED_SUMMARY="$(summary_count "${REPORT}")"
COMMITTED_PARSED="$(wc -l < "${COMMITTED_FILES}" | tr -d ' ')"
if [ -z "${COMMITTED_SUMMARY}" ]; then
  fail "docs/libmudlet-widgets-report.md has no 'Files depending on Qt Widgets' summary row - it is not a generated report.
       Regenerate it:
${REGENERATE}"
fi
if [ "${COMMITTED_PARSED}" -ne "${COMMITTED_SUMMARY}" ]; then
  fail "docs/libmudlet-widgets-report.md lists ${COMMITTED_PARSED} offending files but its summary says ${COMMITTED_SUMMARY}.
       It has been hand-edited; it is a generated file. Regenerate it:
${REGENERATE}"
fi

BASE="$(sed -n '1p' "${BASELINE}" | tr -d '\r')"
case "${BASE}" in
  '' | *[!0-9]*) fail "cmake/core-widgets-baseline.txt must have a bare integer on its first line, got '${BASE}'" ;;
esac

echo "mudlet_core Qt Widgets audit: ${LIVE_COUNT} offending files (committed report ${COMMITTED_SUMMARY}, baseline ${BASE})"

NEW_FILES="${WORK_DIR}/new-files.txt"
comm -23 "${LIVE_FILES}" "${COMMITTED_FILES}" > "${NEW_FILES}"

if [ -s "${NEW_FILES}" ]; then
  {
    echo "FAIL: $(wc -l < "${NEW_FILES}" | tr -d ' ') file(s) gained a Qt Widgets dependency that docs/libmudlet-widgets-report.md does not record:"
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

if [ "${LIVE_COUNT}" -lt "${BASE}" ] || [ "${LIVE_COUNT}" -ne "${COMMITTED_SUMMARY}" ]; then
  # An improvement must never break somebody's build, so this only reports.
  echo "NOTE: the count has dropped to ${LIVE_COUNT} (baseline ${BASE}, committed report ${COMMITTED_SUMMARY})."
  echo "      Lock the gain in so it cannot be given back:"
  echo "${REGENERATE}"
fi

echo "All checks passed"
