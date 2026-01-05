# Fix OSC BEL Terminator Support (#3912)

## Problem

When a game sends OSC escape sequences terminated with BEL (0x07) instead of ST (ESC \), Mudlet's parser waits indefinitely for a terminator that never comes. All subsequent output is swallowed until the user reconnects.

Example: `ESC ] 2 ; Window Title BEL` (xterm-style window title)

## Solution

1. **Add BEL as valid terminator**: Modify the OSC terminator loop to recognize BEL (0x07) in addition to ST (ESC \)

2. **Add length limit**: Abort OSC parsing if sequence exceeds 4096 bytes without a terminator (defense against malformed sequences)

## Implementation

### Location

`src/TBuffer.cpp`, lines 850-870 (OSC handling in `translateToPlainText`)

### Changes

**Add constant** (near other constants):
```cpp
constexpr size_t MAX_OSC_SEQUENCE_LENGTH = 4096;
```

**Modify terminator loop** (lines 850-854):
- Add BEL check: `localBuffer[spanEnd] != '\x07'`
- Add length check: `(spanEnd - spanStart) < MAX_OSC_SEQUENCE_LENGTH`

**Modify post-loop logic** (lines 856-870):
- If BEL found: process OSC, advance past BEL
- If ST found: existing behavior
- If length exceeded: abort OSC mode, log warning, continue processing
- If incomplete (under limit): existing behavior (wait for more data)

### Key Details

- BEL-terminated sequence content: `spanStart` to `spanEnd` (excludes BEL)
- ST-terminated sequence content: `spanStart` to `spanEnd - 1` (excludes ESC before \)
- Unsupported OSC codes (like `2` for window title) are already logged and ignored in `decodeOSC()`

## Testing

### Manual Test

1. Connect to `mud.theforestsedge.com:4000`
2. Run: `color ansi`, `opt window.name`, `look`
3. Verify room description displays (previously swallowed)

### Unit Tests

| Case | Input | Expected |
|------|-------|----------|
| BEL-terminated OSC 2 | `\x1b]2;Title\x07` | Ignored, output continues |
| BEL-terminated OSC P | `\x1b]P0FF0000\x07` | Color redefined |
| ST-terminated OSC P | `\x1b]P0FF0000\x1b\\` | Color redefined |
| Exceeds length limit | `\x1b]2;` + 5000 chars | Aborted, no hang |
| Split across packets | `\x1b]P0FF00` / `00\x07` | Reassembled correctly |
| Text after BEL OSC | `\x1b]2;Title\x07Hello` | "Hello" displayed |

## Risk Assessment

- **Low risk**: Additive change to existing terminator logic
- **Backward compatible**: ST-terminated sequences unchanged
- **Well-scoped**: Single function modified
