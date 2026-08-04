#!/usr/bin/env python3
"""Check packaged .mpackage archives against their checked-in sources.

Mudlet installs the .mpackage archive, not the loose config.lua/.xml files
sitting next to it, so editing a source file without rebuilding the archive
silently changes nothing at all.

Some of these packages are also published to the package repository
(Mudlet/mudlet-package-repository), which offers updates by comparing the
version in config.lua. A content change that keeps the old version number
never reaches players who installed the package with mpkg.

Run with no arguments to check archive contents. Pass --base-ref to also
require a version bump for any package whose contents changed.
"""

import argparse
import io
import subprocess
import sys
import zipfile
from pathlib import Path

# every default package lives in its own directory under src/packages, built
# from the loose sources next to it - the archive and those sources must agree.
# mpkg is the exception: it is maintained upstream in the package repository and
# synced in wholesale by update-3rdparty.yml, so it has no sources of ours.
PACKAGES = sorted(str(path) for path in Path("src/packages").glob("*/*.mpackage"))

# packages the package repository syncs weekly, where mpkg needs a version bump
# to offer the update - see update-core-packages.yml over there
PUBLISHED_PACKAGES = [
    "src/packages/deleteOldProfiles/deleteOldProfiles.mpackage",
    "src/packages/echo/echo.mpackage",
    "src/packages/enable-accessibility/enable-accessibility.mpackage",
    "src/packages/generic_mapper/generic_mapper.mpackage",
    "src/packages/mudlet-base-ui/mudlet-base-ui.mpackage",
    "src/packages/run-lua-code/run-lua-code.mpackage",
]

errors = []


def contents(archive):
    """Map member name to bytes, ignoring directory entries."""
    with zipfile.ZipFile(archive) as package:
        return {member.filename: package.read(member) for member in package.infolist() if not member.is_dir()}


def version_of(members):
    config = members.get("config.lua", b"").decode("utf-8", "replace")
    for line in config.splitlines():
        name, _, value = line.partition("=")
        if name.strip() == "version":
            return value.strip().strip("[]\"' ")
    return None


def as_number(version):
    """Sortable form of a version, tolerating parts like "2" or "1.0.0rc1"."""
    return [(int("".join(filter(str.isdigit, part)) or 0), part) for part in version.split(".")]


def at_base_ref(path, base_ref):
    """Archive contents at base_ref, or None if the package is new there."""
    result = subprocess.run(["git", "show", f"{base_ref}:{path}"], capture_output=True)
    if result.returncode != 0:
        return None
    return contents(io.BytesIO(result.stdout))


def check_sources_match(path, members):
    """Every member with a file of the same name beside the archive must match it."""
    for name, packaged in members.items():
        source = Path(path).parent / name
        if source.is_file() and source.read_bytes() != packaged:
            errors.append(f"{path} does not match {source} - rebuild the archive after editing the source")


def check_version_bumped(path, members, base_ref):
    was = at_base_ref(path, base_ref)
    if was is None or was == members:
        return

    old, new = version_of(was), version_of(members)
    if not new:
        errors.append(f"{path} has no version in its config.lua")
    elif old and as_number(new) <= as_number(old):
        errors.append(f"{path} changed but is still version {new} - bump it so mpkg offers the update")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-ref", help="branch to compare against, e.g. origin/development")
    arguments = parser.parse_args()

    for path in PUBLISHED_PACKAGES:
        if path not in PACKAGES:
            errors.append(f"{path} is listed as published but is not in src/packages")

    for path in PACKAGES:
        members = contents(path)
        check_sources_match(path, members)
        if arguments.base_ref and path in PUBLISHED_PACKAGES:
            check_version_bumped(path, members, arguments.base_ref)

    for error in errors:
        print(f"error: {error}")

    if errors:
        print(f"\n{len(errors)} problem(s) found. Rebuild an archive from its sources with:")
        print("  cd src/packages/<name> && zip <name>.mpackage config.lua <name>.xml")
        return 1

    print(f"{len(PACKAGES)} mpackage archives match their sources.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
