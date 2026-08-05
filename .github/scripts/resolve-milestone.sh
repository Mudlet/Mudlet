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
# Turns the version in .github/repo-metadata.yml into the number of the matching
# open milestone, and prints "<number> <title>".
#
# Milestone titles carry a suffix in practice - "4.23.0 next release" for a
# metadata value of "4.23.0" - so the exact-title match this replaces found
# nothing, set an empty number, and exited 0, leaving every pull request in the
# repository unassigned without ever failing (#9671). Anything short of exactly
# one match is now an error, so a renamed milestone cannot go unnoticed again.
#
# Diagnostics go to stderr, because stdout is this script's answer.
#
# Inputs, all from the environment:
#   REPO            owner/name of the repository (required)
#   NEXT_MILESTONE  version to look for (required)
#   GH_TOKEN        token for the gh call

set -euo pipefail

: "${REPO:?REPO must be set}"
: "${NEXT_MILESTONE:?NEXT_MILESTONE must be set}"

if ! milestones=$(gh api "repos/${REPO}/milestones?state=open&per_page=100"); then
  echo "::error::Could not read the open milestones of ${REPO}" >&2
  exit 1
fi

# An exact title wins outright; otherwise the version has to be the leading word
# of exactly one title, so a genuinely ambiguous set is refused rather than
# guessed at
matches=$(jq -c --arg wanted "${NEXT_MILESTONE}" '[.[] | select(.title == $wanted)]' <<< "${milestones}")
if [ "$(jq 'length' <<< "${matches}")" -eq 0 ]; then
  matches=$(jq -c --arg wanted "${NEXT_MILESTONE}" \
    '[.[] | select(.title | startswith($wanted + " "))]' <<< "${milestones}")
fi

match_count=$(jq 'length' <<< "${matches}")

if [ "${match_count}" -eq 0 ]; then
  echo "::error::No open milestone matches '${NEXT_MILESTONE}' from .github/repo-metadata.yml. Open milestones: $(jq -r '[.[].title] | join(", ")' <<< "${milestones}")" >&2
  exit 1
fi

if [ "${match_count}" -gt 1 ]; then
  echo "::error::'${NEXT_MILESTONE}' matches several open milestones, so which one to use is not clear: $(jq -r '[.[].title] | join(", ")' <<< "${matches}")" >&2
  exit 1
fi

jq -r '"\(.[0].number) \(.[0].title)"' <<< "${matches}"
