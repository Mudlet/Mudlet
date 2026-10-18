#!/bin/bash
# Cuts a changelog on stdin to <max-bytes> for GitHub, which refuses a release
# body over 125000 characters. Whole lines go from the end, where the changelog
# keeps its Infrastructure and Other sections, and a closing line says how many
# entries are missing and links to <more-url>. Bytes are never fewer than the
# characters GitHub counts, and LC_ALL=C makes every awk count bytes.
#
# Usage: fit-release-notes.sh <max-bytes> <more-url>

set -euo pipefail

MAX_BYTES="${1:-}"
MORE_URL="${2:-}"

if [ $# -ne 2 ] || ! [[ "${MAX_BYTES}" =~ ^[0-9]+$ ]] || [ -z "${MORE_URL}" ]; then
  echo "usage: $(basename "$0") <max-bytes> <more-url>" >&2
  exit 2
fi

LC_ALL=C awk -v max="${MAX_BYTES}" -v url="${MORE_URL}" '
  function footer(count) {
    return sprintf("_%d more %s not listed here, as GitHub limits how long release notes can be - see [every pull request merged in this period](%s)._", count, count == 1 ? "change is" : "changes are", url)
  }
  { line[NR] = $0; size += length($0) + 1 }
  END {
    if (size <= max) {
      for (i = 1; i <= NR; i++) print line[i]
      exit
    }

    entries = 0
    for (i = 1; i <= NR; i++) {
      if (line[i] ~ /^\\- /) entries++
    }

    # Sized for the most entries the footer could report, so keeping some never
    # makes it longer than was reserved for it - bar "1 ... change is", shorter
    # than the "0 ... changes are" that keeping that one entry would print
    reserve = length(footer(entries))
    if (length(footer(0)) > reserve) reserve = length(footer(0))
    budget = max - reserve - 2

    kept = 0
    used = 0
    for (i = 1; i <= NR; i++) {
      if (used + length(line[i]) + 1 > budget) break
      used += length(line[i]) + 1
      kept = i
    }
    # A heading left with none of its entries under it
    while (kept > 0 && (line[kept] == "" || line[kept] ~ /^#/)) kept--

    listed = 0
    for (i = 1; i <= kept; i++) {
      print line[i]
      if (line[i] ~ /^\\- /) listed++
    }
    if (kept > 0) print ""
    print footer(entries - listed)
  }
'
