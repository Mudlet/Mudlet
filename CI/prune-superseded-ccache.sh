#!/bin/bash
# Deletes every ccache entry for one build but its newest, so superseded saves
# don't sit in the 10GB cache quota until GitHub's LRU eviction drops them -
# which is as likely to drop a current entry that only development builds restore.
# Usage: prune-superseded-ccache.sh <ccache_id>
# Needs GH_TOKEN with actions: write, GITHUB_REPOSITORY and CACHE_REF.

set -o pipefail

prefix="ccache-${1:?usage: $0 <ccache_id>}-sha-"

# Overlapping builds can save out of order, so keep the newest commit's entry
# rather than the newest save; older commits fall back to save order
recent="$(gh api "repos/${GITHUB_REPOSITORY}/commits?sha=${CACHE_REF}&per_page=100" --jq '[.[].sha]')" || exit 1
# jq -s rather than gh's --slurp: --paginate applies --jq per page, and the
# sort has to see every page at once
gh api --paginate "repos/${GITHUB_REPOSITORY}/actions/caches?per_page=100&ref=${CACHE_REF}&key=${prefix}" \
  --jq '.actions_caches[]' \
  | jq -rs --argjson recent "${recent}" --arg prefix "${prefix}" '
      map(select(.key | startswith($prefix) and test("-sha-[0-9a-f]{40}$")))
      | sort_by([-(. as $c | $recent | index($c.key[-40:]) // 100), .created_at])
      | .[:-1][]
      | "\(.id) \(.key)"' \
  | while read -r id key; do
      echo "deleting ${key}"
      # a 404 means it is already gone, which is the wanted end state
      response="$(gh api --method DELETE "repos/${GITHUB_REPOSITORY}/actions/caches/${id}" 2>&1)" \
        || [[ "${response}" == *"HTTP 404"* ]] \
        || echo "::warning::could not delete ${key}: ${response}"
    done
