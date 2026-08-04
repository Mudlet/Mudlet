#!/bin/sh
# Rebuilds the .mpackage fixtures used by Package_spec.lua from the directories
# under sources/. Run it after editing any fixture source, then commit both the
# source and the rebuilt archive.
#
# The archives are committed rather than zipped at spec runtime so the specs do
# not depend on a zip tool being installed on every platform CI runs busted on.
# The timestamps are forced and -X drops the platform-specific extra fields, so
# rebuilding from unchanged sources produces a byte-identical archive.
set -eu

cd "$(dirname "$0")"

command -v zip >/dev/null 2>&1 || { echo "zip is not installed" >&2; exit 1; }

# Any fixed date does; this one is the day the fixture kit was added.
timestamp=202608040000.00

for source in sources/*/; do
	name=$(basename "$source")
	# mudlet-spec-xmlonly is installed straight from its .xml file, it has no archive
	if [ "$name" = "mudlet-spec-xmlonly" ]; then
		continue
	fi
	archive="$name.mpackage"
	rm -f "$archive"
	find "$source" -exec touch -t "$timestamp" {} +
	(cd "$source" && zip -q -r -X -9 "../../$archive" .)
	echo "built $archive"
done
