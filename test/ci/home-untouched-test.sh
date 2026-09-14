#!/bin/bash
# Runs the tests that reach Mudlet's credential store under a throwaway HOME and
# fails if anything at all landed in it.
#
# The source scan in XdgRecipeConsistencyTest says a test names a temporary
# directory and points XDG_CONFIG_HOME at it; only running the binary says where
# the bytes went. CredentialManager and SecureStringUtils file per-profile
# encryption keys under QStandardPaths::AppConfigLocation, which for a QTEST_MAIN
# program is $HOME/.config/<test class> - so this check needs no knowledge of what
# a config root looks like, and nothing a test does to its own paths can satisfy
# it by accident. See #10777, where two tests wrote key material there for a year.
#
# The candidates come from the sources rather than a list kept here, so a test
# renamed or added is covered without editing this script. It deliberately does
# not set XDG_CONFIG_HOME: ctest provides one (see test/CMakeLists.txt), and the
# point is what happens to somebody running the binary by hand without it.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BINARY_DIR="${1:-}"

if [[ -z "${BINARY_DIR}" || ! -d "${BINARY_DIR}" ]]; then
  echo "usage: $0 <directory holding the built unit test binaries>" >&2
  exit 1
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

FAILURES=0
CHECKED=0

for source in "${SOURCE_DIR}"/*.cpp; do
  grep -qE '\b(CredentialManager|SecureStringUtils)\b' "${source}" || continue
  name="$(basename "${source}" .cpp)"
  binary="${BINARY_DIR}/${name}"
  if [[ ! -x "${binary}" ]]; then
    # Not a skip: a candidate whose binary is missing is a check that silently
    # stopped covering it. Build the test tree before running this.
    echo "FAIL: ${name} reaches the credential store but ${binary} is not built" >&2
    FAILURES=$((FAILURES + 1))
    continue
  fi

  home="${WORK_DIR}/${name}"
  mkdir -p "${home}"
  CHECKED=$((CHECKED + 1))
  env -u XDG_CONFIG_HOME -u XDG_DATA_HOME -u XDG_CACHE_HOME \
      HOME="${home}" QT_QPA_PLATFORM=offscreen MUDLET_TEST_MODE=1 \
      DBUS_SESSION_BUS_ADDRESS='disabled:' ASAN_OPTIONS=detect_leaks=0 \
      "${binary}" > "${WORK_DIR}/${name}.log" 2>&1
  status=$?

  written="$(find "${home}" -mindepth 1 | sed "s|${home}/||" | sort)"
  if [[ -n "${written}" ]]; then
    echo "FAIL: ${name} wrote into the home directory of whoever runs it:" >&2
    echo "${written}" | sed 's|^|    ~/|' >&2
    FAILURES=$((FAILURES + 1))
  fi
  if [[ ${status} -ne 0 ]]; then
    echo "FAIL: ${name} exited ${status}; see ${WORK_DIR}/${name}.log" >&2
    cat "${WORK_DIR}/${name}.log" >&2
    FAILURES=$((FAILURES + 1))
  fi
done

# A pattern that matches nothing would otherwise report a clean sweep
if [[ ${CHECKED} -lt 4 ]]; then
  echo "FAIL: only ${CHECKED} test(s) checked, so this would pass whatever they write" >&2
  FAILURES=$((FAILURES + 1))
fi

if [[ ${FAILURES} -gt 0 ]]; then
  echo "${FAILURES} failure(s)" >&2
  exit 1
fi

echo "${CHECKED} tests ran without writing anything into HOME"
