# Image fixtures

| File | Used by | What it is |
| --- | --- | --- |
| `solid-magenta-4x4.png` | `MapViewportLimits_spec.lua`, `UI_spec.lua`, `Package_spec.lua` | A 4x4 opaque RGB PNG, every pixel `#ff00ff`, so a map image label made from it changes any image it is drawn into; the gui-drop specs drop it as an image file |

Regenerate `solid-magenta-4x4.png` with:

```bash
python3 - <<'EOF'
import struct, zlib
def chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)
row = b"\x00" + b"\xff\x00\xff" * 4
open("solid-magenta-4x4.png", "wb").write(b"\x89PNG\r\n\x1a\n"
    + chunk(b"IHDR", struct.pack(">IIBBBBB", 4, 4, 8, 2, 0, 0, 0))
    + chunk(b"IDAT", zlib.compress(row * 4, 9))
    + chunk(b"IEND", b""))
EOF
```
