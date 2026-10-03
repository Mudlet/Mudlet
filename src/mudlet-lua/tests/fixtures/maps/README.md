# Map fixtures

## `minimal-map.xml`

Hand-written. The smallest document that reaches every Lua-observable branch of
`XMLimport::readMap()` - see the comment at the top of the file itself.

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
