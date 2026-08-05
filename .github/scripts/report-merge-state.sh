#!/usr/bin/env bash
#
# Publishes whether pull requests still merge cleanly, as a commit status on
# each one's head commit. See .github/workflows/merge-conflict-check.yml for why
# this is needed and which event contexts it runs in.
#
# Inputs, all from the environment:
#   REPO      owner/name of the repository (required)
#   BASE_REF  branch the pull requests are merging into (required)
#   PR_NUMBER a single pull request to report on; all open ones when unset
#   DRY_RUN   when non-empty, print the statuses instead of posting them
#   GH_TOKEN  token for the gh calls

set -euo pipefail

: "${REPO:?REPO must be set}"
: "${BASE_REF:?BASE_REF must be set}"

readonly STATUS_CONTEXT='no merge conflicts'
# GitHub works out mergeability in the background: the first read of a pull
# request usually answers null and starts the job, and a push to the base branch
# invalidates the answer for every open pull request at once. So ask more than
# once, but only about the ones still undecided, to keep a large backlog from
# multiplying the wait.
readonly MAX_ROUNDS=5
readonly ROUND_DELAY_SECONDS=10

if [ -n "${PR_NUMBER:-}" ]; then
  pull_requests=("${PR_NUMBER}")
else
  mapfile -t pull_requests < <(gh api --paginate \
    "repos/${REPO}/pulls?state=open&base=${BASE_REF}&per_page=100" --jq '.[].number')
fi

if [ "${#pull_requests[@]}" -eq 0 ]; then
  echo "No open pull requests against ${BASE_REF}"
  exit 0
fi

declare -A mergeable_of head_sha_of
undecided=("${pull_requests[@]}")

for ((round = 1; round <= MAX_ROUNDS; round++)); do
  still_undecided=()

  for pr in "${undecided[@]}"; do
    read -r pr_state mergeable head_sha < <(gh api "repos/${REPO}/pulls/${pr}" \
      --jq '"\(.state) \(.mergeable) \(.head.sha)"')
    head_sha_of["${pr}"]="${head_sha}"

    if [ "${pr_state}" != "open" ]; then
      # A closed pull request never gets a mergeability answer, so waiting for
      # one would spend every round for nothing. Only PR_NUMBER can reach here,
      # by the pull request closing between the event and this run.
      mergeable_of["${pr}"]="${pr_state}"
    elif [ "${mergeable}" = "null" ]; then
      still_undecided+=("${pr}")
    else
      mergeable_of["${pr}"]="${mergeable}"
    fi
  done

  undecided=("${still_undecided[@]}")
  if [ "${#undecided[@]}" -eq 0 ]; then
    break
  fi

  if [ "${round}" -lt "${MAX_ROUNDS}" ]; then
    echo "Merge state not ready yet for: ${undecided[*]}"
    sleep "${ROUND_DELAY_SECONDS}"
  fi
done

# What "Details" on the status leads to: the run that decided it
target_url="${GITHUB_SERVER_URL:-https://github.com}/${REPO}"
if [ -n "${GITHUB_RUN_ID:-}" ]; then
  target_url="${target_url}/actions/runs/${GITHUB_RUN_ID}"
fi

for pr in "${pull_requests[@]}"; do
  case "${mergeable_of[${pr}]:-undecided}" in
    true)
      state=success
      description="Merges cleanly into ${BASE_REF}"
      ;;
    false)
      state=failure
      description="Conflicts with ${BASE_REF}, so the build and test checks cannot run"
      ;;
    closed)
      echo "#${pr} is closed, so there is no merge state to report"
      continue
      ;;
    *)
      # Left as pending rather than guessed at, so nothing reads as reviewed
      # when it has not been. The next push to the pull request or to the base
      # branch runs this again.
      state=pending
      description="GitHub has not worked out the merge state yet"
      echo "::warning::Gave up waiting for GitHub to say whether #${pr} merges cleanly into ${BASE_REF}"
      ;;
  esac

  echo "#${pr} ${head_sha_of[${pr}]} -> ${state}: ${description}"

  if [ -n "${DRY_RUN:-}" ]; then
    continue
  fi

  gh api --method POST "repos/${REPO}/statuses/${head_sha_of[${pr}]}" \
    -f "state=${state}" \
    -f "context=${STATUS_CONTEXT}" \
    -f "description=${description}" \
    -f "target_url=${target_url}" \
    --silent
done
