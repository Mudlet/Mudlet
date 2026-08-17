# The Addon Toolbar and Menu API

Lua functions letting packages add their own controls to Mudlet's main
toolbar and menu bar, without any core changes of their own. Controls are
identified by the numeric id returned at creation — names and labels carry no
identity and may repeat freely. Every control belongs to the profile that
created it and is removed automatically when that profile closes.

Written to be implementable by clients other than desktop Mudlet: a client
with different chrome maps "toolbar" and "menu" onto whatever equivalent
surfaces it has, preserving the id/event contract below.

## Toolbar buttons

| Function | Returns | Behaviour |
| --- | --- | --- |
| `addToolbarButton(name, icon, tooltip)` | id \| `nil, error` | Adds a button to the main toolbar. `icon` accepts a filesystem path or a Qt resource path, and may be empty for a text-only button — packages ship their own artwork. The first addon button also adds a separator dividing addon controls from Mudlet's own. |
| `removeToolbarButton(id)` | boolean | Removes the button. Removing the last addon button removes the separator too; add/remove cycles leave no residue. |
| `setToolbarButtonState(id, state)` | boolean | Sets a state string on the button (styling hook; shown in the tooltip area by implementations that surface it). |
| `setToolbarButtonIcon(id, icon)` | boolean | Replaces the icon; path rules as above. |
| `setToolbarButtonTooltip(id, tooltip)` | boolean | Replaces the tooltip. |
| `setToolbarButtonEnabled(id, enabled)` | boolean | Enables or disables the button. |
| `setToolbarButtonPulse(id, enabled[, color1, color2, interval])` | boolean \| `nil, error` | Starts or stops a two-colour background pulse (defaults `#ff4444`/`#cc0000`, 500ms). `interval` below 1ms is rejected — a zero interval would restyle on every event-loop pass. |

## Menu items

| Function | Returns | Behaviour |
| --- | --- | --- |
| `addMenuItem(menuPath, name[, shortcut])` | id \| `nil, error` | Adds an item under an **Extensions** menu (created on first use inside Mudlet's Options menu). `menuPath` builds nested submenus: `"Speech/Settings"` yields Extensions → Speech → Settings. `shortcut` is a key sequence string. |
| `removeMenuItem(id)` | boolean | Removes the item. Submenus created for a `menuPath` prune themselves once empty; the Extensions root hides itself while empty and reappears on the next add. |
| `setMenuItemEnabled(id, enabled)` | boolean | Enables or disables the item. |
| `setMenuItemChecked(id, checked)` | boolean | Sets a checkmark (the item becomes checkable on first use). |

**Finding the menu:** the Extensions menu lives in Mudlet's menu bar — on
macOS that is the system menu bar at the top of the screen, not the Mudlet
window.

## Events

| Event | Argument | When |
| --- | --- | --- |
| `sysToolbarButtonClicked` | id (as string) | The button was clicked. |
| `sysMenuItemClicked` | id (as string) | The item was activated. |

Both events are raised on the profile that created the control, resolved at
click time — a control whose profile has since closed raises nothing.

## Semantics implementations must preserve

1. **Ids are the only identity.** Labels, paths and names may repeat;
   operations address ids, and an unknown id returns `false` rather than
   erroring.
2. **Removal from a control's own handler is safe.** The usual caller of
   `removeToolbarButton`/`removeMenuItem` is a Lua handler running from that
   very control's click event; implementations must tolerate this (desktop
   Mudlet defers destruction past the event loop turn for exactly this
   reason).
3. **Profile close cleans up.** Every control created by a profile is removed
   when that profile closes, without the package's involvement.
4. **Cycles leave no residue.** Repeated add/remove of buttons must not
   accumulate toolbar spacing, separators, or menu entries; emptied
   `menuPath` submenus disappear.
