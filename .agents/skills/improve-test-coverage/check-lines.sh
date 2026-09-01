#!/bin/bash
# Prints the per-line hit counts gcovr recorded for a file's line range, so a
# specific code path can be checked rather than a whole-file percentage.
#
# Usage: check-lines.sh <coverage.json> <src/File.cpp> <firstLine> <lastLine>
set -euo pipefail

json="$1"
file="$2"
first="$3"
last="$4"

jq -r --arg f "$file" --argjson a "$first" --argjson b "$last" '
  .files[] | select(.file == $f) | .lines[]
  | select(.line_number >= $a and .line_number <= $b)
  | "\(.line_number)\t\(.count)"
' "$json"
