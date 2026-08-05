#!/usr/bin/env bash
###########################################################################
#   Copyright (C) 2026 by Vadim Peretokin - vadim.peretokin@mudlet.org    #
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
#   This program is distributed in the hope that it will be useful,       #
#   but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#   GNU General Public License for more details.                          #
#                                                                         #
#   You should have received a copy of the GNU General Public License     #
#   along with this program; if not, write to the                         #
#   Free Software Foundation, Inc.,                                       #
#   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
###########################################################################
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
#
# MERGE_STATE_MAX_ROUNDS and MERGE_STATE_ROUND_DELAY_SECONDS override the
# polling shape, which is how the tests keep themselves quick.

set -euo pipefail

: "${REPO:?REPO must be set}"
: "${BASE_REF:?BASE_REF must be set}"

readonly STATUS_CONTEXT='no merge conflicts'
# GitHub works out mergeability in the background: the first read of a pull
# request usually answers null and starts the job, and a push to the base branch
# invalidates the answer for every open pull request at once. So ask more than
# once, but only about the ones still undecided, to keep a large backlog from
# multiplying the wait.
readonly MAX_ROUNDS="${MERGE_STATE_MAX_ROUNDS:-5}"
readonly ROUND_DELAY_SECONDS="${MERGE_STATE_ROUND_DELAY_SECONDS:-10}"

failures=0
reported=0

if [ -n "${PR_NUMBER:-}" ]; then
  pull_requests=("${PR_NUMBER}")
else
  # Captured before it is split up, because a process substitution swallows the
  # exit status of what it runs: a failed listing would otherwise look like a
  # repository with nothing open, report on nothing, and leave every stale
  # status in place - the false all-clear this workflow exists to prevent.
  if ! pr_list=$(gh api --paginate \
      "repos/${REPO}/pulls?state=open&base=${BASE_REF}&per_page=100" --jq '.[].number'); then
    echo "::error::Could not list the open pull requests against ${BASE_REF}"
    exit 1
  fi
  mapfile -t pull_requests <<< "${pr_list}"

  # Invalidating the cached mergeability lags behind the push webhook that
  # starts this run, so reading straight away can still return the answer from
  # before the push - a clean bill of health for the pull request it just broke.
  sleep "${ROUND_DELAY_SECONDS}"
fi

# mapfile over empty input leaves one empty element rather than none
if [ "${#pull_requests[@]}" -eq 0 ] || [ -z "${pull_requests[0]}" ]; then
  echo "No open pull requests against ${BASE_REF}"
  exit 0
fi

declare -A mergeable_of head_sha_of
undecided=("${pull_requests[@]}")

for ((round = 1; round <= MAX_ROUNDS; round++)); do
  still_undecided=()

  for pr in "${undecided[@]}"; do
    # One unreadable pull request must not cost the others their refresh, so it
    # goes back in the queue and only counts as a failure if it never reads
    if ! details=$(gh api "repos/${REPO}/pulls/${pr}" --jq '"\(.state) \(.mergeable) \(.head.sha)"'); then
      echo "::warning::Could not read pull request #${pr}, will try again"
      still_undecided+=("${pr}")
      continue
    fi

    read -r pr_state mergeable head_sha <<< "${details}"
    head_sha_of["${pr}"]="${head_sha}"

    if [ "${pr_state}" != "open" ]; then
      # A closed pull request never gets a mergeability answer, so waiting for
      # one would spend every round for nothing. Either path can land here: the
      # pull request closing between the event and this run, or between the
      # listing and this read.
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
  if [ -z "${head_sha_of[${pr}]:-}" ]; then
    echo "::error::Never managed to read pull request #${pr}, so its merge state goes unreported"
    failures=$((failures + 1))
    continue
  fi

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
      reported=$((reported + 1))
      continue
      ;;
    *)
      # Left pending rather than guessed at, so nothing reads as checked when it
      # has not been. The next push to the pull request or to the base branch
      # runs this again.
      state=pending
      description="GitHub has not worked out the merge state yet"
      echo "::warning::Gave up waiting for GitHub to say whether #${pr} merges cleanly into ${BASE_REF}"
      ;;
  esac

  echo "#${pr} ${head_sha_of[${pr}]} -> ${state}: ${description}"

  if [ -n "${DRY_RUN:-}" ]; then
    reported=$((reported + 1))
    continue
  fi

  if gh api --method POST "repos/${REPO}/statuses/${head_sha_of[${pr}]}" \
      -f "state=${state}" \
      -f "context=${STATUS_CONTEXT}" \
      -f "description=${description}" \
      -f "target_url=${target_url}" \
      --silent; then
    reported=$((reported + 1))
  else
    echo "::error::Could not post the ${state} status for #${pr}"
    failures=$((failures + 1))
  fi
done

# Reconciling the two counts is what turns any future "quietly did nothing" back
# into a red job, which is the whole point of this script
if [ "${reported}" -ne "${#pull_requests[@]}" ] || [ "${failures}" -gt 0 ]; then
  echo "::error::Reported the merge state of ${reported} of ${#pull_requests[@]} pull requests, with ${failures} failure(s)"
  exit 1
fi

echo "Reported the merge state of ${reported} pull request(s)"
