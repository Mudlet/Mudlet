# The Addon Command API

Lua functions letting packages put their own commands into Mudlet's chrome,
without any core changes of their own. A command is declared once and the
client places it: the same command can appear as a menu entry and a toolbar
button, and pressing either raises one event carrying one id.

That follows what Mudlet already does with its own commands - Triggers,
Mapper, Notepad and 16 others exist on both surfaces and are kept in step -
and it is what makes the API implementable by clients other than desktop
Mudlet. "Here is a command" maps onto whatever chrome a client has; "here is
a toolbar button" does not.

Commands are identified by the numeric id returned at creation - names and
labels carry no identity and may repeat freely. Every command belongs to the
profile that created it and is removed when that profile closes or resets.

## Placing a command

```lua
local id = addCommand{
  name     = "Speech settings",   -- required; the label on every surface
  icon     = "/path/icon.png",    -- filesystem path, Qt resource path, or theme name
  tooltip  = "Configure voices",
  menuPath = "Speech",            -- position within Extensions; "Speech/Voices" nests
  shortcut = "Ctrl+Alt+S",        -- menu only
  surfaces = {"menu", "toolbar"}, -- omit for every surface this client places commands on
}
```

`surfaces` is a list so that a client which grows another surface takes
another entry rather than a new spelling. A single name may be given as a
bare string. Omitting it means "wherever this client puts commands", which on
desktop Mudlet is the menu and the toolbar.

`addCommand` returns an id, or `nil, error` explaining why the command could
not be placed - an unparseable or already-taken shortcut, a `menuPath` part
that names an existing command, or `surfaces = "toolbar"` while the main
toolbar is hidden.

| Function | Returns | Behaviour |
| --- | --- | --- |
| `addCommand{...}` | id \| `nil, error` | As above. The first toolbar command also adds a separator dividing addon commands from Mudlet's own. |
| `removeCommand(id)` | boolean | Removes the command from every surface. Emptied `menuPath` submenus and the separator go with it; add/remove cycles leave no residue. |
| `enableCommand(id)` | boolean | Enables it on every surface. |
| `disableCommand(id)` | boolean | Disables it on every surface. |
| `setCommandChecked(id, checked)` | boolean | Sets a checkmark (the command becomes checkable on first use), on every surface. |
| `setCommandIcon(id, icon)` | boolean | Replaces the icon; path rules as above. |
| `setCommandTooltip(id, tooltip)` | boolean | Replaces the tooltip. Package text is escaped, so `<` and `&` show as typed. |
| `setCommandPulse(id, enabled[, color1, color2, interval])` | boolean \| `nil, error` | Toolbar-only refinement: a two-colour background pulse (defaults `#ff4444`/`#cc0000`, 500ms). Refuses a colour the client cannot parse, and an `interval` below 1ms. |

**Finding the menu:** the Extensions menu lives in Mudlet's menu bar - on
macOS that is the system menu bar at the top of the screen, not the Mudlet
window. It is created on first use and hides itself while empty.

**The main toolbar is hidden by default.** A command placed only on the
toolbar would be invisible on a fresh profile, so that combination is refused
with an error rather than returning an id for something nobody can see. A
command on both surfaces is always reachable, which is why that is the
default.

## Events

| Event | Argument | When |
| --- | --- | --- |
| `sysCommandClicked` | id (a number, as `addCommand` returned it) | The command was activated, from any surface. |

The event is raised on the profile that created the command, resolved at
click time - a command whose profile has since closed raises nothing.

```lua
local id = addCommand{name = "Speech", menuPath = "Speech"}
registerAnonymousEventHandler("sysCommandClicked", function(_, clicked)
  if clicked == id then toggleSpeech() end
end)
```

## Semantics implementations must preserve

1. **Ids are the only identity.** Labels, paths and names may repeat - two
   commands may share a label; operations address ids, and an unknown id
   returns `false` rather than erroring. The one exception is that within a
   single menu a label may not be both a command and a submenu, in either
   order: a later `menuPath` naming it could not say which was meant, and the
   player would see the label twice. Ids come from one sequence covering every command, so an id
   names one command or nothing. An id belonging to another profile answers
   as an unknown one does: a command only takes orders from the profile that
   created it.
2. **One command, one event.** However many surfaces a command occupies, it
   has one id and raises one event. A package never pairs up its own
   representations - the client knows which chrome exists, so the client does
   the placing.
3. **Removal from a command's own handler is safe.** The usual caller of
   `removeCommand` is a Lua handler running from that very command's event;
   implementations must tolerate this (desktop Mudlet defers destruction past
   the event loop turn for exactly this reason).
4. **Profile close and profile reset clean up.** Every command created by a
   profile is removed when that profile closes *or* resets, without the
   package's involvement - a reset takes the Lua state with it, so the ids
   the package was holding die there and the commands must not outlive them.
5. **Cycles leave no residue.** Repeated add/remove must not accumulate
   toolbar spacing, separators, or menu entries; emptied `menuPath` submenus
   disappear.
6. **A command that cannot be placed says so.** Returning an id for something
   the user cannot see or reach is worse than refusing, because the package
   has no way to find out.
