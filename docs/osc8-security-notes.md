# OSC 8 hyperlinks: threat model and hardening notes

The [OSC 8 wiki page](https://wiki.mudlet.org/w/Manual:Supported_Protocols) documents what
servers can do with hyperlinks. This file records what Mudlet does to keep that safe, and
what it deliberately does not do. It exists because the wiki's "Security & Limitations"
section listed only the accepted URI schemes, a length cap, and reserved parameter names —
which is a list of rules, not a threat model, and rules without a threat model are hard to
extend correctly.

## Who the attacker is

In a terminal emulator, the untrusted party is a program the user chose to run. In a MUD
client it is the game server — and more often, another player, whose say/tell/channel text
the server relays to everyone without necessarily stripping escape sequences.

That inverts the usual assumption. Untrusted OSC 8 is not an edge case in Mudlet; it is the
normal case. Any OSC 8 feature has to be safe when the person who authored the sequence is
hostile to the person reading it and neither of them administers the server.

## What Mudlet guarantees

**Only six URI schemes are accepted.** `send:`, `prompt:`, `http:`, `https:`, `ftp:` and
`preset:`. Everything else is refused with a warning, in `TBuffer::decodeOSC()`. Web links
do still reach the platform handler via `QDesktopServices::openUrl()`, but only ever as
`http:`, `https:` or `ftp:` — the `file:` and arbitrary custom-scheme dispatches that other
terminals have had to defend against cannot be reached from an OSC 8 link at all. Note that
`ftp:` can be bound to a registered application on Windows and macOS.

**Link payloads cannot become code.** Mudlet transports a link's action as generated Lua
source that is executed when the link is clicked. Any remote text embedded in that source is
quoted through `LuaLiteral::quote()`, which picks a long-bracket level (`[[`, `[=[`, `[==[`,
…) that the payload can neither close, nor reopen under Lua 5.1's deprecated-nesting rule,
nor complete early by merging with the closing bracket appended after it.

This matters more than it might sound. Mudlet's Lua interpreter is created with
`luaL_openlibs()`, so `os.execute`, `io.popen` and `loadstring` are all reachable. Before
this was quoted, a `]]` in a server-supplied payload closed the string and the remainder ran
as code on a single left click. `LuaLiteralTest` locks the property down by evaluating
generated literals in a real Lua state and asserting the payload comes back as data: a table
of hand-picked adversarial payloads, plus an exhaustive sweep of every string up to five
characters over the `[`, `]`, `=`, `a` alphabet. The exhaustive sweep exists because the
hand-picked table originally missed payloads ending in `]`, which is a real shape — `say
[OOC]`, `get sword from bag[1]`, `http://[::1]`.

**Link metadata cannot lie about where a link goes.** Invisible and direction-reordering code
points — C0/C1 controls, bidi overrides and isolates, zero-width characters, line and
paragraph separators, word joiner, BOM — are rewritten into a visible `\u{202E}` form.
Sanitizing where the metadata is parsed rather than where it is drawn means the hover
tooltip, the right-click menu and the screen reader announcement are all covered at once.

There are two policies, because the two kinds of text have different needs:

- `UntrustedText::forTarget()` handles the command or URL shown in the default hint — the
  text a user reads to decide whether to trust a link. Nothing invisible survives it.
- `UntrustedText::forAuthoredText()` handles tooltips, menu labels and menu titles: prose the
  server author wrote to be read. It applies the same policy except for the zero-width
  joiner, the zero-width non-joiner and the tag character block.

Those three exceptions are not a relaxation for convenience. ZWJ is what assembles
multi-part emoji — 👨‍🍳, 🏳️‍🌈, 🏴‍☠️, family sequences — and the tag block is what assembles
subdivision flags like 🏴󠁧󠁢󠁳󠁣󠁴󠁿. ZWNJ is structurally required for correct Persian, Arabic and
Indic shaping; escaping it corrupts ordinary words. Emoji in menus are a documented feature
of the protocol and are used in the wild, so escaping them everywhere broke working servers.
They stay escaped in a link target, where they have no legitimate role and would serve only
to hide part of what the user is being asked to trust.

**Tooltips are not a markup channel.** `QToolTip` renders anything `Qt::mightBeRichText()`
accepts as HTML. Server tooltip text is escaped and wrapped in an explicit document so that
guess cannot decide whether server markup is live.

**A user can turn the whole feature off.** The per-profile "Enable OSC 8 hyperlinks from the
server" preference defaults to on. Unchecking it makes Mudlet ignore OSC 8 sequences *and*
advertise `0` for every `OSC_HYPERLINKS*` capability over NEW-ENVIRON, so a server that
honours the handshake falls back to MXP or plain text instead of emitting links into a void.

Toggling it mid-session tells the server immediately, rather than waiting for a reconnect.
RFC 1572 gives `INFO` the same syntax as `IS`, so all of the changed `OSC_HYPERLINKS*`
variables go out in a single `INFO` subnegotiation and the server sees one consistent change
instead of a run of partial ones. Only variables the server actually asked for are included,
so a server that requested just the umbrella `OSC_HYPERLINKS` receives only that one.

## What Mudlet does not guarantee

**Link body text is not sanitized.** Only metadata is. Bidi and zero-width handling in
ordinary game output is a broader question than OSC 8 and would need its own change.

**The escaped set is enumerated, not exhaustive.** `UntrustedText::unsafeCharacter()` lists
specific code points. Known invisible characters outside it include U+00AD soft hyphen,
U+3164 Hangul filler and U+2800 braille blank. Widening the set is a data change; assume it
is incomplete rather than assuming coverage.

**Authored text can still contain invisible joiners.** `forAuthoredText()` deliberately
permits ZWJ, ZWNJ and tag characters so emoji and Persian, Arabic and Indic text survive, so
a tooltip or menu label can carry invisible content. This is accepted: a hostile server can
already write a plainly untrue label in ASCII, and the text that states a link's actual
target uses the strict policy.

**Turning the preference off does not retract links already on screen.** The check happens
as sequences are decoded, so links drawn before the change stay clickable until the buffer
is cleared.

**A link that combines `menu` with `selection` does not get a `selected=` callback.** Its
commands come from the menu items rather than the base URI, so there is no single payload to
append the state to, and sending the base command instead would run something the user did
not pick. Selection state is still tracked and styled; only the server callback is skipped.

**A `send:` link fires a game command on one left click, with no confirmation.** That is the
feature working as designed, not an oversight. Server authors should not put irreversible
actions behind a bare `send:` link without their own confirmation step.

**The Lua interpreter is not sandboxed.** Quoting closes the path from OSC 8 to code
execution, but a Mudlet *package* the user installs still has unrestricted `os` and `io`
access. That is a separate, much larger piece of work with real compatibility cost.

**`openUrl()` accepts any scheme when called from Lua.** It is script-facing, and a script
that can call it can already call `os.execute`, so narrowing it would break packages for no
security gain. The OSC 8 path applies its own allowlist before ever reaching it.

## What server authors should do

- **Do not relay player-supplied text into OSC 8 sequences without stripping ESC (0x1B).**
  This is the single highest-value thing a server can do. If players can get escape
  sequences into channel or tell output, every protection above is defending users against
  each other rather than against you.
- **Write descriptive tooltips.** They double as the screen reader announcement. Without
  one, a keyboard user hears the raw command.
- **Use meaningful selection `group` and `value` names.** They are exposed to assistive
  technology.
- **Percent-encode `config` and `preset` if they appear as parameter names in your own web
  URLs**, as the wiki describes.

---

## Draft replacement for the wiki's "Security & Limitations" section

The wiki is edited by hand — this text needs a human to paste it in. It keeps the three
existing subsections intact and adds a threat model ahead of them and a sanitization note
after them.

```mediawiki
====Security & Limitations====

=====Threat Model=====

Unlike a terminal emulator, where hyperlinks come from a program you chose to run, an OSC 8
sequence in Mudlet comes from the game server — and often from another player, whose text
the server relays. Mudlet therefore treats every OSC 8 sequence as untrusted input.

Server authors carry one obligation that the client cannot discharge for them: '''strip the
ESC character (0x1B) from player-supplied text''' before it is echoed to other players.
Otherwise one player can author links that another player sees.

=====Trusted URI Schemes=====

Only these schemes are supported (others are rejected):
* <code>send:</code>, <code>prompt:</code>
* <code>http:</code>, <code>https:</code>, <code>ftp:</code>
* <code>preset:</code> (for definitions only)

Only allowlisted <code>http:</code>, <code>https:</code> and <code>ftp:</code> targets are
passed to the operating system's URL handler via <code>openUrl(...)</code>. Custom schemes
and <code>file:</code> targets are rejected and cannot be used to launch a local
application.

=====Limits=====

* '''URL length:''' Maximum 4096 characters
* '''Invalid JSON:''' Silently ignored (link still works without styling)
* '''Multi-line visibility:''' Only single-line links support visibility management

=====Display Sanitization=====

{{MudletVersion|4.22}}

Tooltips, menu labels, menu titles and the default hint that shows a link's target are
sanitized before display. Invisible and direction-reordering characters — bidi overrides and
isolates, zero-width characters, line and paragraph separators, the byte order mark, and
C0/C1 control characters — are shown in a visible <code>\u{202E}</code> form rather than
being rendered. This stops link text claiming one destination while the link carries
another.

Tooltip text is rendered as '''plain text'''. HTML in a <code>tooltip</code> value is
displayed literally rather than interpreted.

=====Turning OSC 8 Off=====

{{MudletVersion|4.22}}

Players can disable OSC 8 entirely with '''Settings → Special Options → Enable OSC 8
hyperlinks from the server''' (on by default). When it is off, Mudlet ignores OSC 8
sequences and advertises <code>0</code> for every <code>OSC_HYPERLINKS*</code> NEW-ENVIRON
variable, so a well-behaved server should fall back to MXP or plain text.

Changing the setting during a session sends an ad-hoc INFO (2) message covering every
<code>OSC_HYPERLINKS*</code> variable the server previously requested, in one
subnegotiation. Servers that track these capabilities should honour it without waiting for
a reconnect:

 IAC SB NEW-ENVIRON (39) INFO (2) USERVAR (3) ''OSC_HYPERLINKS'' VAL (1) ''0'' USERVAR (3) ''OSC_HYPERLINKS_SEND'' VAL (1) ''0'' [ .. ] IAC SE

===== Reserved Parameters =====

(unchanged)
```
