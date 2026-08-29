# The Addon Command API

Lua functions letting packages put their own commands into Mudlet's chrome,
without any core changes of their own. A command is declared once and the
client places it: the same command can appear as a menu entry and a toolbar
button, and pressing either raises one event carrying one id.

That follows what Mudlet already does with its own commands - Triggers,
Mapper, Notepad and most of the rest exist on both surfaces and are kept in
step - and it is what makes the API implementable by clients other than
desktop Mudlet. "Here is a command" maps onto whatever chrome a client has;
"here is a toolbar button" does not.

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
  shortcut = "Ctrl+Alt+P",        -- needs the menu item, so not with surfaces = "toolbar"
  surfaces = {"menu", "toolbar"}, -- omit for every surface this client places commands on
}
```

`surfaces` is a list so that a client which grows another surface takes
another entry rather than a new spelling. A single name may be given as a
bare string. Omitting it means "wherever this client puts commands", which on
desktop Mudlet is the menu and the toolbar.

`addCommand` returns an id, or `nil, error` explaining why the command could
not be placed. The reasons are:

- `name` is missing, so the command has nothing to show
- a field was given a value Mudlet cannot read as the type it expects -
  `menuPath = {"Speech"}`, say - rather than being left out. A number is read
  as a string, so `menuPath = 12345` places a submenu of that name
- every surface it asked for is hidden (see *The main toolbar is hidden by
  default* below)
- a `shortcut` or a `menuPath` was given alongside `surfaces = "toolbar"`,
  where there is no menu item for either to attach to
- a `shortcut` was given while the menu bar is hidden, so it could never fire
- the `shortcut` is not a key sequence Qt understands, is longer than the four
  steps Qt can hold, or is one already taken - by Mudlet, or by a command of
  this or another profile
- a `menuPath` part names an existing command of this profile's, or the
  command's own name is already a submenu of this profile's in the menu it
  would land in
- `surfaces` names something other than `"menu"` or `"toolbar"`, or is an
  empty list - which asks for the command to go nowhere

| Function | Returns | Behaviour |
| --- | --- | --- |
| `addCommand{...}` | id \| `nil, error` | As above. The first toolbar command also adds a separator dividing addon commands from Mudlet's own. |
| `removeCommand(id)` | boolean | Removes the command from every surface. Emptied `menuPath` submenus go with it, and the separator once the last toolbar command does; add/remove cycles leave no residue. |
| `enableCommand(id)` | boolean | Enables it on every surface. |
| `disableCommand(id)` | boolean | Disables it on every surface. |
| `setCommandChecked(id, checked)` | boolean | Sets a checkmark (the command becomes checkable on first use), on every surface. A checkable command activated by the user stays in step across surfaces: the state the pressed surface reached is the state the others take, before `sysCommandClicked` is raised. |
| `setCommandIcon(id, icon)` | boolean | Replaces the icon; path rules as above. |
| `setCommandTooltip(id, tooltip)` | boolean | Replaces the tooltip; an empty string takes it away, leaving the menu item to fall back on its own label as every other menu item does. Package text is escaped, so `<` and `&` show as typed - in labels too, where a bare `&` would otherwise be read as a keyboard mnemonic and vanish from the text. |
| `setCommandPulse(id, enabled[, color1, color2, interval])` | boolean \| `nil, error` | Toolbar-only refinement: a two-colour background pulse (defaults `#ff4444`/`#cc0000`, 500ms). Refuses a colour the client cannot parse, and an `interval` below 1ms. |

**Finding the menu:** the Extensions menu lives inside Mudlet's Options
menu - on macOS that is in the system menu bar at the top of the screen, not
the Mudlet window. It is created on first use and hides itself while empty.

**The main toolbar is hidden by default.** A command is refused when every
surface it asked for is hidden - the toolbar alone while the toolbar is off,
the menu alone while the menu bar is off, or either default while both are.
A command on both surfaces is reachable as long as one bar is showing, which
is why that is the default. A `shortcut` is the exception to "one bar is
enough": it hangs on the menu item, so it needs the menu bar itself.

**Menu placement is per profile.** `menuPath` submenus and the label rule
above are decided by the calling profile's own commands. Two profiles may use
the same path or the same label, and neither can see or clear the other's -
which is why a refusal never names one.

Shortcuts are the exception: a key sequence belongs to the window, so two
profiles cannot hold the same one at once. The second is refused, without
being told whose command has it.

**Detached profiles keep their own chrome.** A profile detached into its own
window builds its own menu bar and toolbar, and commands are not mirrored into
them; they stay reachable from the main window.

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
   single menu one profile may not use a label as both a command and a submenu,
   in either order: a later `menuPath` naming it could not say which was meant,
   and the player would see the label twice. The rule is scoped to the profile,
   for the reason under *Menu placement is per profile* above. Ids come from one sequence covering every command, so an id
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
7. **Package text is data, not markup.** Labels, tooltips and colours come
   from a package and are escaped or validated before they reach the client's
   chrome - a `<` must not eat the rest of a tooltip, an `&` must not become a
   mnemonic, and a colour must not be able to carry styling of its own.
8. **A checkable command holds one state.** Where a client shows a command on
   more than one surface and the user can toggle it, activating one surface
   carries the others with it. Two representations of one command must never
   disagree about whether it is checked.
