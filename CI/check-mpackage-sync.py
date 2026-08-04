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

# packages built from the loose sources next to them - the two must agree
SOURCED_PACKAGES = [
    "src/mudlet-lua/lua/base-ui/mudlet-base-ui.mpackage",
    "src/mudlet-lua/lua/generic-mapper/generic_mapper.mpackage",
    "src/mudlet-lua/lua/gui-drop/gui-drop.mpackage",
]

# packages the repository syncs weekly, where mpkg needs a version bump to
# offer the update - see update-core-packages.yml in the package repository
PUBLISHED_PACKAGES = [
    "src/deleteOldProfiles.mpackage",
    "src/echo.mpackage",
    "src/enable-accessibility.mpackage",
    "src/mudlet-lua/lua/base-ui/mudlet-base-ui.mpackage",
    "src/mudlet-lua/lua/generic-mapper/generic_mapper.mpackage",
    "src/run-lua-code.mpackage",
]

errors = []
warnings = []


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


def check_sources_match(path, members, enforced):
    """Every member with a file of the same name beside the archive must match it."""
    for name, packaged in members.items():
        source = Path(path).parent / name
        if not source.is_file():
            continue
        if source.read_bytes() == packaged:
            continue
        complaint = f"{path} does not match {source} - rebuild the archive after editing the source"
        (errors if enforced else warnings).append(complaint)


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

    for path in sorted(set(SOURCED_PACKAGES) | set(PUBLISHED_PACKAGES)):
        if not Path(path).is_file():
            errors.append(f"{path} is listed in {__file__} but does not exist")
            continue

        members = contents(path)
        check_sources_match(path, members, enforced=path in SOURCED_PACKAGES)
        if arguments.base_ref and path in PUBLISHED_PACKAGES:
            check_version_bumped(path, members, arguments.base_ref)

    for warning in warnings:
        print(f"warning: {warning}")
    for error in errors:
        print(f"error: {error}")

    if errors:
        print(f"\n{len(errors)} problem(s) found. Rebuild an archive with:")
        print("  cd <package directory> && zip -r <name>.mpackage config.lua <name>.xml .mudlet")
        return 1

    print("mpackage archives match their sources.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
