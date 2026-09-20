#!/bin/bash
# Tests the key check in CI/linux-packages/publish, which is the only automated
# thing standing between a wrong CI/linux-packages/mudlet.asc and the package
# trust store of every Mudlet user on Linux.
#
# That file is published as the copy consumers pin to check the key they install
# against, and apt and dnf trust every key it holds. Checking only its
# fingerprint was not enough: a private key block, a second key appended to the
# real one, and a copy frozen before the key grew a signing subkey all passed,
# and the last one publishes a key that cannot verify what was just signed. Each
# of those is a case below.
#
# Keys are generated into a throwaway GNUPGHOME, so this needs no network and no
# secrets. It drives "publish sign" on an empty directory, which runs the check
# and then signs nothing, so no rpm tooling is needed either.
#
# Linux only - publish uses globstar, which needs bash 4, so on the bash 3.2 that
# macOS ships it exits before any of this is reached. test/CMakeLists.txt
# registers it accordingly.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGING_DIR="$(cd "${SCRIPT_DIR}/../../CI/linux-packages" && pwd)"

# Matches SKIP_RETURN_CODE in test/CMakeLists.txt, so a skip reads as one rather
# than as a pass - a zero exit here would report the only pre-merge key gate
# green having run none of it. Skipping is a local convenience either way: on CI
# nothing this needs is allowed to be missing.
SKIP=77

skip_or_fail() {
  if [ -n "${CI:-}${GITHUB_ACTIONS:-}" ]; then
    echo "FAIL: $1" >&2
    exit 1
  fi
  echo "SKIP: $1"
  exit "${SKIP}"
}

if ! command -v gpg > /dev/null 2>&1; then
  skip_or_fail "gpg is not installed"
fi

WORK_DIR="$(mktemp -d)"
trap 'gpgconf --homedir "${WORK_DIR}/gnupg" --kill all > /dev/null 2>&1; rm -rf "${WORK_DIR}"' EXIT

# gpg-agent's socket path has to fit in sun_path, so this cannot live somewhere
# deep - a long TMPDIR is the one thing that makes key generation fail here
export GNUPGHOME="${WORK_DIR}/gnupg"
install -d -m 700 "${GNUPGHOME}"

FAILURES=0
OUT="${WORK_DIR}/out.txt"

start_test() {
  echo "=== $1"
}

fail() {
  echo "    FAIL: $*" >&2
  FAILURES=$((FAILURES + 1))
}

new_key() {
  gpg --batch --pinentry-mode loopback --passphrase '' \
    --quick-generate-key "$1" rsa2048 sign never > /dev/null 2>&1 || return 1
  gpg --batch --with-colons --list-keys "$1" 2> /dev/null | awk -F: '/^fpr:/ {print $10; exit}'
}

if ! SIGNING_KEY="$(new_key 'Mudlet key gate test')" || [ -z "${SIGNING_KEY}" ]; then
  skip_or_fail "this gpg cannot generate a key here (no agent?)"
fi
OTHER_KEY="$(new_key 'Unrelated other key')"

# publish resolves mudlet.asc next to itself, so a copy in the work directory is
# what lets each case hand it a different one
cp "${PACKAGING_DIR}/publish" "${WORK_DIR}/publish"
mkdir -p "${WORK_DIR}/packages"

gate() {
  "${WORK_DIR}/publish" sign "${WORK_DIR}/packages" > "${OUT}" 2>&1
  STATUS=$?
}

assert_accepted() {
  gate
  if [ "${STATUS}" -ne 0 ]; then
    fail "expected the key to be accepted, got exit ${STATUS}:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

assert_refused() {
  gate
  if [ "${STATUS}" -eq 0 ]; then
    fail "the key was ACCEPTED - $1"
  elif ! grep -qF -- "$2" "${OUT}"; then
    fail "expected '$2' in the refusal:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

start_test "the key's own public half is accepted"
gpg --batch --armor --export "${SIGNING_KEY}" > "${WORK_DIR}/mudlet.asc"
assert_accepted

start_test "a different key is refused"
gpg --batch --armor --export "${OTHER_KEY}" > "${WORK_DIR}/mudlet.asc"
assert_refused "any key would be publishable" "is not the public half"

start_test "a missing mudlet.asc is refused, and says so"
rm -f "${WORK_DIR}/mudlet.asc"
assert_refused "publishing with no committed copy at all" "is missing"

start_test "a private key block is refused"
# The published file is served world-readable, so this one leaks the signing key
gpg --batch --armor --export-secret-keys "${SIGNING_KEY}" > "${WORK_DIR}/mudlet.asc"
assert_refused "the signing key would be published to everyone" "is not the public half"

start_test "the signing key plus an extra key is refused"
# apt and dnf trust every key in the file, so the extra one signs packages too
gpg --batch --armor --export "${SIGNING_KEY}" "${OTHER_KEY}" > "${WORK_DIR}/mudlet.asc"
assert_refused "an extra key would be trusted by every install" "is not the public half"

start_test "a copy frozen before the key grew a subkey is refused"
# gpg signs with the newest capable subkey, so a stale copy stops verifying
gpg --batch --armor --export "${SIGNING_KEY}" > "${WORK_DIR}/mudlet.asc"
gpg --batch --pinentry-mode loopback --passphrase '' \
  --quick-add-key "${SIGNING_KEY}" rsa2048 sign never > /dev/null 2>&1
assert_refused "the published key could not verify what was signed" "is not the public half"

start_test "a file that is not a key at all is refused"
echo "not a key" > "${WORK_DIR}/mudlet.asc"
assert_refused "garbage would be published as the key" "is not the public half"

# Guards the real file on every pull request, where nothing else does: the
# publishing workflow only runs the check when it actually publishes a release
start_test "the committed mudlet.asc is exactly one public key"
if ! head -n 1 "${PACKAGING_DIR}/mudlet.asc" | grep -qx -- "-----BEGIN PGP PUBLIC KEY BLOCK-----"; then
  fail "CI/linux-packages/mudlet.asc does not start with a public key block"
fi
PRIMARY_KEYS="$(gpg --batch --show-keys --with-colons "${PACKAGING_DIR}/mudlet.asc" 2> /dev/null | grep -c '^pub:')"
if [ "${PRIMARY_KEYS}" -ne 1 ]; then
  fail "CI/linux-packages/mudlet.asc holds ${PRIMARY_KEYS} keys, expected exactly 1"
fi

if [ "${FAILURES}" -ne 0 ]; then
  echo "${FAILURES} failure(s)" >&2
  exit 1
fi
echo "All linux-packages key checks passed"
