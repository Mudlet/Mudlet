# Map fixtures

## `minimal-map.xml`

Hand-written. The smallest document that reaches every Lua-observable branch of
`XMLimport::readMap()` - see the comment at the top of the file itself.

## `stale-label-keys.dat`

A format 20 binary map from a build whose save wrote its file-only keys into the
live area user data, so the file carries them for a label that no longer exists.
Its one area, `StaleLabelKeysArea`, holds one room, a label `Kept Label` (id 0)
with a red outline, and this user data:

| Key | Value |
| --- | --- |
| `climate` | `temperate` |
| `system.labelFont_99` | `Sans\|10\|50\|0` |
| `system.labelOutlineColor_99` | `1\|2\|3\|255` |
| `system.labelOutlineColor_0` | `200\|10\|10\|255` |
| `system.fallback_map2DZoom` | the area's zoom |

Label 0 claims its outline key and the zoom key is read as the zoom, so loading
it should leave only `climate` behind.

## `phantom-room-hashes.dat`

A binary map from a build whose `setRoomIDbyHash()` accepted any room id and an
empty hash. Its one area, `PhantomHashArea`, holds rooms 1 and 2, and its hash
table holds:

| Hash | Room |
| --- | --- |
| `phantom-hash` | 999999, which does not exist |
| (empty) | 1 |
| `real-hash` | 2 |

Loading it should leave only `real-hash` behind.

## `cross-platform-map.dat` and `cross-platform-map.json`

A binary map saved by desktop Mudlet at its default format version, and its
`saveJsonMap()` export. `build-cross-platform-map.lua` makes both: two areas,
five rooms, normal and special exits (some locked), an exit weight, a door, a
stub, a room weight, a locked room, a hidden room, a custom environment colour, a
symbol and its colour, a room border colour and thickness, a room hash, a custom
exit line, a label with a font and an outline colour, an area in grid mode, an
area's 2D zoom, and room, area and map user data. The JSON has LF line ends; its
`mapSymbolFontDetails` is cut to its first 16 fields, as `QFont::toString()` gains
fields across Qt versions; and a label's `image` becomes `["<png>"]`, or stays `[]`
for a label without one, as the browser build's PNG of the label comes out a few
bytes apart. `normalise-map-json.lua` does this, for the script, the spec's
comparison and the smoke test.

`Mapper_spec.lua` checks that the file loads to that JSON and that it is at the
format version Mudlet saves by default. `CI/wasm-headless-smoke.js` checks that
the WebAssembly build, the one 32-bit build CI runs, loads it to the same JSON, and
loads its own save of it back to the same JSON, so a field streamed with a size
that differs between 32 and 64 bits shows up there. Both then ask for the same
routes, which its locks, weights and special exit decide, the spec of the file
and the smoke test of its own save of it, so the browser's route finding is held
to desktop's.

The JSON is what gets compared, not a save's bytes: `saveMap()` writes rooms in
hash order, so those differ from one run to the next, while the export is
sorted. Changing the default map format version fails the spec until both
files are made again with a desktop build that has the change.

`TMap::mMaxVersion` can be raised above the default for development, and
`saveMap()` and the profile preferences then save that newer format too. The
script then also writes `cross-platform-map-v<N>.dat` and
`cross-platform-map-v<N>.json`, the map as built, saved at version N, and its
export, which the spec and the smoke test check as they do the default pair. It
finds N by saving at each newer version until `saveMap()` refuses one, which
prints an `[ ERROR ]` line. Raising `mMaxVersion` fails the spec until the files
are made again with a desktop build that has the change; made again once
`mMaxVersion` is back at the default, the pair is removed.

Any intended change to what `loadMap()` restores or `saveJsonMap()` writes also
needs the files remade; the golden records weight 900 for the symbol font until
Mudlet issue #12913 is fixed. The JSON files hold the translated names of the
default and unnamed areas, so the specs leave these tests pending in any language
but `en_US`, and the files are made in it, from the repository root:

```bash
config=$(mktemp -d)
printf '[General]\ninterfaceLanguage=en_US\n' > "$config/Mudlet.ini"
build/src/mudlet-headless --config-dir "$config" -p MapFixture --host localhost --port 23 --offline \
  --no-stdin -e 'dofile("src/mudlet-lua/tests/fixtures/maps/build-cross-platform-map.lua")' -e 'closeMudlet()'
```

## `achaea-map.zip`

A verbatim snapshot of the MMP map Iron Realms publishes for Achaea, used by
`Mapper_spec.lua` for the routing behaviour a two-room fixture cannot reach:
routes that cross area boundaries, one-way exits, hidden exits, and the
weight-driven rerouting that only shows up once there is a second route to
compare against.

| | |
| --- | --- |
| Source | <https://www.achaea.com/maps/map.xml> |
| Retrieved | 2026-08-24 |
| Contained file | `achaea-map.xml`, 5,756,152 bytes |
| SHA-256 of that file | `66fe3a643a20038f91fe5af15123d20b0679dd6c18553e681ec009328c557520` |

Rebuild the archive with:

```bash
curl -o achaea-map.xml https://www.achaea.com/maps/map.xml
zip -9 -X achaea-map.zip achaea-map.xml
```

Replacing it with a fresh download is not a like-for-like swap - Achaea's world
changes, so room ids come and go and the step counts the specs pin move with
them. Treat a refresh as a change to the specs too.

### What is in it

22,854 rooms over 379 areas, 56,864 usable directed edges once imported, all
twelve directions including `in` and `out`, 22 z-levels, 2,935 doors and 583
hidden exits (which import as locked doors). No special exits and no exit or
room weights: MMP carries neither, so every edge costs 1 until a spec sets a
weight itself.

Two properties of real IRE map data that the specs lean on:

- It is not one connected graph. 258 undirected components, the largest holding
  11,890 rooms, because the game joins the rest by ship, portal and other things
  MMP has no way to spell. So "no route exists" is reachable between two
  perfectly ordinary city rooms.
- 646 of its exits point at rooms the file does not contain. The importer
  removes them and says so, which is why importing this map raises the
  "at least one thing was detected" notice and writes a ~230KB report to the
  profile's `log/errors.txt`. That is the fixture working, not failing.

### Why it is zipped

Compressed it is 633KiB; the XML inside is 5.5MiB. That difference is paid per
checkout - and, because `src/CMakeLists.txt` installs this directory so the
specs travel with a distribution package, once more in every such package.
`Mapper_spec.lua` unpacks it into the profile directory when it needs it, which
measured 12ms.

An import of it costs about 1.3s and the first `getPath()` afterwards about
135ms more, of which 123ms is the `initGraph()` rebuild.
