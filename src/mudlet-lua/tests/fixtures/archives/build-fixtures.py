#!/usr/bin/env python3
# Rebuilds the zip archives used by the unzip() specs in LuaGlobal_spec.lua.
# Run it after editing an entry below, then commit the rebuilt archives.
#
# These are generated here rather than zipped from a source folder because the
# entries they are about - names that climb out of the destination, bare folder
# entries, and empty files - cannot be made with a zip tool from a directory.
# Every entry carries the same fixed date, so a rebuild is byte for byte.
import os
import zipfile

TIMESTAMP = (2026, 9, 25, 0, 0, 0)

FIXTURES = {
    # names that resolve outside the destination, next to ones that stay inside
    "unzip-escape.zip": [
        ("../escaped.txt", b"climbed one level\n"),
        ("inner/../../../escaped-deep.txt", b"climbed out through a folder\n"),
        ("/escaped-absolute.txt", b"absolute\n"),
        ("..\\escaped-backslash.txt", b"climbed with a Windows separator\n"),
        ("inner/../kept.txt", b"stays inside\n"),
        ("ok.txt", b"fine\n"),
    ],
    # empty files at the top level and inside a folder, beside real folder entries
    "unzip-empty.zip": [
        ("resources/", None),
        ("resources/nested/", None),
        ("resources/note.txt", b"note\n"),
        ("resources/empty-inside.txt", b""),
        ("unzip-spec-empty-top.txt", b""),
        ("config.lua", b"x\n"),
    ],
}


def build(path, entries):
    with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, data in entries:
            info = zipfile.ZipInfo(name, TIMESTAMP)
            if data is None:
                info.external_attr = 0o40755 << 16 | 0x10
                archive.writestr(info, b"")
            else:
                info.external_attr = 0o644 << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, data)
    print("built " + os.path.basename(path))


here = os.path.dirname(os.path.abspath(__file__))
for name, entries in FIXTURES.items():
    build(os.path.join(here, name), entries)
