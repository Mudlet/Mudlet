#!/bin/bash
# Checks that the tests which reach Mudlet's credential store leave the home
# directory of whoever runs them alone.
#
# The source scan in XdgRecipeConsistencyTest says a test names a temporary
# directory and points XDG_CONFIG_HOME at it; only looking at the filesystem says
# where the bytes went. CredentialManager and SecureStringUtils file per-profile
# encryption keys and passwords under QStandardPaths::AppConfigLocation without
# asking Mudlet where its config root is, so for a QTEST_MAIN program that is a
# directory named after the test class in the user's own config root - a path no
# source scan can recognise and nothing in the test tree names. See #10777, where
# two tests wrote key material there for a year.
#
# The candidates come from the sources rather than a list kept here, so a test
# renamed or added is covered without editing this script. The whole of test/ is
# walked, not its top level: a source compiled into one of the grouped functional
# binaries (test/functional_tests/CMakeLists.txt) has no binary of its own, and
# the first sweep needs none.
#
# Sweep 1 - no config root named after a candidate exists in the real home. Needs
# no binary and no run of its own, so it covers the grouped sources too, and it is
# the sweep that means something on macOS.
#
# Sweep 2 - each candidate that ctest builds as its own binary, run with a home
# directory of its own and without the XDG_CONFIG_HOME that ctest hands it (see
# test/CMakeLists.txt): the point is what happens to somebody running the binary
# by hand. What that proves differs by platform, because the platforms do not
# resolve AppConfigLocation the same way; see the two branches below.

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
CANDIDATES=0
CONTAINED=0

case "$(uname -s)" in
  Darwin) MACOS=1 ;;
  *)      MACOS=0 ;;
esac

# Where a QTEST_MAIN program's AppConfigLocation lands for the user running this.
# On macOS that is NSSearchPathForDirectoriesInDomains(NSLibraryDirectory,
# NSUserDomainMask) + "/Preferences"; ~/.config is listed there too because Mudlet
# keeps its own config root there on every platform (mudlet::setupConfig()).
CONFIG_ROOTS=("${HOME}/.config")
if [[ ${MACOS} -eq 1 ]]; then
  CONFIG_ROOTS+=("${HOME}/Library/Preferences")
fi
if [[ -n "${XDG_CONFIG_HOME:-}" ]]; then
  CONFIG_ROOTS+=("${XDG_CONFIG_HOME}")
fi

# The class name is the leaf of AppConfigLocation either way: QTEST_MAIN leaves
# the application name at the executable's, and MUDLET_GROUPED_TEST_MAIN sets it
# to the case's (test/functional_tests/GroupedTest.h), and both match the file.
candidates=()
while IFS= read -r source; do
  grep -qE '\b(CredentialManager|SecureStringUtils)\b' "${source}" || continue
  candidates+=("${source}")
done < <(find "${SOURCE_DIR}" -name '*.cpp' | sort)

# "${candidates[@]}" on an empty array is an unbound variable under `set -u` in the
# bash 3.2 macOS ships, so the sweep below has to be reached with something in it
for source in ${candidates[@]+"${candidates[@]}"}; do
  name="$(basename "${source}" .cpp)"
  CANDIDATES=$((CANDIDATES + 1))

  # ---- sweep 1: the real home ----------------------------------------------
  for root in "${CONFIG_ROOTS[@]}"; do
    if [[ -e "${root}/${name}" ]]; then
      echo "FAIL: ${root}/${name} exists, so ${name} filed a config root in the home directory" >&2
      echo "      of whoever ran it. Delete it, then give the test a config root of its own." >&2
      FAILURES=$((FAILURES + 1))
    fi
  done

  # ---- sweep 2: a run of its own -------------------------------------------
  binary="${BINARY_DIR}/${name}"
  if [[ ! -x "${binary}" ]]; then
    if [[ "$(dirname "${source}")" == "${SOURCE_DIR}" ]]; then
      # Not a skip: every source at the top of test/ is a binary of its own, so a
      # candidate without one is a check that silently stopped covering it. Build
      # the test tree before running this.
      echo "FAIL: ${name} reaches the credential store but ${binary} is not built" >&2
      FAILURES=$((FAILURES + 1))
    fi
    # A grouped functional source has no binary to run on its own, and running it
    # through its group binary would run a whole functional test to learn what
    # sweep 1 already knows. Sweep 1 is its coverage.
    continue
  fi

  home="${WORK_DIR}/${name}"
  mkdir -p "${home}"
  CHECKED=$((CHECKED + 1))
  # CFFIXED_USER_HOME is the CoreFoundation home override that
  # NSSearchPathForDirectoriesInDomains resolves against, and so the only way to
  # move AppConfigLocation on macOS. It is set on every platform because the ones
  # that do not have CoreFoundation ignore it.
  env -u XDG_CONFIG_HOME -u XDG_DATA_HOME -u XDG_CACHE_HOME \
      HOME="${home}" CFFIXED_USER_HOME="${home}" \
      QT_QPA_PLATFORM=offscreen MUDLET_TEST_MODE=1 \
      DBUS_SESSION_BUS_ADDRESS='disabled:' ASAN_OPTIONS=detect_leaks=0 \
      "${binary}" > "${WORK_DIR}/${name}.log" 2>&1
  status=$?

  written="$(find "${home}" -mindepth 1 | sed "s|${home}/||" | sort)"
  if [[ ${MACOS} -eq 1 ]]; then
    # A test cannot redirect AppConfigLocation on macOS: it is built from the home
    # directory Foundation reports, which no variable the test process sets moves -
    # qputenv("XDG_CONFIG_HOME") included, since macOS QStandardPaths never reads
    # it. So "the sandbox stayed empty" is not a property any test here can have,
    # and requiring it would pass by describing the platform rather than the test.
    # What is checkable is that the sandbox holds the store rather than the real
    # home does, which is what the ctest environment relies on, so that is what is
    # required: sweep 1 above says the real home is clean, and a store inside the
    # sandbox says it is clean because the sandbox caught it.
    if [[ -d "${home}/Library/Preferences/${name}" ]]; then
      CONTAINED=$((CONTAINED + 1))
    fi
    stray="$(printf '%s\n' "${written}" | grep -vE '^Library($|/)' | grep -v '^$')"
    if [[ -n "${stray}" ]]; then
      echo "FAIL: ${name} wrote outside the config root of the home directory it was given:" >&2
      echo "${stray}" | sed 's|^|    ~/|' >&2
      FAILURES=$((FAILURES + 1))
    fi
  elif [[ -n "${written}" ]]; then
    echo "FAIL: ${name} wrote into the home directory of whoever runs it:" >&2
    echo "${written}" | sed 's|^|    ~/|' >&2
    FAILURES=$((FAILURES + 1))
  fi

  if [[ ${status} -ne 0 ]]; then
    # A binary that died before it wrote anything would otherwise read as clean
    echo "FAIL: ${name} exited ${status}; see ${WORK_DIR}/${name}.log" >&2
    cat "${WORK_DIR}/${name}.log" >&2
    FAILURES=$((FAILURES + 1))
  fi
done

# A pattern that matches nothing would otherwise report a clean sweep
if [[ ${CANDIDATES} -lt 5 || ${CHECKED} -lt 4 ]]; then
  echo "FAIL: ${CANDIDATES} candidate(s) found and ${CHECKED} run, so this would pass whatever they write" >&2
  FAILURES=$((FAILURES + 1))
fi

# Likewise on macOS: with nothing caught in a sandbox, sweep 1 finding the real
# home clean says only that nothing wrote anywhere this run could see
if [[ ${MACOS} -eq 1 && ${CONTAINED} -eq 0 && ${FAILURES} -eq 0 ]]; then
  echo "FAIL: no candidate's credential store turned up under the home directory it was given," >&2
  echo "      so CFFIXED_USER_HOME did not move AppConfigLocation and a clean home proves nothing." >&2
  echo "      If these tests now redirect on macOS as well, this check needs rewriting rather than" >&2
  echo "      deleting - sweep 1 is still what keeps the home clean." >&2
  FAILURES=$((FAILURES + 1))
fi

if [[ ${FAILURES} -gt 0 ]]; then
  echo "${FAILURES} failure(s)" >&2
  exit 1
fi

echo "${CANDIDATES} credential-store tests left the home directory alone, ${CHECKED} of them run under one of their own"
