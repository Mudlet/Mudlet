#!/bin/bash
# Prints gcovr's per-line hit counts for a line range of one file.
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
