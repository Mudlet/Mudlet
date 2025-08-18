#!/bin/bash
###########################################################################
#   Copyright (C) 2024 by John McKisson - john.mckisson@gmail.com         #
#   Copyright (C) 2025 by Stephen Lyons - slysven@virginmedia.com         #
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

# This script reads the JSON feed for a given GitHub artifact with the given
# BUILD_COMMIT and registers it with dblsqd. It will attempt this for up to
# an hour, every minute until success.
# GHA Env vars needed:
#    BUILD_COMMIT - For listing the json feed
#    PATH - For path of dblsqd
#    VERSION_STRING - The version of Mudlet being released

# Version: 1.0.0    Initial Release
# Version: 1.1.0    Hard-code only for x86_64 architecture

# Exit codes:
# 0 - Everything is fine. 8-)
# 1 - Timeout exceeded, failure

echo "=== Downloading JSON feed ==="
json_url="https://make.mudlet.org/snapshots/json.php?commitid=${BUILD_COMMIT}"
echo "$json_url"

# Timeout in seconds before we give up
timeout_period=$((60 * 60))
# Time interval between retries in seconds
retry_interval=60
# Start time
start_time=$(date +%s)


# Return Codes:
# 0 - Success!
# 1 - curl failure
# 2 - json parse failure
# 3 - no matching URL found
FetchAndCheckURL() {
    json_data=$(curl -s "$json_url")

    # Check if curl succeeded
    if ! curl -s "$json_url" -o /dev/null; then
      echo "Failed to download JSON feed."
      return 1
    fi

    SEARCH_PATTERN="windows-64.exe"
    echo "Searching for ${SEARCH_PATTERN}"

    # Use jq to filter the JSON data
    matching_url=$(echo "$json_data" | jq -r --arg search_pattern "${SEARCH_PATTERN}" '.data[] | select(.platform == "windows" and (.url | test(${SEARCH_PATTERN}))) | .url')

    # Check if the URL was found
    if [ -z "$matching_url" ]; then
      echo "No matching URL found."
      return 3
    fi

    echo "Matching URL:"
    echo "$matching_url"

    return 0
}

# Loop until timeout or success
while true; do
    if FetchAndCheckURL; then
        echo "=== Found URL, proceeding with registration ==="
        break
    fi

    # Check if timeout period has been reached
    current_time=$(date +%s)
    elapsed_time=$((current_time - start_time))
    if [ $elapsed_time -ge $timeout_period ]; then
        echo "=== Timeout period reached. Exiting ==="
        exit 1
    fi

    echo "=== Retrying in ${retry_interval} seconds ==="
    sleep $retry_interval
done


echo "=== Registering release with Dblsqd ==="
echo "dblsqd push -a mudlet -c public-test-build -r \"${VERSION_STRING}\" -s mudlet --type 'standalone' --attach win:x86_64} \"${matching_url}\""

PATH="/c/Program Files/nodejs/:/c/npm/prefix/:${PATH}"
export PATH
dblsqd push -a mudlet -c public-test-build -r "${VERSION_STRING}" -s mudlet --type 'standalone' --attach win:x86_64 "${matching_url}"
