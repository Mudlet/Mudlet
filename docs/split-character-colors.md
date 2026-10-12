# Split colours in legacy CJK text

Some games that use a legacy double-byte encoding send an ANSI SGR sequence
(`ESC [ … m`) between the two bytes of a character, so that its left and right
halves are drawn in different colours. For example, in Big5 the bytes `A4 A4`
are `中`, and

```text
ESC [ 31 m   A4   ESC [ 32 m   A4   ESC [ 0 m
```

(sent without the spaces) is one `中` with a red left half and a green right
half. Mudlet stores the complete character, so triggers, searches and copied
text see `中`, and paints each half in its own rendition.

Supported encodings: Big5, Big5-HKSCS, GBK, the two-byte characters of GB18030,
EUC-KR, Shift JIS and the two-byte characters of EUC-JP. Only full-width
characters are split; GB18030's four-byte and EUC-JP's three-byte characters and
half-width katakana are decoded as usual. UTF-8 is unaffected.

## Decoding

`TBuffer::translateToPlainTextInner()` holds a lead byte that is immediately
followed by `ESC` in `mPendingLead`, together with the rendition it arrived in.
The SGR sequences that follow are parsed by the ordinary CSI code, so they may be
split across packets or partial-line flushes like any other sequence. The next
valid trail byte completes the character: the held rendition paints the left
half, and the current one the right half and whatever follows.

Only SGR may come between the bytes. Any other escape or control sequence, a line
end, or a byte that cannot be a trail byte turns the held lead byte into U+FFFD
and is then processed normally. The held byte belongs to its data channel, so
locally fed text (`feedTriggers()`, echoes) cannot complete or discard a
character that the game has half sent.

## Storage

`TChar` stays 16 bytes. Link ids fit in 16 bits (`TLinkStore` caps them at
20000), and the other 16 bits hold the index of the right half's rendition -
foreground, background and display attributes - in a process-wide table that
interns each distinct rendition once. A character that is not split has index 0
and costs nothing extra, and a character copied to another buffer keeps its index
without remapping. The table only grows by the number of distinct right-half
renditions a game uses; once full (65535 of them) further characters are drawn
in one colour.

Every `TChar` setter restyles the whole character, so recolouring from Lua,
search highlights, selection, link styling and `paste()` keep both halves
consistent. A change that leaves both halves alike returns the character to an
ordinary one.

## What sees only one colour

HTML logs and copy as HTML, and the colour query APIs (`getFgColor()`,
`getBgColor()`, colour triggers), use the left half.
