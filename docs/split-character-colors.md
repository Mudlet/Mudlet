# Split colours in legacy CJK text

A legacy double-byte character can carry different renditions on its left and
right halves. Mudlet accepts ANSI SGR sequences (`ESC [ … m`) between its two
encoded bytes. The rendition before the first byte paints the left half; the
rendition after the intervening SGR sequences paints the right half and remains
active for subsequent text. Foreground, background and SGR attributes are kept
independently. Rendering uses the existing font and character width settings.

Supported encodings:

- Big5 and Big5-HKSCS
- GBK and the two-byte characters in GB18030
- EUC-KR
- Shift-JIS (`SHIFT_JIS`) and the full-width two-byte characters in EUC-JP

UTF-8 is unchanged. GB18030 four-byte characters, EUC-JP three-byte characters,
and half-width Japanese katakana use ordinary decoding. Only double-width
rendered glyphs are painted in two halves. This is a legacy byte-stream
convention, not a Unicode escape-sequence extension.

For example, in Big5 the bytes `A4 A4` represent `中`. Send:

```text
ESC [ 31 m   A4   ESC [ 32 m   A4   ESC [ 0 m
```

The resulting character has a red left half and green right half. In GBK use
`D6 D0` for `中`, in EUC-KR use `C7 D1` for `한`, or in Shift-JIS use `93 FA`
for `日`. The spaces above separate bytes for readability and must not be sent.

Both semicolon and colon SGR colour forms use Mudlet's existing ANSI handling.
A character or its SGR sequences may cross network packet boundaries, including
Mudlet's partial-line flushes. Only SGR may occur inside a character; other
control sequences terminate the incomplete character and are processed normally.
The pending sequence is bounded by the existing CSI length limit.

The buffer stores a complete Unicode character, so text searches, Lua triggers
and plain-text copying still see the original character. Recolouring a selection
with the existing foreground/background APIs recolours both halves. The screen
and Copy as Image share the split renderer. HTML/plain-text logs and existing
single-colour query APIs do not encode a second rendition; colour queries and
HTML output use the left half.

Split formats are shared when characters are copied and detached when recoloured.
Each `TChar` remains 16 bytes; ordinary characters store their colors inline, while
only split characters allocate storage for both renditions. There is no global format
registry that retains styles after scrollback is discarded.
