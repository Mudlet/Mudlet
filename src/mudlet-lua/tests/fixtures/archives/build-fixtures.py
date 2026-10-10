#!/usr/bin/env python3
# Rebuilds the zip archives used by the unzip() specs in LuaGlobal_spec.lua.
# Run it after editing an entry below, then commit the rebuilt archives.
#
# Names that climb out of the destination and a wrong CRC cannot be made by
# zipping a folder, so every fixture is written entry by entry. Every entry
# carries the same fixed date, so a rebuild with the same zlib is byte for byte.
import os
import struct
import zipfile
import zlib

TIMESTAMP = (2026, 9, 25, 0, 0, 0)

FIXTURES = {
    # names that resolve outside the destination, next to ones that stay inside
    "unzip-escape.zip": [
        ("../escaped.txt", b"climbed one level\n"),
        ("inner/../../../escaped-deep.txt", b"climbed out through a folder\n"),
        ("/escaped-absolute.txt", b"absolute\n"),
        ("..\\escaped-backslash.txt", b"climbed with a Windows separator\n"),
        ("C:/escaped-drive.txt", b"absolute on a Windows drive\n"),
        ("C:escaped-drive-relative.txt", b"relative to a Windows drive\n"),
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
    # an entry named like the temporary file unzip() writes the next entry through
    "unzip-collide.zip": [
        ("data.bin.unzip-partial", b"an entry of its own\n"),
        ("data.bin", b"data\n"),
    ],
}

# entries whose recorded CRC is made wrong after the archive is written
CORRUPT = {
    "unzip-corrupt.zip": [
        ("config.lua", b"x\n"),
        ("broken.txt", b"the archive's CRC for this does not match\n"),
    ],
}
CORRUPT_ENTRY = "broken.txt"


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


def corrupt_crc(path, entries, name):
    data = dict(entries)[name]
    good = struct.pack("<I", zlib.crc32(data))
    bad = struct.pack("<I", zlib.crc32(data) ^ 0xFFFFFFFF)
    with open(path, "rb") as archive:
        raw = archive.read()
    # once in the entry's local header and once in the central directory
    assert raw.count(good) == 2, path
    with open(path, "wb") as archive:
        archive.write(raw.replace(good, bad))


here = os.path.dirname(os.path.abspath(__file__))
for name, entries in FIXTURES.items():
    build(os.path.join(here, name), entries)
for name, entries in CORRUPT.items():
    build(os.path.join(here, name), entries)
    corrupt_crc(os.path.join(here, name), entries, CORRUPT_ENTRY)
