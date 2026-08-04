#!/bin/sh
# Rebuilds the .mpackage fixtures used by Package_spec.lua from the directories
# under sources/. Run it after editing any fixture source, then commit both the
# source and the rebuilt archive.
#
# The archives are committed rather than zipped at spec runtime so the specs do
# not depend on a zip tool being installed on every platform CI runs busted on.
# Each fixture is staged in a temporary folder where the timestamps are forced,
# and -X drops the platform-specific extra fields, so rebuilding from unchanged
# sources produces a byte-identical archive without disturbing the sources.
set -eu

cd "$(dirname "$0")"
outputDirectory=$(pwd)

command -v zip >/dev/null 2>&1 || { echo "zip is not installed" >&2; exit 1; }

# Any fixed date does; this one is the day the fixture kit was added.
timestamp=202608040000.00

for source in sources/*/; do
	name=$(basename "$source")
	# mudlet-spec-xmlonly is installed straight from its .xml file, it has no archive
	if [ "$name" = "mudlet-spec-xmlonly" ]; then
		continue
	fi
	archive="$outputDirectory/$name.mpackage"
	staging=$(mktemp -d)
	cp -R "$source." "$staging"
	find "$staging" -exec touch -t "$timestamp" {} +
	rm -f "$archive"
	(cd "$staging" && zip -q -r -X -9 "$archive" .)
	rm -rf "$staging"
	echo "built $name.mpackage"
done
