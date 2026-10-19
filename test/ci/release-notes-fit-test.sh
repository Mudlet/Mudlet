#!/bin/bash
# Tests CI/fit-release-notes.sh and the release workflow's changelog step that uses
# it: over GitHub's size limit, the release is refused and nothing is published.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
FIT="${REPO_DIR}/CI/fit-release-notes.sh"
RELEASE_WORKFLOW="${REPO_DIR}/.github/workflows/create-github-release.yml"
URL="https://github.com/Mudlet/Mudlet/pulls?q=is%3Apr+is%3Amerged"

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "${WORK_DIR}"' EXIT

IN="${WORK_DIR}/in"
OUT="${WORK_DIR}/out"
FAILURES=0

start_test() {
  echo "=== $1"
}

fail() {
  echo "    FAIL: $*" >&2
  FAILURES=$((FAILURES + 1))
}

run_fit() {
  bash "${FIT}" "$@" < "${IN}" > "${OUT}" 2>&1
  echo $?
}

assert_status() {
  local expected="$1" actual="$2"
  # -ne on an empty status errors out and takes the false branch, turning every
  # assertion into a pass
  case "${actual}" in
    ''|*[!0-9]*)
      fail "expected exit ${expected}, got a non-numeric status '${actual}'"
      return
      ;;
  esac
  if [ "${actual}" -ne "${expected}" ]; then
    fail "expected exit ${expected}, got ${actual}, output:"
    sed 's/^/        /' "${OUT}" >&2
  fi
}

assert_contains() {
  local file="$1" needle="$2"
  if ! grep -qF -- "${needle}" "${file}"; then
    fail "expected '${needle}' in ${file}:"
    sed 's/^/        /' "${file}" >&2
  fi
}

assert_at_most_bytes() {
  local file="$1" limit="$2" size
  if [ ! -f "${file}" ]; then
    fail "${file} was not written"
    return
  fi
  size=$(wc -c < "${file}" | tr -d ' ')
  if [ "${size}" -gt "${limit}" ]; then
    fail "${file} is ${size} bytes, over the ${limit} allowed"
  fi
}

entries_in() {
  grep -c '^\\- ' "$1"
}

# The same shape CI/generate-changelog.lua writes: sections of "\- " entries,
# each followed by a blank line
write_changelog() {
  local per_section="$1" section i
  : > "${IN}"
  for section in Added Improved Fixed Infrastructure; do
    printf '##### %s:\n' "${section}" >> "${IN}"
    for i in $(seq 1 "${per_section}"); do
      printf '\\- %s change number %d [#%d](https://github.com/Mudlet/Mudlet/pull/%d)\n\n' \
        "${section}" "${i}" "${i}" "${i}" >> "${IN}"
    done
  done
}

start_test "notes that fit come back byte for byte"
write_changelog 20
assert_status 0 "$(run_fit 1000000 "${URL}")"
if ! cmp -s "${IN}" "${OUT}"; then
  fail "notes within the budget were changed"
fi

start_test "notes exactly at the budget are not shortened"
assert_status 0 "$(run_fit "$(wc -c < "${IN}" | tr -d ' ')" "${URL}")"
if ! cmp -s "${IN}" "${OUT}"; then
  fail "notes exactly at the budget were changed"
fi

start_test "notes over the budget are cut to it, from the end"
write_changelog 500
assert_status 0 "$(run_fit 20000 "${URL}")"
assert_at_most_bytes "${OUT}" 20000
# Everything but the closing line is the start of the input, untouched
sed '$d' "${OUT}" | sed '$d' > "${WORK_DIR}/kept"
if ! head -c "$(wc -c < "${WORK_DIR}/kept" | tr -d ' ')" "${IN}" | cmp -s - "${WORK_DIR}/kept"; then
  fail "the kept part is not the start of the notes"
fi
assert_contains "${OUT}" "##### Added:"
assert_contains "${OUT}" "${URL}"

start_test "the closing line counts exactly the entries that were left out"
omitted=$(( $(entries_in "${IN}") - $(entries_in "${OUT}") ))
assert_contains "${OUT}" "_${omitted} more changes are not listed here"

start_test "the cut never leaves a heading with none of its entries"
last_kept=$(sed '$d' "${OUT}" | sed '$d' | tail -1)
case "${last_kept}" in
  '\- '*) ;;
  *) fail "the last line kept is '${last_kept}', not an entry" ;;
esac
# Lands the budget just past a heading, where a cut by size alone would keep it
heading_end=$(grep -n '^##### Improved:' "${IN}" | cut -d: -f1)
heading_bytes=$(head -n "${heading_end}" "${IN}" | wc -c | tr -d ' ')
# A budget of 0 prints only the closing line; the room for it is that and a blank line
footer_room=$(( $(bash "${FIT}" 0 "${URL}" < "${IN}" | wc -c | tr -d ' ') + 1 ))
assert_status 0 "$(run_fit $((heading_bytes + footer_room)) "${URL}")"
if grep -q '^##### Improved:' "${OUT}"; then
  fail "the Improved heading was kept without any of its entries"
fi

start_test "a single entry left out reads as one change"
write_changelog 3
kept_bytes=$(wc -c < "${IN}" | tr -d ' ')
# Longer than the closing line, so the room that line needs costs only this entry
printf '\\- %s\n\n' "$(printf 'long%.0s' $(seq 1 200))" >> "${IN}"
assert_status 0 "$(run_fit $((kept_bytes + 400)) "${URL}")"
assert_contains "${OUT}" "_1 more change is not listed here"
assert_contains "${OUT}" "Fixed change number 3"
assert_at_most_bytes "${OUT}" $((kept_bytes + 400))

start_test "the budget is counted in bytes, so multibyte text cannot overshoot it"
: > "${IN}"
printf '##### Fixed:\n' >> "${IN}"
for i in $(seq 1 400); do
  printf '\\- SvenskMUD: färdas söderut, 地图 %d\n\n' "${i}" >> "${IN}"
done
# Under a UTF-8 locale, where an awk that counts characters would undercount
LC_ALL=C.UTF-8 bash "${FIT}" 5000 "${URL}" < "${IN}" > "${OUT}" 2> /dev/null
assert_status 0 "$?"
assert_at_most_bytes "${OUT}" 5000

start_test "a budget with no room for any entry still says where they are"
write_changelog 20
assert_status 0 "$(run_fit 10 "${URL}")"
assert_contains "${OUT}" "_80 more changes are not listed here"
if [ "$(wc -l < "${OUT}" | tr -d ' ')" -ne 1 ]; then
  fail "expected only the closing line"
fi

start_test "a changelog over GitHub's limit fits the workflow's limit"
write_changelog 600
if [ "$(wc -c < "${IN}" | tr -d ' ')" -le 125000 ]; then
  fail "the generated changelog is too small to exercise the cut"
fi
assert_status 0 "$(run_fit 120000 "${URL}")"
assert_at_most_bytes "${OUT}" 120000

start_test "a missing, surplus or malformed argument is a usage error, not a silent pass"
: > "${IN}"
assert_status 2 "$(run_fit)"
assert_status 2 "$(run_fit 1000)"
assert_status 2 "$(run_fit 1000 "")"
assert_status 2 "$(run_fit "" "${URL}")"
assert_status 2 "$(run_fit "12k" "${URL}")"
assert_status 2 "$(run_fit 1000 "${URL}" extra)"

# The rest runs the workflow's own changelog step, against a repository with the
# tags it looks for and a generator stubbed to print as much as asked
STEP="${WORK_DIR}/generate-changelog-step.sh"
awk '
  /^ *- name: Generate changelog$/ { in_step = 1; next }
  in_step && /^ *run: [|]$/ { match($0, /^ */); indent = RLENGTH + 2; in_step = 0; in_run = 1; next }
  in_run {
    if ($0 ~ /^ *$/) { print ""; next }
    match($0, /^ */)
    if (RLENGTH < indent) exit
    print substr($0, indent + 1)
  }
' "${RELEASE_WORKFLOW}" > "${STEP}"

STUB_DIR="${WORK_DIR}/bin"
mkdir -p "${STUB_DIR}"
cat > "${STUB_DIR}/lua" <<'STUB'
#!/bin/bash
# Stands in for CI/generate-changelog.lua: STUB_PTB_ENTRIES entries per section for
# a range starting at a PTB tag, STUB_ENTRIES for any other, or fails on STUB_FAIL
[ -z "${STUB_FAIL}" ] || exit 1
entries="${STUB_ENTRIES}" label="Full"
while [ $# -gt 0 ]; do
  if [ "$1" = "--start-commit" ]; then
    case "$2" in *-ptb-*) entries="${STUB_PTB_ENTRIES}" label="PTB" ;; esac
  fi
  shift
done
for section in Added Improved Fixed Infrastructure; do
  printf '##### %s:\n' "${section}"
  for i in $(seq 1 "${entries}"); do
    printf '\\- %s %s change number %d, long enough to look like a real one [#%d](https://github.com/Mudlet/Mudlet/pull/%d)\n\n' \
      "${label}" "${section}" "${i}" "${i}" "${i}"
  done
done
STUB
chmod +x "${STUB_DIR}/lua"

REPO="${WORK_DIR}/repo"
# A developer's own signing or hooks settings must not reach the scratch repository
repo_git() {
  git -C "${REPO}" -c commit.gpgSign=false -c tag.gpgSign=false -c core.hooksPath=/dev/null \
    -c user.name="Release notes test" -c user.email="test@example.com" "$@"
}
make_commit() {
  GIT_COMMITTER_DATE="$1" GIT_AUTHOR_DATE="$1" repo_git commit -q --allow-empty -m "$2"
}
git init -q --template= "${REPO}"
# As in Mudlet, the stable tag is on a release branch that forked from development
# before the tag was made, so the changelog since it starts with a development
# change older than the tag. The commits carry the offsets Mudlet's do, and the
# last is on the 17th in UTC
make_commit "2026-08-20T06:35:53+02:00" "Release branch point"
repo_git checkout -q -b release
make_commit "2026-09-02T22:31:46+02:00" "Release 5.0.1"
repo_git tag Mudlet-5.0.1
repo_git checkout -q -
make_commit "2026-08-25T12:00:00+02:00" "Development change made before the release"
make_commit "2026-10-15T09:00:00+02:00" "Previous PTB"
repo_git tag Mudlet-5.0.0-ptb-2026-10-15-aaaaaaa
make_commit "2026-10-18T01:00:00+02:00" "This PTB"
ln -s "${REPO_DIR}" "${REPO}/release-scripts"

# Usage: run_step <type> <tag> <entries> [ptb-entries] [fail]
# Away from UTC, so the step has to ask for UTC itself
run_step() {
  rm -f "${REPO}/changelog.md"
  (cd "${REPO}" && PATH="${STUB_DIR}:${PATH}" TZ="Asia/Tokyo" \
    STUB_ENTRIES="$3" STUB_PTB_ENTRIES="${4:-$3}" STUB_FAIL="${5:-}" RELEASE_TYPE="$1" RELEASE_TAG="$2" \
    GITHUB_SERVER_URL="https://github.com" GITHUB_REPOSITORY="Mudlet/Mudlet" bash -e "${STEP}") > "${OUT}" 2>&1
  echo $?
}

NOTES="${REPO}/changelog.md"
PRS="https://github.com/Mudlet/Mudlet/pulls?q=is%3Apr+is%3Amerged"
PTB_PRS="${PRS}+merged%3A%3E%3D2026-10-17T23%3A00%3A00Z+merged%3A%3C%3D2026-10-17"
FULL_PRS="${PRS}+merged%3A%3E%3D2026-08-25T10%3A00%3A00Z+merged%3A%3C%3D2026-10-17"

line_of() {
  grep -nF -- "$2" "$1" | head -1 | cut -d: -f1
}

start_test "the workflow's changelog step could be extracted whole"
if ! grep -q 'fit_notes' "${STEP}" || [ "$(grep -v '^$' "${STEP}" | tail -1)" != "head -20 changelog.md" ]; then
  fail "the Generate changelog step of create-github-release.yml was not found, or not to its end"
fi

start_test "a PTB's own changes are published whole while the full changelog gives way"
assert_status 0 "$(run_step ptb Mudlet-5.0.0-ptb-2026-10-18-bbbbbbb 600 5)"
assert_at_most_bytes "${NOTES}" 125000
assert_contains "${NOTES}" "#### Changes in this build since the previous PTB (Mudlet-5.0.0-ptb-2026-10-15-aaaaaaa)"
assert_contains "${NOTES}" "PTB Infrastructure change number 5,"
if grep -qF "${PTB_PRS}" "${NOTES}"; then
  fail "the PTB's own changes were cut although they fit"
fi
assert_contains "${NOTES}" "(${FULL_PRS})"
if [ "$(tail -1 "${NOTES}")" != "</details>" ]; then
  fail "the full changelog's <details> is not closed at the end"
fi

start_test "each changelog links to its own period when both are cut"
assert_status 0 "$(run_step ptb Mudlet-5.0.0-ptb-2026-10-18-bbbbbbb 600)"
assert_at_most_bytes "${NOTES}" 125000
ptb_link=$(line_of "${NOTES}" "(${PTB_PRS})")
details=$(line_of "${NOTES}" "<details>")
full_link=$(line_of "${NOTES}" "(${FULL_PRS})")
if [ -z "${ptb_link}" ] || [ -z "${details}" ] || [ -z "${full_link}" ]; then
  fail "expected a PTB link, then <details>, then a full changelog link"
  sed 's/^/        /' "${OUT}" >&2
elif [ "${ptb_link}" -gt "${details}" ] || [ "${full_link}" -lt "${details}" ]; then
  fail "the PTB link is on line ${ptb_link} and the full one on ${full_link}, around <details> on ${details}"
fi

start_test "the first PTB after a release stays under GitHub's limit"
repo_git tag -d Mudlet-5.0.0-ptb-2026-10-15-aaaaaaa > /dev/null
assert_status 0 "$(run_step ptb Mudlet-5.0.0-ptb-2026-10-18-bbbbbbb 600)"
assert_at_most_bytes "${NOTES}" 125000
assert_contains "${NOTES}" "(${FULL_PRS})"

start_test "a release's notes stay under GitHub's limit"
repo_git tag Mudlet-5.1.0
assert_status 0 "$(run_step release Mudlet-5.1.0 600)"
assert_at_most_bytes "${NOTES}" 125000
assert_contains "${NOTES}" "(${FULL_PRS})"

start_test "notes that fit are published whole"
assert_status 0 "$(run_step release Mudlet-5.1.0 3)"
assert_contains "${NOTES}" "Full Infrastructure change number 3,"
if grep -q 'more change' "${NOTES}"; then
  fail "notes well within the limit were cut"
fi

start_test "a release whose changelog cannot be generated fails rather than publishing it empty"
assert_status 1 "$(run_step release Mudlet-5.1.0 3 3 fail)"
assert_contains "${OUT}" "::error::Changelog generation failed"

start_test "a link that cannot be dated falls back to every merged pull request, with a warning"
FUNCTION="${WORK_DIR}/merged-prs-url.sh"
sed -n '/^merged_prs_url() {/,/^}/p' "${STEP}" > "${FUNCTION}"
url=$(cd "${REPO}" && GITHUB_SERVER_URL="https://github.com" GITHUB_REPOSITORY="Mudlet/Mudlet" \
  bash -e -c '. "$1" && merged_prs_url Mudlet-no-such-tag HEAD' _ "${FUNCTION}" 2> "${OUT}")
assert_status 0 "$?"
if [ "${url}" != "${PRS}" ]; then
  fail "expected the link to fall back to ${PRS}, got '${url}'"
fi
assert_contains "${OUT}" "::warning::Could not date Mudlet-no-such-tag..HEAD"

if [ "${FAILURES}" -gt 0 ]; then
  echo "${FAILURES} check(s) failed"
  exit 1
fi

echo "All checks passed"
