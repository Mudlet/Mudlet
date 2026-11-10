# Mudlet design language

The shared UI language for Mudlet's redesigned dialogs. Seeded by the settings
redesign (`src/dlgProfilePreferences.cpp`), consumed next by the editor
redesign. Read this before building or migrating a dialog; the settings shell is
the worked example to copy from.

The helpers live in `src/uiDesign.h` (namespace `uiDesign`), implemented in
`src/uiDesign.cpp`; the sidebar's item delegate is in
`src/SidebarItemDelegate.h`. Consume them - do not copy them out into a new
file.

This document is the reasoning. The working reference - the rules in one list,
the recipe index, which windows have adopted the design and which have not, and
the checklist for bringing one in - is the `design-language` skill in
`.agents/skills/design-language/SKILL.md`, which every coding assistant reads
before touching a window. Keep the two in step: a new recipe or a newly adopted
window goes into both.

## 1. Principles

### Colour comes from the palette, at runtime

Every colour is derived from `QApplication::palette()` when the shell is styled,
never written as a hex literal. See `applyShellStyle()`
(`src/dlgProfilePreferences.cpp`) and the two helpers it derives colours with
(`src/uiDesign.cpp`):

- `blend(from, to, amount)` - mixes two palette colours to get borders, muted
  text, scrollbar handles.
- `rgba(color, alpha)` - a stylesheet colour string with an alpha component.

Four palette roles carry the design: `QPalette::Window` (page),
`QPalette::Base` (field), `QPalette::WindowText` (text), `QPalette::Highlight`
(accent). Two of the four are taken as they come and two are softened before
anything is mixed from them - see below. Light and dark treatments are chosen
from the measured lightness of the page colour (`darkPage`), not from
`MudletApp::darkMode()`, so a dark system theme under "follow the system" is
handled too.

This is correctness, not taste: a profile's Lua stylesheet can retheme the whole
application, and a hardcoded colour becomes unreadable the moment it does. Read
`qApp`'s palette rather than the dialog's own - assigning a stylesheet to a
widget freezes that widget's palette.

### One base style for both appearances

Changing the appearance changes colours, never sizes or positions. On macOS
and Windows both appearances are drawn on Fusion: `DarkTheme` over it for the
dark one, a plain `MudletProxyStyle` over it for the light one
(`mudlet::setAppearance()`). The platform styles there measure controls their
own way - spin box heights, the gaps between form rows, the margins round an
item view's rows - and with the light appearance on the platform style and the
dark one on Fusion, 794 widget positions across the settings dialog's twelve
pages differed between the two (macOS, Qt 6.11.3). A window the design has not
reached yet takes Fusion's shapes in the light appearance, as it already did in
the dark one. Linux keeps the desktop's own style for the light appearance,
since a player there may have chosen it, and macOS and Windows keep a style
the player chose with `-style` or `QT_STYLE_OVERRIDE` for it too.
`AppearanceBaseStyleTest` guards this.

### Three tones of depth

`themeTokens()` mixes three surfaces, and which one a rule reaches for is the
whole of what says how deep the thing it draws sits. Pick by what the widget
*is*, never by what colour looks right:

| Token | Recipe | Use it for |
| --- | --- | --- |
| `page` | `QPalette::Window` | The window itself and every piece of it: toolbar, status bar, sidebar pane, the column an item is edited in, scroll areas |
| `card` | `page` lifted towards white (6% on dark, 55% on light) | A panel raised off the page: the options cards, menu surfaces |
| `field` | `QPalette::Base`, lifted 85% towards `page` on dark, so a check box on a card is no black well | Anything the user types into or picks a value in: line edits, combo boxes, spin boxes, the search field, a check indicator's fill |

The order is fixed - `field` is sunk into `card`, `card` is lifted off `page` -
and it is what makes the windows read as having depth rather than as flat
collapsing to black. Deriving the page from `QPalette::Base` gets this exactly
backwards: Base is the *input field* colour, near-black on a dark theme, so a
page mixed off it ends up darker than the fields lying on it.

A dark theme's `Base` is near-black - #191919 under a #353535 page - which
reads as a hole cut through the window rather than as a well sunk into it, so
the dark field is lifted back towards the page and lands at #212121. A light
theme's `Base` is white and is left where it is: lifting white towards a grey
page only muddies it.

### The words are softened towards the page

`text` is not `QPalette::WindowText` as it comes. A palette answers pure white
on a dark theme and pure black on a light one, and neither is a colour a page of
text is set in - the white glares against a #353535 page and the black is
heavier than anything else in the window. So the words are pulled a little way
back towards whatever they are written on: 16% on dark, which turns #ffffff into
#dfdfdf, and 10% on light, which turns #000000 into #181818.

| Theme | palette `WindowText` | `text` | `mutedText` | `page` | `card` | `field` |
| --- | --- | --- | --- | --- | --- | --- |
| dark (#353535 window, #191919 base) | #ffffff | #dfdfdf | #afafaf | #353535 | #414141 | #212121 |
| light (#efefef window, white base) | #000000 | #181818 | #585858 | #efefef | #f8f8f8 | #ffffff |

Every other ink is mixed from `text` over a surface, so the whole scale softens
with it - `mutedText`, `disabledText`, `border`, and the `accentText` blend.
That is the point rather than a side effect, and the contrast is measured rather
than assumed (`EditorSurfaceToneTest`, `ReadabilityAuditTest`): body text clears
4.5:1 on all three surfaces in both themes - 9.2:1, 8.9:1 and 7.7:1 on dark,
15.4:1, 15.7:1 and 16.7:1 on light. `mutedText` is a quieter weight of the same
words rather than a different class of thing - a card's description, a chip, the
status bar - so it carries the same floor and is walked until it meets it on all
three: 5.6:1, 5.4:1 and 4.7:1 on dark, 6.2:1, 6.3:1 and 6.7:1 on light.
`disabledText` is walked the same way to the 3:1 an inactive word is held to,
over the three surfaces and the field as well.

A toolbar button is the exception. `disabledText` has to read on a card, which
on dark lifts it to #8d8d8d beside the bar's #afafaf, 1.5:1 apart: an
unavailable button looked like an available one. `disabledBarText` is walked
against the page alone, where every bar is drawn, and only to
`scmDisabledBarMinimumRatio` (2:1, about where macOS draws an inactive control):
#656565 on dark and #aaaaaa on light, 2.7:1 and 3.1:1 from the buttons beside
it. `toolBarStyleSheet()` writes the word in it and `restyleToolBarGlyphs()`
the glyph, on the editor's, the main window's, a detached window's and the
notepad's bar. WCAG asks nothing of an inactive control, and
`ReadabilityAuditTest` already passes disabled buttons by.

#### One ink for the editor's chrome

In the script editor, `mutedText` is not one tone among several - it is *the*
tone. Every word the window says outside a field is written in it: the toolbar's
buttons, the sidebar's names, every row of all seven item trees and their
headings, the search results' titles, a card's title and everything on the card,
check boxes, radio buttons, group boxes, the pattern rows' numbers and prompt
labels, the "Lua script" heading, the word a trigger's options strip is led by
and the two segments its matching mode is chosen with, and the status bar. The
full `text` tone is what is *inside* a field - a line edit, a spin box, a combo
box's displayed value, the code pane, the error console - and nothing else has
it. Two greys in one window's chrome read as two windows.

Three things keep an ink of their own, and each says a state rather than a tone:
`accentText` for what is chosen or under the pointer, `disabledText` for what is
unavailable, and the state chips - the compile chip, the OR/AND mode chip, and
the note that refuses a duplicate event name - whose colour is walked against
their own fill by `readableOn()`. Everything that says something is *broken*
comes from one function rather than from the same expression in nine places:
`uiDesign::errorInk(tokens)` walks the error hue off the strip's surface, and
that one red is what the compile note and its mark over the code pane, the mark on a
broken item's tree row, the picture on the editor's error notice, the note
refusing a duplicate event name, a clashing shortcut and an untrusted
certificate in the settings, and the edge round a field the connection dialog
will not take are all drawn in.

The line the compiler stopped on is marked among the code pane's line numbers
as well, by `uiDesign::EditorLineMarginDelegate` on edbee's margin: the trees'
mark for a broken item just before the number, and the number itself in the
error hue - the same mark the compile note carries, so the row, the note and the
line read as one, where a red dot in that place would read as a breakpoint. That margin
is painted on the edbee theme's background rather than on the strip, so its red
is the error hue walked by `readableOn()` against the theme's own background
instead of `errorInk()`. The room for the mark is kept on every line, so the code
does not move sideways when a save fails.

The strip is the darkest surface any of them sits on, which under a light theme
makes it the hardest to read on and under a dark one the easiest: measured on
this Mac the dark red reads 4.60:1 on the strip but 3.54:1 on the page, 2.94:1
on a card and 2.38:1 on the certificate warning's wash. So the three places
written on a lighter surface than the strip - the events row's note, the
shortcut warning and the certificate labels - walk that value on with
`readableOn()` against the surface they are really on. Each of those walks is a
no-op wherever the red already clears the floor, which is everywhere under the
light theme, so the one red holds unless holding it would put words under
`scmTextMinimumRatio`. And what the user typed is content wherever it
is shown, not chrome: a script's own event names on their chips keep the full
`text` tone the way the words in a field do, while Mudlet's own `sys*` events on
the same row are quiet.

The **dot** at the row's leading edge says whether the item is on, but nine
pixels at the far left is not enough to find the one switched-off script in a
long list by. So a row that is not running has its name in `disabledText`, and
one whose own switch is off is struck through as well - the line is a shape, so
it holds in greyscale and for a colour-blind reader, and it is what browser
devtools draw through a CSS rule that is switched off. A row that is on inside a
group that is off is dimmed but not struck: its own switch is fine, and the
strike marks the row a click would fix. A closed group says at its trailing edge
how many rows anywhere inside it are struck ("2 off",
`EditorTreeDelegate::switchedOffInside()`), the name eliding short of it; an open
group says nothing, its rows show it. A change to a row inside a closed group
repaints the tree, or the count on screen would stay as it was. The Folders view
leaves the count out, since its trailing edge is the item's type. Hovering a
dot says which of the three states it is in and what a click will do, naming the
group that is off for a row on inside it (`EditorTreeDelegate::dotToolTip()`,
shown from `helpEvent()` over the dot's hit rectangle only).
`EditorTreeErrorMarkTest` holds all of it.

A `color:` rule cannot reach a view's rows - a stylesheet only gets at them
through `::item`, and a `::item` rule reaches the widget's palette for some
selectors and not others - nor a control that paints itself, such as the
sidebar's collapse chevron. Those are told outright, through
`inkAsChrome()` in `src/dlgTriggerEditor.cpp`, which writes the tone into
`QPalette::Text`, `WindowText` and `ButtonText` and the accent into
`HighlightedText`. It runs *after* anything that re-polishes the widget: a
re-polish puts back the palette the rules were applied to.

The settings dialog is not held to this. Its sidebar is the whole of that
dialog's navigation and keeps the full tone (the `itemColor` argument to
`sidebarStyleSheet()`), and `cardStyleSheet()` - shared by both windows - writes
no title colour at all, so a settings card's title stays whatever the palette
answers. The editor colours its own cards' titles from its own sheet;
nothing about the shared builder changed.

`test/functional_tests/EditorChromeInkTest.cpp` is the guard. It walks every
visible, enabled thing in the editor that shows words - labels, buttons with
text, group box titles, and each view once by its palette - in every one of the
seven views, with an item on show in each - skipping fields and everything under
them, the state chips and a chip's own name by object name (`editorCompileChip`,
`editorChipNote`, `editorChipLabel`), and anything chosen,
pressed or under the pointer, and compares each ink against `mutedText` on the
dark appearance and then the light. It reports how many things it read and fails
below a pinned floor, so a walk that stopped finding widgets cannot pass. One
case reads pixels rather than palettes: the ink of the Triggers heading row, off
a grab of the window, has to be that same tone to within two levels per channel.

Anything mixed to sit *on* a surface takes that surface as its `from`:
`border`, `mutedText` and `disabledText` are blends over `page`; a card's check
indicator outline is a blend over `card`; placeholder text is a blend over
`field`. The three tones can collapse where a palette leaves no room - macOS
answers white to `Window` and `Base` alike on its light appearance - so
`themeTokens()` steps the page down instead of the card up when the lift would
be under six levels of lightness. Neither window does that arithmetic itself.

Two more tones say where one column of a window ends and the next begins. They
are not a fourth and fifth level of depth - `pane` is the *smallest* step the
model has, and `separator` is a line rather than a surface:

| Token | Recipe | Use it for |
| --- | --- | --- |
| `pane` | a fifth of the way from `page` to `card` | A column that is a surface of its own rather than a piece of the page, taken as one thing from the row heading it to the trees under it: the editor's panel of items (`#editorItemPane`, its trees and their scroll bars). Two levels of lightness on a dark theme - enough to be told apart from the page beside it, and far short of reading as a panel laid on top of it |
| `separator` | `page` taken towards black (36% on dark, 10% on light) | The seam between two panes, and along the sidebar's right edge. A `GripSplitterHandle` carrying nothing draws it as one pixel down the middle of itself, with each neighbour's own tone carried up to it, so the nine pixels the mouse needs are not nine the reader sees; hovered, that line widens to three of the accent. A handle carrying a heading - the code pane's - is the same tone filling a strip deep enough to read it in, with the top corners cut to `scmRadiusPanel` since it is the top of the pane under it; hovered, that one lights the same three pixels of the accent along the edge the drag moves, clipped to the cut corners, since a strip with words on it has no room to say it twice. A groove cut into the window, as against `border`, which is a hairline drawn on it |

The separator's drop is the smaller one on a light theme on purpose: a light page
is near enough to white that a dark-theme drop would draw a grey rule across the
window rather than a seam between two panes. Measure a pane against the page it
lies beside, never against a card: a pane a card's distance off the page stops
reading as part of the window.

### One treatment for every input

`inputStyleSheet(tokens, selectorPrefix)` draws everything a value is typed into
or picked in - line edits, combo boxes, spin boxes - as one control: the `field`
surface, a 1px border, `scmRadiusInput`, an accent frame on focus. Framing a
`QComboBox` or a `QAbstractSpinBox` from a stylesheet removes the arrows the
platform draws inside it, so the sheet claims those two only once it has tinted
arrow PNGs cached to point at, and otherwise leaves them the platform's frame.

The list a combo box drops down is the field opened up: the same surface, the
same hairline and the same `scmRadiusInput` corner as the box it came out of,
with the accent under the chosen row. It was the card once, and on a card -
which is where every settings combo box sits - that opened a list in the card's
own grey, lifted off nothing. The frame the platform draws round that list is a
`QFrame` of its own, named away by the same rule, since its bevel is the one
part of a field nothing else reaches. On macOS the light appearance drops a
menu-style list instead, which the platform draws; the rule is there for the
list, wherever a style shows one.

That frame is also a window, and a window is filled before what is in it is
drawn - so a radius on the list alone leaves the window's own square corners
showing through in the fill, and the arc has to be cut out of the window as
well. `letPopupsTakeTheFieldsCorner(container)` is what cuts it: the window is
asked to be see-through and the frame's brushes are named as nothing, which the
sheet cannot say for itself, since a stylesheet gives a `QFrame` subclass no
styled background and its rule for the container reaches only that widget's
palette. Both have to be said before the popup's first show - a platform makes
the window's surface then and never again - so it runs from the shell's style
pass and again wherever a window builds a combo box after that pass, the way
`keepClickFocusOffControls()` does. The list's own 2px padding is what keeps its
square viewport inside the arc, so nothing else needs clipping.

A menu is a popup too, and takes the same corner from the same call - said on
the menu itself, which *is* its own window rather than a list living inside
one. Every `QMenu` under the container is opened up, and the container when it
is one, so a context menu built at the moment it is wanted can be handed
straight to this. Only call it where the menu sheet reaches: a popup opened up
with nothing painting its surface is see-through all over rather than at the
corner.

Those chevrons are the one part of a field that is pressed rather than typed
into, so they say so: a stepper's or a drop-down's chevron takes the accent
under the pointer and holds it while the button is down, and the stepper's own
square lights the `hoverSoft` wash and then `accentSoft`. A segmented control
follows the same reading - the one hairline its two halves share belongs to
whichever of them is chosen, so the accent runs all the way round it, and the
other half takes that pixel back as padding so nothing moves.

Scope it, never set it on a window: the editor sets it on each of its seven
forms, the settings dialog passes `#settingsStack` so the rules stop at the
pages. Unscoped it would take the search field, the sidebar's editors and every
tree's inline editor with it.

### One mark for every choice

`choiceStyleSheet(tokens, selectorPrefix)` draws the three controls a choice is
made on as one mark: a check box's box, a radio button's circle and the box a
checkable card's title begins with. `scmChoiceIndicatorSize` across, the `field`
surface inside, a 1px outline mixed from `card` towards `text`, and the corner
that says which it is - `scmChoiceBoxRadius` for a box, half its own size for a
circle. The hairline takes the accent under the pointer, on focus and once the
choice is made; `accentSoft` washes the box while it is held down; an
unavailable one drops to the same quiet border and let-down fill a disabled
field takes. It also names the gap between the mark and the words, which a style
would otherwise measure off the indicator it was going to draw itself - and so
left touching on one platform and well apart on the next - and it names the
control's own frame, as none. That last is not decoration: a rule that leaves
the frame to the platform leaves the *layout rectangle* to it too, and the
macOS style trims eleven pixels off a check box's for the bezel it would have
drawn, so a `FlowLayout` placed the next control's words over the last letters
of this one. `EditorTriggerOptionsStripTest` measures the strip's check box
against its own hint for that reason.

Three states, three pictures, all tinted through `readableOn(field, accent,
text, scmQuietMinimumRatio)`: the Lucide `check` when it is on, a filled dot for
a chosen radio button, and the Lucide `minus` for `Qt::PartiallyChecked`. That
dash is the point of the sheet as much as the rest: a platform paints a
part-checked box as a filled grey square, which reads as a third kind of
control rather than as a box holding both answers at once. Each picture is
written into the glyph cache the way the field arrows are, and a state whose
file could not be written leaves its rule out rather than pointing at nothing.

`cardIndicatorStyleSheet(cardProperty, tokens)` draws the card's own indicator
from that same body, so turning a whole card on reads as the same act as turning
one option on. It keeps a function of its own only because
`measuredCardTitleHeight()` has to lay a box out under those rules alone, before
the rest of the window's sheet exists.

Scope it the way the inputs are scoped: the editor sets it on each form, the
settings dialog passes `#settingsStack`. Two things follow from the prefix. A
scoped rule carries an ID, so it outranks any rule of the window's own that
names only a type or a property - `settingsChevronRow` and the search-match
highlight both carry `#settingsStack` for that reason. And unscoped, a rule
naming a type ties with one naming a property once both add a state, which is
settled by which was written last: the editor's segmented control gives its
indicator no size at all, so the shared rules go first in that form's sheet and
the segment's own follow.

The accent on `:focus` is the keyboard's. A click does not focus a mark in
either window, so turning an option on and off again leaves nothing behind;
tabbing onto it does, and that is what the ring is for.
`keepClickFocusOffControls(container)` says so outright - on the container the
sheet was set on, after setting it - because otherwise the answer is the base
style's rather than the design's. `QAbstractButton` reads
`SH_Button_FocusPolicy` once, in its constructor, and keeps what that style
answered, and the base style under the app is replaced on every appearance
change: `DarkTheme` over Fusion for the dark one, a plain `MudletProxyStyle`
over Fusion for the light one (over the desktop's own style on Linux, or a
style the player chose). Both answer `Qt::StrongFocus` on macOS with Qt 6.11, which is what
left the accent sitting on a mark after a click; a platform whose style answered
`Qt::TabFocus` would have made the same code look correct. The helper reaches
check boxes, radio buttons, push buttons and checkable cards, and leaves a
control that asks for `Qt::NoFocus` where it is. `makeChevronRow()` calls it on
the row it is given, since a search result's row is built long after the sheet
naming its accent was set.

### Buttons

`buttonStyleSheet(tokens, selectorPrefix)` draws the ordinary push button: the
field's corner and content height on a face lifted off the card rather than sunk
into it, since a button is pressed where a field is typed into. Accent hairline
on hover and focus, `hoverSoft` then `accentSoft` behind it, and an unavailable
one written in `disabledText` on nothing at all. A button carrying a menu says so
with the same chevron a combo box drops its list under, cached by
`themedArrowFile()` and only claimed once that file exists; no selector can ask
whether a button has a menu, so the room the words are held clear by is what
that rule's own width reserves through `PM_MenuButtonIndicator`. Its accent on
focus is the keyboard's too, and `keepClickFocusOffControls()` is what keeps a
click from leaving it behind - see the paragraph closing the section above.

A button that has to stay a `QToolButton` - because the body of it acts and the
trailing half opens a menu, which is `QToolButton::MenuButtonPopup` and nothing
else - joins the same rule by carrying `uiDesign::scmProp_menuButton`
(`uiMenuButton`). Face, hairline, corner, padding and content height are the
push button's. The property is opt-in because a tool button without it is
whatever its window makes of it; the editor's forms carry several that paint
themselves. The connection dialog's Copy button is the one that has it today.
Assigning a sheet to the button takes the room for the words with it, since the
label is laid out in the whole of the contents rectangle: the rule's own
`padding-right` of `scmButtonPaddingHorizontal + scmInputDropDownWidth` is what
holds the word clear of the half.

**Two click areas have to read as two.** The trailing half is a segment of its
own, not a chevron floating on one face, and
`splitButtonMenuHalfStyleSheet(buttonSelector, cornerRadius, tokens)` is the one
recipe that draws it - called by `buttonStyleSheet()` for a form's split button
and by the editor's shell pass for the toolbar's Save Profile button. It writes
three things on `::menu-button`, which is `scmInputDropDownWidth` (18px) wide at
`subcontrol-position: center right`:

- **The seam.** A `border-left` of the design's hairline on the half's leading
  edge, and nothing else at rest - no fill, no other border. Assigning a sheet
  to the button takes the platform's own separator between the halves away, and
  this puts one back in the design's tone: before the pointer arrives, the seam
  is the whole of what says there are two areas here.
- **Its own wash.** `::menu-button:hover` and `:pressed`, with the half's outer
  corners rounded to the button's own radius so a lit half does not poke square
  corners out of a rounded button. The wash is a step ahead of whatever the body
  takes, because the body's hover fills the whole button, the half included:
  where the body lights to `hoverSoft` the half lights to `accentSoft` - the
  same wash a row of the menu it opens takes under the pointer - and where the
  body is pressed to `accentSoft` the half takes twice that. A half washed in
  what the body is already washed in never reads as its own target.

  **It says the half is a target, not that it is the one under the pointer**,
  and cannot say the second: `QToolButton` tracks its hovered sub-control
  through `QStyle::hitTestComplexControl`, which `QStyleSheetStyle` does not
  answer for `CC_ToolButton`, so the button reports `SC_ToolButton` for a point
  anywhere on it and `::menu-button:hover` matches whenever the button as a
  whole is pointed at. Measured, not assumed - the half reads the same with the
  pointer on the words as with it on the chevron. The press is unaffected:
  `QToolButton::mousePressEvent` asks `subControlRect()`, which the sheet does
  answer, so clicking the half opens the menu and clicking the words does not.
- **The design's chevron** through `::menu-arrow`, `themedArrowFile()`'s Lucide
  one at `mutedText` and the accent one under the pointer, in place of the
  filled triangle a platform style leaves there. Guarded like every rule
  pointing at a picture: no cached file, no rules at all.

Only two things differ between the two callers, and both are arguments: the
selector naming the button, and the corner its own rule rounds to
(`scmRadiusInput` on a form, `scmToolBarButtonRadius` on the bar).

Three kinds of button are left out on purpose:

- **Colour wells** (`generateButtonStyleSheet()`, `mTEXT_ON_BG_STYLESHEET`)
  carry a value rather than a surface, and set their own per-widget sheet, which
  wins per property. It names `min-height: 0px` outright: left unsaid, the
  shared rule would answer it and a well that grew to a field's height stopped
  sharing its row's centre.
- **Chevron rows** (`settingsChevronRow`) lead somewhere rather than setting
  something, and restate every property the shared rule would otherwise leave on
  them - the content height included.
- **The editor's placeholders** (`uiDesign::PlaceholderButton`) are tool buttons
  painting their own dashed frame, so no `QPushButton` rule reaches them.

### Menus

`menuStyleSheet(tokens, selectorPrefix)` draws every menu one of these windows
owns: the one the editor's Save Profile button drops, the search options behind
the editor's search field, the connection dialog's Copy menu and its games-list
context menu, the settings dialog's "other profiles to map to", and the context
menu over the code pane.

The surface is the `card` tone, the hairline is the field's and the corner is
`scmRadiusInput` - the corner a combo box's dropped-down list takes, because
both are a list opened out of the window it belongs to. The tone is the card's
rather than the field's for the other half of that reading: a menu is a panel
lifted off the page, where a combo box's list is the field it came out of,
opened up. Nothing in the rules names a font, since a menu is read in the font
of whatever it hangs from.

A row is a word in a box, so it takes the chip's corner rather than the menu's -
the menu's own corner repeated inside itself reads as a second frame. The
pointer's row, and the keyboard's, is `QMenu::item:selected`: `accentSoft`
behind `accentText`, which is how a combo box's list draws the row that is
chosen. An unavailable row is `disabledText`. A separator is a one pixel line in
the `separator` tone, inset by the room the rows leave at their leading end.

A row that is switched on or off carries the one mark every other choice is made
with, written out by the same builder the check boxes are drawn from - the same
size, the same fill, the same accent hairline and the same tick. The states a
menu has no way to enter, a hover and a press and a third answer, are simply
never matched. Where the mark stands, and the picture on a row carrying one
instead, is the menu's own: a sub-control is placed from the row's leading edge,
which is the very edge the row's highlight is drawn from, so both are moved into
the middle of the room the row leaves them or they sit on the arc of that
highlight rather than inside it.

The corner is the list's, cut by the same `letPopupsTakeTheFieldsCorner()` - see
the paragraph on it above. A window's style pass gives both to whatever it owns;
a menu built at the moment it is needed, which is what a context menu is, is
given both by the code that builds it, before `exec()`.

### Tabs

`tabBarStyleSheet(tabWidgetSelector, tokens)` draws a `QTabWidget`'s strip as a
row of chips lying on the page, not as the folder tabs a platform cuts. A tab is
a word in a box, the same object as a chip, a menu row and a sidebar's row, so it
carries the same three states and the same corner: `mutedText` on nothing at
rest, `hoverSoft` behind `accentText` under the pointer, `accentSoft` behind
`accentText` while it is the one on show, `scmRadiusChip` throughout, and the
accent on its border from the keyboard alone. What that says is that the strip is
a choice being made, which is what it is - a stack of folder tabs says instead
that the window is a filing cabinet.

The pane under it gets no border and no fill of its own, only `scmTabPaneInset`
of margin, so that whatever fills it - a field, in the notepad's case - opens its
rounded corner onto the page rather than butting into the window's edge. The
strip starts at that same inset, so the row of chips and the field under it begin
on the same line.

The cross on a closable tab is `editor-clear.svg`, the one x the rest of the
design is drawn with, tinted into the glyph cache by `themedGlyphFile()` for the
rule to point at - quiet at rest, `accentText` under the pointer. Where the cache
cannot be written the rule is left out rather than aimed at nothing, and the tab
keeps whatever cross the platform draws it with. A cross is on the strip only
while it can close something: the notepad keeps its last note, so a sole tab
carries none, and both tabs carry one again the moment a second note is made.
Closing a note that has text asks first, with Return and Escape both keeping it,
since nothing keeps a copy of a closed note; an empty note goes without a word.

The buttons a strip too crowded to fit scrolls with are left alone: their arrows
are a sub-control of a `QToolButton` the bar makes for itself, and giving those
buttons a face without also replacing the arrows would leave the reader a blank
square to press.

`prepareTabStrip(pTabBar)` is the caller's one job, and it says the two things a
rule cannot. The base, because in document mode the macOS style fills the whole
bar with a band of its own behind the tabs - neither the page the chips lie on
nor anything a rule asked for, and not something `background: transparent` takes
away. And the box the cross is drawn in, which is the button widget's own size
rather than anything the `::close-button` rule holds: Qt's close button asks the
style for `PM_TabCloseIndicatorWidth` in its constructor and resizes itself to
the answer, and `QStyleSheetStyle` has no case for that metric, so the rule's
`width` and `height` are never read and the style underneath answers - twenty
pixels under Fusion, which the dark appearance is drawn on, fourteen under the
macOS style. The picture the rule points at is painted into whatever rectangle
the button ended up with, so on a twenty pixel button an eight pixel mark comes
out at eleven. The moment to say otherwise is `QEvent::ChildPolished`: the
button's `sizeHint()` polishes itself before it asks the style anything, which
sends that event to the bar while the constructor's `resize()` is still ahead -
and a resize is bounded by the widget's minimum and maximum size, so a box fixed
from the event is the size the button keeps and the size `setTabButton()` lays
the tabs out from.

The keeper moves that box as well as sizing it, after every layout of the bar,
because Qt places it outside the chip. A `::tab` rule with a box makes
`QStyleSheetStyle` answer 0 for `PM_TabBarTabHSpace`, and its
`SE_TabBarTabRightButton` hands `QCommonStyle` the tab's raw rectangle - only
`SE_TabBarTabText` is given the rule's contents rect - so the cross comes out at
`tab->rect.right() - width`, which is `scmTabGap` past the drawn chip and a
further `scmTabPaddingHorizontal + scmInputBorderWidth` outside the box the word
is laid in. The rule's `subcontrol-position` decides the side and nothing else,
so the reader got a cross flush against the chip's edge beside a word with ten
pixels of air. `QTabBarPrivate::layoutTab()` applies that rectangle with
`move()`, which makes `QEvent::Move` on the button the one moment after every
layout, and the keeper puts the cross back on the word's own padding line there.
Its own `move()` sends a second `Move`, so the wanted position is compared before
it is asked for.

#### The profile strip

The row of profile tabs across the head of the main window, and of every
detached one, is the same chip - but `TTabBar` paints it rather than being handed
a sheet. Any `QTabBar::tab` rule sends the tab to `QStyleSheetStyle`, which draws
it through `QWindowsStyle` and never reaches the bar's own style, and a
stylesheet has no per-tab pseudo-state - so the tab would lose both the bold,
italic or underlined name that says a profile has new output and the connection
indicator beside it. Replacing `QTabBar::paintEvent()` in a subclass is not open
either: the offsets a tab is dragged and reordered by are private to Qt.

So `TStyle` draws the chip from the same tokens and the same constants the sheet
uses. `scmTabPaddingVertical`, `scmTabPaddingHorizontal`, `scmTabGap`,
`scmTabStripInset`, `scmTabCloseBoxSize` and `scmTabCloseGlyphSize` live in
`src/uiDesign.h` for that reason - a measurement two strips share is one number,
not two that agree today. The three states are the ones every other chip carries:
nothing behind a `mutedText` word at rest, `hoverWash` behind `accentText` under
the pointer, `accentWash` behind `accentText` while it is the profile on show -
and down that chip's leading edge the bar the settings sidebar draws on its
chosen row, through `paintAccentBar()` cut to the chip's own corner, because on
a dark page the wash alone did not say which profile was on show. The chosen
chip's word is bold as well, as the sidebar's chosen row is - bold on a resting
chip says that profile has new output, and the wash is what tells the two
readings apart - and every tab is measured bold whether or not it is drawn that
way, so the strip does not step sideways each time the choice moves.
The chosen chip is also outlined all the way round in the accent, at
`scmInputBorderWidth`, stroked after the wash and after the bar so the bar's
outer edge is the outline's. A border elsewhere in the design says where the
keyboard is, but this bar takes no keyboard focus, so on it a border can only
mean chosen - and on the light appearance the wash alone was too pale to say
which profile was on show (2026-09-10, the user's call, against the settings
sidebar's chosen row as the reference; that row shows the same 1px accent border
while it holds focus). The width was always in the metrics, so nothing moved to
make room for it.

On a light page the chip the reader chose is not washed at all: it is filled, and
its word, its cross and a disconnected profile's ring are written on that fill in
white, as the platform's own chosen tab is. The fill is the accent taken towards
black until `field` - the white Qt writes every well in the window with - reads
on it at the text floor, which is `readableOn(tokens.field, tokens.accent,
tokens.text, scmTextMinimumRatio)`: the walk ends at black, which white clears
many times over, so it always lands on a colour rather than falling back. Nothing
is left for a bar or an outline to say there, both being the accent on the
accent, so neither is drawn. The wash, the bar and the outline above are the dark
page's treatment: a solid fill would be the loudest thing in that window, and the
wash there already reads as the settings sidebar's chosen row does. On the light
page it did not, even with the bar and the outline, which is what took that
appearance the rest of the way (2026-09-11, the user's call).

The sidebar's row also reads darker than the chip's word did, and for a reason
that is not the border: it lies on the white pane, where `accentText` clears far
more than the 4.5:1 it was walked to. `themeTokens()` walks that ink only to the
floor, and against a wash on the strip's grey page the floor is where it stays,
so the accent fades into its own wash. The chosen chip's word is therefore walked
a second time - `readableOn()` against the wash as it is actually composited over
the page, up to `TStyle::sChosenWordMinimumRatio`, the 7:1 the sidebar's row is
given for free. A hovered chip's word is still plain `accentText`: it says where
the pointer is, not which profile is on show.

The cross is the same `editor-clear.svg` as the sheet's, through
`themedGlyphPixmap()` - the call the cache files are written from as well, so a
painted cross and a rule's cannot come apart. It trails the word on every
platform, as it does on the notepad's strip: macOS alone puts a close button in
front of a tab's name, and a cross in the leading position reads as a bullet
until it is pressed.

The chips share the width of the bar between them, as this strip always has:
`QTabBar`'s own expanding layout is left on, and each chip holds its word centred
in what the indicator at its leading padding edge and the cross at its trailing
one leave. `SH_TabBar_Alignment` stays `Qt::AlignLeft` for the one case that is
still the alignment's to decide - a strip too narrow to fill, which begins at the
leading edge as every other row in the design does.

Where the row of chips begins is the two windows' business rather than the bar's.
`QTabBar` lays its first tab out at x = 0 and ignores its own contents margins,
so `mudlet.cpp` and `TDetachedWindow.cpp` each put the bar in a horizontal layout
holding `scmTabStripInset` of leading margin - the same inset the notepad's pane
and strip start at.

The connection indicator's inks are `stateColor()` for the reading - working,
connecting, broken - walked with `readableOn()` at the quiet floor against what
the chip it stands on is actually filled with, so a dot means the same thing in
either appearance and does not disappear into an accent fill. A disconnected
profile is a ring rather than a fill - nothing is happening, and nothing is
wrong - drawn in the same ink as the word beside it: `mutedText` at rest, and the
field's white on a filled chip.

Mudlet sets no stylesheet of its own on this bar; `setProfileStyleSheet()` puts
the profile's there. A script's sheet that names `QTabBar`, through that or
`setAppStyleSheet()`, keeps the last word, exactly as before the design:
`TStyle` hands the tabs to the application style, a `::tab` rule of theirs takes
the tab over, font size included, and a `::close-button` rule takes the cross.
With neither, Qt's stylesheet style falls through to `TStyle`, which is its
base. `ProfileTabBarStyleTest` guards the lot.

### One sidebar for both windows

The panel down the left is one component, not two that resemble each other.
`sidebarStyleSheet(listName, separatorName, itemColor, metrics, tokens)` draws
all of it: the pill per item, the hover wash, the chosen item's gradient and its
accent bar, the ring that says the list holds the keyboard
(`[settingsFocused="true"]`), the collapsed variants (`[settingsRail="true"]`)
and the divider rows. `setSidebarCollapsed(pane, list, separatorName, collapsed,
metrics)` is the other half - the pane's width and margins, the `settingsRail`
property that both those rules and `SidebarItemDelegate` read the mode off, and
the re-polish that makes it take. It answers whether anything moved, so a window
with more to do at that moment can skip it too: the settings dialog hides its
wordmark and offers the hidden names as tooltips, the editor's rows already
carry a tooltip naming their shortcut.

Most of the measurements are the component's rather than a window's, and live
beside the recipe in `src/uiDesign.h`: `scmSidebarRailWidth` (46),
`scmSidebarPadding` (12), `scmSidebarRailPadding` (6),
`scmSidebarSeparatorInset` (12), `scmSidebarRowHeight` (36) and
`scmSidebarIconSize` (18), the glyph both draw a row with. What a row comes to
beside its name is measured rather than written down - `sidebarRowWidth()`,
below. `SidebarMetrics` then carries what a window has a reason of its own for:
the expanded width, which each measures for itself, and the vertical padding,
which is the inset that window's own columns start at (12 in the editor,
`scmEditorColumnTopInset`; 16 in the settings dialog, whose sidebar leads with
the wordmark row). The `itemColor` parameter beside it
carries the only other difference, the colour an unchosen name is written in:
muted in the editor where all the chrome is, full strength in the settings
dialog where the sidebar is the navigation. The accent bar is a gradient stop
rather than a `border-left`, which would be drawn as an arc where the pill's
corner radius is and pinched to nothing at both ends; a stop is a *fraction* of
the item, which is why those widths have to be known numbers.

Both windows measure their expanded width off the widest of their rows -
`editorSidebarWidths()` and `measuredSidebarWidth()` - and a row is measured by
`uiDesign::sidebarRowWidth()`, plus the pane's padding, clamped between the rail
width and a ceiling of their own (180 in the editor, 232 in the settings dialog,
which is the flat width that dialog used to be held to). A row is what the base
style says an item of that name, in the bold a chosen row is drawn in, and the
list's own icon needs - `sizeFromContents(CT_ItemViewItem)`, asked with no
widget so a stylesheet style defers to the style underneath - plus the two
things the sheet above puts on a row and no style can know about: the accent
bar and the `::item` padding beside it. That bold is `SidebarItemDelegate`'s
rather than the sheet's, because a stylesheet's font on an `::item` never
reaches the painter, which lays a row's name out in the style option's own font.
Not a constant, because what a style leaves round an item's text is its own: the
same row of the same name measures 88px under Fusion and 96px under the macOS
platform style, which leaves two pixels either
side against four
(`PM_FocusFrameHMargin`, which `QCommonStyle` then draws the name inside again).
Held to one number for both, the editor's sidebar drew "Statistics" and
"Variables" as "Statist..." and "Variabl..." in light. An interface font, a
translation's longer names and the style itself all move the answer, so both
windows drop it on a language, style or font change and take it again.

Both also carry the same control on the seam: `uiDesign::SidebarToggle`
(`src/SidebarToggle.h`), a painted pill with a chevron pointing the way the
sidebar will go, a child of the shell holding the sidebar and what is beside it
rather than of either - either would clip the half of it that overlaps the
other. It is where Finder and VS Code put the same control; a toolbar is no
place for it, since the user can drag one to another edge of the window. What it
is called is what will become of the *names*, never of the sidebar, which stays:
"Minimise the sidebar to icons" and "Show the sidebar's labels", and on a window
too narrow to draw the names it is disabled and says so.

Its choice is kept per window: `editorSidebarLabelsShown` and
`settingsSidebarLabelsShown`, both defaulting to true, so either window opens
with its names showing until the user minimises them. Both are labels-shown
preferences, since the sidebar has no closed state to remember. The space-driven collapse sits on top of that and writes nothing down:
`collapsed = !(namesFit && labelsShown)`, so a stretch of work in a small window
never decides what the next session opens with.

A chosen row in the editor's item trees carries the same bar, at the same width
- `scmAccentBarWidth` in `src/uiDesign.h`, so the two lists cannot come to
disagree about it - and it reads the same: one straight stroke down the row's
leading edge rather than a bracket. It is neither a stop nor a border. A tree row
is as wide as the panel happens to be dragged to, so there is no fraction to
write a stop at; and a `border-left` follows the pill's corner radius, bending
inward at both ends until the bar is pinched to nothing.
`EditorTreeDelegate::paint()` fills the rectangle itself, over the pill the style
has just drawn - the full height of the row, so the two ends are square, where
the sidebar's is clipped by its own pill and rounds off. The transparent
`border-left` stays on every row, never coloured, because it is what holds the
gutter the bar stands in - the row's padding gives back what it takes, so nothing
steps sideways when a row is chosen and the delegate's dot, chevron and mark stay
where they were.

`treeWidget_variables` is drawn by a delegate of its own, `VariableTreeDelegate`
(`src/VariableTreeDelegate.h`), because what it shows is what the Lua
interpreter holds rather than what the profile is made of: nothing in it is on
or off. Its row is the same grammar - the same pill, the same straight bar
painted by the delegate, the same delegate-drawn chevron and indentation, from
the measurements both delegates take from `src/EditorTreeRowMetrics.h` - with
the two slots at the row's leading edge read differently, which "One mark, one
size" below describes.

The Folders view, which a profile turns on in its settings, is a sidebar row
after Variables (`editor-folder.svg`), hidden while the setting is off, and an
eighth tree, `treeWidget_folders`, holding items of every type grouped by folder
name. It is drawn by `EditorTreeDelegate` built with `TreeType::None`: each row
names its type in `EditorTreeDelegate::scmRole_rowTreeType` beside its id and
reads its state off that item as a row of the item's own tree does, and a row
naming no item - folders of several types merged into one, or "Not in a folder" -
carries the folder mark and no dot, keeping the dot's room so its mark lines up
with its siblings'. The tree has no heading row, so its top rows are held in one
step as an item tree's first rows are. The item type is the tree's hidden second
column, written in `mutedText` at the row's trailing edge in whatever room the
name leaves.

### Radius follows control size

How round a corner is says how big the thing behind it is. The same 8px that
reads as a card's corner turns a chip into a lozenge, and the 4px that suits a
chip leaves a card looking square - so the radius is proportional to the control,
and the scale is four named constants in `src/uiDesign.h`. No rule writes a
radius of its own.

| Constant | Value | Use it for |
| --- | --- | --- |
| `scmRadiusChip` | 4px | A word in a box: the ID beside an item's name (`#frameId`), the kind beside a search result (`SearchResultDelegate`), the compile state over the code pane |
| `scmRadiusInput` | 5px | The controls a form is filled in through, a little under 30px tall: line edits, combo boxes, spin boxes - every rule in `inputStyleSheet()` |
| `scmRadiusPanel` | 8px | The boxes a window is laid out in: `settingsCard` group boxes, the migration banner, the editor's notice frame, the deep-link spotlight ring |
| `scmRadiusProminentInput` | 8px | A search field: the one control a panel is headed by rather than one of several filled in on it, and drawn taller than a form control, so it takes the corner of the panel it heads (`#settingsSearchField`, `#editorSearchRow QComboBox`) |

Sizes not in the scale stay off it on purpose: the 6px of a hovered toolbar
button (`scmToolBarButtonRadius`, named beside the bar it belongs to) or a
navigation row, the 8px pill of a sidebar item, the 3px of a
marker-pen highlight. Those are rows and glyphs, not the boxes this scale is
about - and a check indicator's own 3px is `scmChoiceBoxRadius`, named beside
the mark it belongs to rather than on this scale, since a radio button takes
half its own size there and comes out a circle.

### Font sizes come from the type scale

A stylesheet names a size only through `uiDesign::typeSize()`, which answers a
whole point size for one of four steps:

| step | ratio | at a 13pt base | what it is set on |
| --- | --- | --- | --- |
| `TypeStep::Caption` | 0.85 | 11pt | chips, captions, the version and copyright lines, a card's or a section's note |
| `TypeStep::Body` | 1.0 | 13pt | everything not named here |
| `TypeStep::Title` | 1.15 | 15pt | a section title, a hero headline, a search header, the sidebar wordmark |
| `TypeStep::Display` | 1.45 | 19pt | the page title over a settings page, and nothing else |

```cpp
qsl("#settingsPageTitle { font-weight: bold; font-size: %1pt; }").arg(QString::number(typeSize(TypeStep::Display)))
```

**In points, because Qt's stylesheet parser reads `pt` and `px` for `font-size`
and nothing else.** A percentage is dropped without a warning, and the rule reads
as though it sets a size while setting none: the 28 `font-size: N%` rules this
tree carried across the settings dialog, the About dialog and the script editor
had never once applied, so every word in all three windows was the interface font
at its own size. Whatever is written has to be a size the parser takes.

That does not cost the accessibility a percentage was there for. The base is
`QApplication::font().pointSizeF()`, read every time `typeSize()` is asked - so a
larger interface font moves all four steps with it, and a sheet rebuilt after a
font change comes out at the new sizes. A size *written* into a rule is the thing
to avoid, and `DesignColourLiteralTest` fails on one.

**Four steps, and no fifth.** Two things are set apart only by a difference a
reader can see; a step between Caption and Body is not one. The 96% rules that
used to sit there - a maker's description, the thanks paragraph - are Body now.
Whole points rather than a fraction, so a height measured off a step (the corner
of an ID pill is half of one) stays an integer.

The one size on a designed surface that is not a step is the notepad strip's
`stripWordPointSize()`: those words line up with the platform's *tool button*
font, which on macOS is not the application font, so the number is read off
`QApplication::font("QToolButton")` and rounded to a whole point.

### Cards, not bare group boxes

Content is grouped into cards: a `QGroupBox` carrying the `settingsCard` dynamic
property, styled with a background, a 1px border, a `scmRadiusPanel` radius and
16px padding. Pages are a single column of cards with 16px spacing
(`createScrollPage()`, `buildPage()`). The script editor has no cards: its forms
are rows. On the Mapper, General and Privacy pages every label-and-control row
is a `uiDesign::FlowLayout` with its items centred on the line
(`reflowMapperCards()`, `reflowGeneralCards()`, `reflowPrivacyCards()`), so a
label and its controls keep their own widths and the row wraps where it runs out
of room rather than widening the column, check boxes that stood side by side are
columns as wide as their own words that stack whole when they do not fit, and
the two colour cards - the Main display's and the map's - are one
`uiDesign::PairGridLayout` (`reflowColourCards()`): label, well, label, well
with every well one width, one pair to a row where two do not fit.

The card is one component for both windows, the way the sidebar is.
`cardStyleSheet(metrics, tokens)` draws the frame and the title inside it, and
`cardIndicatorStyleSheet(cardProperty, tokens)` the check indicator a checkable
card's title begins with - a sheet of its own, because the title height the
frame reserves room for has to be measured with those rules already in force.
`CardMetrics` carries what the two windows differ by: the property the rules
select on (`scmProp_settingsCard` or `scmProp_aboutCard`, interpolated into the
selectors rather than spelled out again), the padding, that measured title
height, and the two variants only the settings dialog has so far - a plain
property (`scmProp_settingsCardPlain`) and the flattening of a group box the
`.ui` file nested inside a card. Everything else - the surface, the hairline, the
corner, where the title goes, what the indicator is drawn as - is the same in
both and comes out of those metrics and the tokens.

The title is the card's first line **inside** the frame, not a heading above it:
`subcontrol-origin: padding` with `left` and `top` set to the card's own
padding, so the title starts on the same left edge as the controls under it. A
stylesheet reserves no room for a title placed that way, so the card's top
padding does it - `padding + measuredCardTitleHeight() + scmCardTitleGap` -
and getting that number wrong draws the first control over the title. A
checkable card's title line begins with its check indicator on that same left
edge and the words after it, which is where the style puts them; nothing insets
a plain card's title to match, because on a card whose controls all start at the
padding edge that would indent the heading away from what it heads.

### Human copy

Controls state what happens in words a player understands. A card may carry a
description label (`settingsCardDescription`) under its title. Keywords and
descriptions are `tr()`-wrapped with `//:` translator notes.

### Measured responsiveness

Breakpoints are computed from real content, never hardcoded pixels. The pattern
is `sidebarWidths()` / `updateSidebarMode()` in
`src/dlgProfilePreferences.cpp`:

- Widths are measured off font metrics, `sizeHint()`s and the actual scrollbar
  width, over every page rather than the one on show, so a page switch does
  not move the collapse point - unless the page on show is wider than the
  reading column, when the collapse point follows that page and the names fold
  rather than the page scrolling sideways. The window's minimum is where even
  the rail leaves the widest page too little room.
- The window has no maximum: a page's column grows with the window and its
  cards use the room, so the reading column is the floor the names fold at and
  never a ceiling. `fullyExpanded`, the names beside the widest page's column,
  is the most a window with no remembered size opens at.
- The test is against the window's own width, not the space left over, so
  collapsing cannot flip the condition that caused it.
- The measurement is the per-window half. What is done with its answer is
  `uiDesign::setSidebarCollapsed()`, shared with `editorSidebarWidths()` /
  `updateEditorSidebarMode()` in `src/dlgTriggerEditor.cpp`. Both windows
  measure the same two things: the longest row name, which is what the sidebar
  is drawn at with its names showing (`measuredSidebarWidth()` here,
  `editorSidebarWidths()` there), and the page beside it, which is what the
  breakpoint is.

#### The editor's actions toolbar gives its names up, never its actions

**The bar's look is a recipe, not the editor's own.**
`uiDesign::toolBarStyleSheet(toolBarSelector, seam, tokens)` draws every flat
bar in the application: the page's surface with the hairline seam on the edge
`ToolBarSeam` names - `Bottom` for a bar across the top of a window, `Top` for
one at its foot - the `::separator` between two groups, the `::handle` grip from
`gripGlyphFile()`, and the buttons themselves, flat and frameless at rest, in
`hoverSoft` under the pointer and `accentSoft` while held, their words in
`accentText` either way because the glyph beside them is inked that for
`QIcon::Active`. The measurements are `scmToolBarGripExtent`,
`scmToolBarButtonRadius` and `scmToolBarButtonPadding*` in `src/uiDesign.h`. A
toolbar cannot call `buttonStyleSheet()` instead: its buttons are the flat kind
a bar draws, and the button rule would give each of them a face and a frame.
Whatever else a particular bar carries - the editor's overflow button and its
split Save Profile - follows the shared part in that window's own sheet. The
script editor and the notepad are the consumers.

A `QToolBar` too narrow for what it holds posts the tail of itself into a
drop-down behind a chevron a few pixels wide at the far edge of the window.
Nothing says it is there. `fitEditorToolBarToItsLength()` in
`src/dlgTriggerEditor.cpp` takes the names off instead, and nothing is ever
hidden while a word is still written out:

- The bar's buttons are held in groups (`mEditorToolBarGroups`), and that list
  is the order they give their names up in: the four that act on the profile -
  Import, Export, Create Module, Save Profile - first, the five that act on the
  item being edited after - Add, Add Group, Save, Activate, Delete. Undo and Redo
  are pictures from the start.
- A collapsed group is `Qt::ToolButtonIconOnly` per button, through
  `toolBar->widgetForAction()`; the split Save Profile button keeps its menu
  half. That half is drawn by `uiDesign::splitButtonMenuHalfStyleSheet()` - the
  same recipe the connection dialog's Copy button takes, at the bar's own
  `scmToolBarButtonRadius` corner - so the seam, the wash and the Lucide chevron
  are one look in both windows. It is the only button on the bar named
  (`editorSaveProfileButton`): the padding that holds its words clear of the
  half would otherwise reach all
  eleven. Every one of the nine leads its tooltip with the words it would
  otherwise be carrying, which for four of the item five is the view's wording -
  "Add Trigger", "Add Alias" - rebuilt by `updateEditorItemActionToolTips()`.
- **Activate says what a click does to the chosen row**: "Activate" on a row that
  is off, "Deactivate" on one that is on, including one on inside a group that is
  off, since the click flips the row's own switch. `updateEditorActivateAction()`
  reads the row from `EditorTreeDelegate::stateOf()`, the answer the dot is drawn
  from, and runs on everything that moves the choice or the state: a tree's
  current row and its model's `dataChanged`, the undo stack's index, the five
  `Host::signal_*Toggled` (a hidden editor skips the repaint those ask for), and
  the view and folder-group changes. It is disabled with no item chosen - a
  tree's heading, Variables, a folder group. The tree menu carries the same
  action, so the two never disagree. The button is held to the wider of its two
  wordings (`scmProp_editorToolBarWordings`, `holdToolBarButtonWidth()`) so
  Delete, Undo and Redo stay where they are as rows are chosen; as a picture it
  is not held. `EditorActivateActionTest` holds all of it.
- What is measured is `toolBar->layout()->sizeHint()` along the bar's
  orientation against the bar's own width or height, so separators, the grip,
  the spacing and a stylesheet's padding are counted the way Qt counts them, a
  bar docked at a side is measured down its length, and the answer does not
  change when Qt has already folded something away.
- Names come back from the other end, and only with `scmEditorToolBarRestoreMargin`
  (16px) to spare. Giving them up costs nothing to spare, so the two tests never
  agree on the same pixel and a drag across the breakpoint settles.
- It runs on the bar's own resize (an event filter, since the bar is the only
  thing the window tells), on the icon size preference, on the view changing
  (four of the item five are renamed there), on a language, style or font change, on
  `applyEditorShellStyle()`, and once the window has first been shown. Measured
  at 39us a call, so a resize drag is answered rather than coalesced.
- Torn off into a window of its own the bar starts from every name shown, since
  a float is sized to what it holds; the resize that gives it that size fits it
  again.

Measured on the offscreen platform at the default 18px glyph: the profile group
gives its names up at 1192px of window and the item group at 920px; at the
largest the preference offers, 24px, at 1256px and 984px.

**The fold is left in place and is reachable.** A bar with every name given up
still wants 474px of window at 18px glyphs and 540px at 24px, against a window
whose own minimum lets it be dragged down to 279px and below - so between those
figures Qt does fold, on a window far too narrow to edit anything in. A
scrolling host of our own would only be a second way of hiding the same buttons,
so what is done instead is to
make the button unmistakable, with the same `uiDesign::ToolBarOverflow` every bar
has - see "The overflow button" under the main window's toolbar.
`test/functional_tests/EditorToolBarOverflowTest.cpp` sweeps the window width and
holds all of it, the numbers above included.

#### The main window's toolbar

The same recipe, on `mpMainToolBar`, on the replay bar a running replay stands
beside it, and on every detached profile window's `detachedMainToolBar`. Left to
the platform the bar read two ways and neither of them said anything: on the
native macOS style a hovered button showed no highlight at all, and on the
Fusion-based dark theme a lighter box with a hairline round it. A switched-on
Sound button was a sunken grey block with the chevron of its menu half pushed
into a corner of it.

- `mudlet::toolBarShellStyleSheet(barSelector)` composes what any of those bars
  is drawn with: `uiDesign::toolBarStyleSheet()` seamed along its bottom, then
  the split buttons.
- **The overflow button.** When a bar is too narrow, Qt folds its tail behind
  `qt_toolbar_ext_button`, which it leaves as an unlabelled sliver: testers took
  that for a scrollbar and thought the icons were missing. `uiDesign::ToolBarOverflow`
  makes it a button of the bar on the main toolbar, the replay bar, every detached
  window, the editor, the notepad and the debug filter bar: the chevron (down while
  folded, up while the row is open, right and left on a bar docked at a window's
  edge) and "3 more" in the bar's own button style, so the glyph alone at the 16px
  size, and the glyph above the word where a bar that sets them side by side is
  docked at an edge, since the button then only has the bar's width. `toolBarStyleSheet()` carries its two rules and no window's sheet names it:
  the font size, since macOS gives the bar's own buttons a smaller font, and no side
  padding. Qt hides at most one more button to make room for this one, and when that
  button is the narrower of the two it tightens every gap in the row from 2px to 1px,
  so the width stays near the bar's narrowest buttons. The count is read off the bar's buttons
  once Qt has laid them out, leaving out separators, spacers and hidden actions, and
  holds while the row is open, when every button is on show. `QToolBarLayout` takes
  the room from `PM_ToolBarExtensionExtent` and offers no other way to be told, so
  the bar carries the width in `scmProp_toolBarOverflowExtent` and `MudletProxyStyle`
  answers with it; `DarkTheme` wraps that same proxy. It is measured for the longest
  the word can be, so a count changing cannot move the point where the bar folds.
  Where a second row would not fit in the window Qt swaps the row for a menu, which
  takes `menuStyleSheet()`. `MainToolBarStyleTest` holds it.
- **The split buttons are picked out by their popup mode, not by name.**
  `QToolButton[popupMode="1"]` is `QToolButton::MenuButtonPopup`, which is how
  Qt's own documentation reaches them: Connect, Sound and Packages are three of
  them on this bar, a detached window builds its own copies, and an addon's
  command button can be a fourth. Each gets `scmToolBarButtonPaddingHorizontal +
  scmInputDropDownWidth` of trailing padding to hold its word clear of the half,
  and `uiDesign::splitButtonMenuHalfStyleSheet()` at the bar's own
  `scmToolBarButtonRadius`, so the seam, the wash and the chevron are the same
  look the editor's Save Profile carries.
- **A checked action is lit rather than sunk.** `toolBarStyleSheet()` grew a
  `QToolButton:checked` rule - `accentText` on `accentSoft`, what a pressed
  button takes - because a checkable action that is on is held down in every way
  but the pointer. Sound, Full Screen, MultiView and the compact input line are
  the ones that reach it, and `tintedIcon()` has already inked their `QIcon::On`
  glyph `accentText` to match.
- **A script's stylesheet that names the bar's classes takes the bar back.** The
  design's rules sit on the bar itself, where they outrank any application sheet
  whatever its selectors, and a profile sheet's plain type selectors; scripts in
  the package repository (dark-theme-for-mudlet, PRS, arkadia) restyle the bar
  with exactly those. So `mudlet::restyleMainToolBar()` leaves the design's rules
  off whenever the application's sheet or the profile's names `QToolBar` or
  `QToolButton`, and the bar carries the profile's sheet alone, as it did before
  the design. Otherwise the profile's sheet, kept in
  `mMainToolBarProfileStyleSheet`, is appended after the design's rules on the
  same widget. The replay bar and a detached window's bar step aside for an
  application sheet the same way.
- `restyleMainToolBar()` runs from `restyleToolBarIcons()`, which is what every
  appearance change already calls, and a detached window's bar is redrawn by its
  own `restyleToolBarIcons()` for the same reason. The replay bar is named
  `mpToolBarReplay` so it can be scoped to; it carries no profile sheet.
- While an update waits, `slot_updateAvailable()` swaps the About action for
  `aboutUpdateButton`: the Lucide "!" in a circle (`toolbar-about-update.svg`) and the
  word "Update". Both take `scmStateHue_update`, walked to `scmTextMinimumRatio` since
  the word carries the ink too, and keep it under the pointer. Qt's menu arrow is hidden
  because it is drawn over the word. The hue is apart from `scmStateHue_ok`: an update
  waiting is never read beside "broken", and the "!" and the word say it without colour.

`test/functional_tests/MainToolBarStyleTest.cpp` holds it: the sheet's
composition, and the hover wash, the accent wash and the seam on the menu half
read off a grab in both appearances.

#### The console's foot

A console's foot - the command line, and in the main window the search box,
buttons and latency readout beside it - is parted from the text above by one 1px
seam across its whole width, `blend()` of the profile's command-line background
towards its text colour at `scmConsoleFootSeamStrength` (20%), with no frame on
the input or on the password box laid over it, so a border a profile sets with
`setCmdLineStyleSheet()` is the only one drawn there (`ConsoleFootSeamTest`).

### Shell over .ui

The `.ui` file is not rewritten. Existing widgets are detached from their
designer layouts and moved into a runtime-built shell:

- `detachFromLayout(pWidget)` - removes a widget from its parent's layout tree.
- `moveIntoCard(pCard, controls)` - reparents controls into a card's layout.
- `buildPage(objectSuffix, cards)` - detaches cards, marks them `settingsCard`,
  stacks them in a scrolling column.

This keeps object names, signal connections and every translated string intact,
so a redesign costs no translation churn and no `.ui` merge conflicts.

The editor's five field-only forms - aliases, timers, keys, scripts, variables -
are shelled the same way in `buildEditorFormHeadRows()`
(`src/dlgTriggerEditor.cpp`): the name, the command and the ID pill are lifted
out of each `.ui` grid into a head row, `insertGridRowAtTop()` puts that row
above what is left of the grid, and the rows under it are built in place from
the grid's own controls - the timer's four fields into a sentence, the key's
field into one that listens, the variable's two pickers onto one row. A control
that is replaced rather than moved (the script's event list, which became a row
of chips) leaves the `.ui` file, and the runtime builds its replacement into the
cell the label leads. The `.ui` files did lose their colons and their caps on
field heights in that pass: a string that changes anyway is not churn.

The Buttons form is the sixth, and its `.ui` file was rewritten rather than only
shelled: its three group boxes were a column of toolbar settings beside a column
of button settings with a stylesheet box under both, which left its labels at
two x positions and neither of them the one the other forms type at. They are
rows of one grid now, and `buildActionRows()` builds the only cell holding more
than one control - the rotation picker with the push-down switch beside it.

### The rows of a form

Every form the editor fills in leads with the same row: the name, whatever is
typed beside it (a command, where the item has one), and the ID pill. The
trigger form's `widget_top` is the original; `buildEditorFormHeadRows()` gives
the other six forms the same row at the same measurements, so a view switch
never moves the Name field. Under it, each row leads with one word.

- **Lead labels share one width.** `alignEditorFormLeadLabels()` measures the
  widest lead word across the forms in the font the window is running at and
  gives every lead label that width, and every Name label that width plus the
  difference between the head row's spacing and the grid's - so a field on any
  row of any form starts at the same x. It runs at style time and again on a
  font change. Labels carry the `editorRowLabel` property and take the quiet
  ink; they lost their colons in the same pass.
- **The ID pill** (`#frameId`, `editorIdChip`) is `styleEditorIdChip()` on all
  six forms: `chipFont()`, a corner of half its measured height, drawn from the
  shared `formRules` rather than once per form.
- **A sentence round a control.** `buildControlSentenceRow()` lays words and
  controls into one row from a translated template - one control with `%1`, or
  several with `%1`..`%n` wherever the translation puts them; a placeholder the
  translation lost still leaves its control on the row. The timer's interval is
  one: "Fires every %1 h %2 min %3 s %4 ms", or "Fires once, ... after the timer
  above it fires" for an offset timer (`editorTimerInterval`). The words are
  scaffolding and take `editorRowLabel`; each control keeps an accessible name
  of its own, since a screen reader never reads the word beside it.
- **A field that listens.** The key binding is a read-only field that arms the
  editor's key grab when clicked, or on Return or Space: it carries
  `editorListening` while it waits, the accent frame and wash with it, its
  placeholder says what to press, and a hint beside it (`editorKeyHint`) says
  what happens next. Escape, losing the focus, a save, a view change or the
  window hiding all end the grab through `endKeyGrab()`; a cross
  (`editorKeyClear`) forgets the keystroke. The grab keeps the keypad modifier.
- **Chips** (`uiDesign::ChipRow`, `src/ChipRow.h`, over `uiDesign::FlowLayout`)
  hold a set of short names the user adds and takes away: a script's events.
  One chip per name with its own cross, and a dashed "Add event" that becomes a
  field in place; Return or a comma commits and keeps the field open for the
  next name, Escape closes it, losing focus keeps a typed name and drops an
  empty one, a name already listed is refused with a note (`editorChipNote`)
  beside the field. Chips wrap, and the row reports the wrapped height through
  its size hint and `heightForWidth()`, so the column it is in follows. A
  script's own event names keep the full `text` tone - they are content, like
  the words in a field - and an event Mudlet raises itself reads in `mutedText`
  (`editorChipSystem`), whatever it is called (`mapOpenEvent` has no `sys`).
  The tone and the tooltip on a chip's word come from where the editor's
  suggestions say its name is from, set again whenever they arrive.
  `chipFont()` is the one recipe for a word in a box: the
  ID pill, the chips, the hidden-variables count, the value preview in the
  variables tree.
- **Rows rather than boxes.** The Buttons form's rows are Rotation - with the
  push-down switch beside the picker - Command, Command up, Icon (built but
  never shown, until Mudlet can put pictures on buttons again), Location,
  Orientation, Rows (Columns, where the toolbar stands on its side), Offset and
  Stylesheet. Which of them an item has is what the item is: a toolbar is laid
  out, a menu and a button are pressed, a module's master folder is neither and
  carries only the stylesheet the module is drawn with. `showEditorFormRow()`
  takes a word and what it leads away together, since a grid row is only out of
  the way when everything on it is; the Command up row also comes and goes with
  the switch, and the Offset row with there being lines to offset into. The
  stylesheet editor holds the grid's vertical stretch, so the room a drag on
  this view's seam gives the column goes to it and never between the rows.
- **A trigger's options are one of those rows**, the options strip, led by
  "Options" (`editorOptionsRow`), between the head row and the pattern list. It
  is a disclosure: the Options button at the right end of the head row
  (`toolButton_toggleExtraControls`, text "Options", the Lucide sliders glyph
  through `tintedIcon()`) is what opens and closes it, and the lead word goes
  away with it - `showEditorFormRow()`
  takes the pair together. That button is drawn by
  `uiDesign::disclosureButtonStyleSheet(buttonSelector, tokens)`, the one recipe
  for a control that opens and closes a strip: a button's own face, since it is
  pressed rather than typed into, the chrome tone and the border hairline at
  rest, and `accentText` on the accent's wash and hairline while it is checked -
  the ink a switched-on control already carries, so the button says the options
  are on show for as long as they are. This is the recipe's one caller: the
  notepad had the other, and gave it up (see the send strip in section 3). The
  editor opens with it closed every time, whatever
  the last session did: the state is held for the session in
  `mShowAllTriggerControls` and never stored, and the `showAllTriggerControls`
  key an older configuration carries is still cleared on write. The tab chain
  runs Name, Command, the Options button, then the strip if it is open, then the
  patterns. They were a 280px column of four cards beside the patterns, folded
  away again by the window being either short or narrow, and then two rows, which
  still wanted a very wide window before they stopped wrapping. The strip is a
  titleless `QGroupBox` carrying
  `editorOptionRow`, drawn as nothing at all: a group box because a screen
  reader is told it is a grouping and is told nothing about a bare `QWidget`,
  and the word leading it is the form's own label rather than a title. Inside
  it a `uiDesign::FlowLayout` holds one widget per group of controls, so a
  group wraps whole rather than being broken up or squeezed - 24px between
  groups, 8px inside one, and no rule drawn between them. The groups, in order:
  the word "Match" with the two segments and "within N lines" after them, all
  one group so the choice never splits across lines; "Every occurrence"; "Keep
  firing N more lines"; "Only pass matches"; "Sound" with its file field and
  cross; "Highlight" with its two wells; and the caption, last and only while
  there is one pattern. Each of those words is as short as the option can be
  said in, and the whole of it is what `setAccessibleName()` gives a screen
  reader instead: "Any pattern", "All patterns", "Every occurrence in a line",
  "Only pass matches to children", "Play a sound", "Highlight matches". Every
  item on the strip is held to `uiDesign::scmInputHeight`, because a
  `FlowLayout` puts each item at the top of the line it lands on: at one height
  each group's own `QHBoxLayout` centres what is inside it, and a check box
  beside a spin box is read level with it rather than a few pixels above. The
  two number boxes are as wide as their largest value and no wider -
  `fitEditorOptionsSpinBoxes()` measures the widest digit repeated as many times
  as 999 has digits in the box's own font and adds the field's padding, its
  hairline and the stepper column, which is 55px here against the 72px they were
  named at.
- **Two segments rather than two radios.** The matching mode is
  `mpRadioButton_matchAny` and `mpRadioButton_matchAll` still, so a screen
  reader says "radio button, 1 of 2, selected", but they carry `editorSegment`
  and `editorSegmentSide` (`first` / `last`) and are drawn as one joined
  control: the indicator is given no size and no spacing, the box round the
  words carries the state, the outer corners take `scmRadiusInput` and the inner
  edge is one hairline. The corner rules come last in the sheet so they outlive
  the `:checked`, `:focus` and `:disabled` rules above them. The pair writes
  into the hidden `spinBox_lineMargin`, which is still what the trigger is saved
  from and loaded into, and `reflectTriggerMatchMode()` is the view of it.
  "within %1 lines" is a `buildControlSentenceRow()` sentence that is dimmed in
  the Any mode rather than taken away, and both it and the segments are greyed
  out while the trigger has fewer than two patterns, with
  `mpLabel_matchModeHint` saying why on the end of the strip.
- **A switch does not gate what stands beside it.** "Play a sound" and
  "Highlight matches" are check boxes rather than checkable cards, so the file
  field and the two colour wells are live while the switch is off, and making a
  choice in one of them turns its own switch on - `acceptTriggerSoundFile()` and
  `turnTriggerHighlightOn()`, each leaving the switch's own `toggled()` to write
  the undo entry. The sound file is a read-only field (`editorSoundFile`) that
  opens the file chooser when clicked, or on Return or Space, the way the key
  binding field arms the key grab; it shows the file's name elided to its width
  and keeps the whole path in a property, since that is what the save and load
  paths read, and the cross beside it is there only while there is a file to
  forget. Every control on the strip carries `setAccessibleName()` and a
  plain-text `setAccessibleDescription()`: Qt falls back to the tooltip when
  there is no description, and these tooltips are rich text.

### The seam over the code pane

The heading over the Lua pane is a `GripSplitterHandle` carrying a strip, and
whether it also resizes depends on what the form above it holds. In Triggers and
Buttons the form has something that uses room - a pattern list, a stylesheet
editor - so the heading drags, and `fitFormPaneToItsContents()` snaps the column
to what the item asks for unless the user has dragged that view's handle in this
session (`mDraggedFormPaneHeights`). In Aliases, Timers, Keys, Scripts and
Variables the form is a fixed set of fields: `formPaneResizes()` says no, the
handle is made inert (`GripSplitterHandle::setResizes(false)` - no grip drawn,
no cursor, no drag), and `holdFormPaneToItsContents()` caps the column
`mpNonCodeWidgets` and gives the code pane the rest. A `LayoutRequest` on the
column re-runs the cap, so a notice appearing, a row hidden for a key group or
chips wrapping onto a second line all move the seam by themselves.
`EditorFormShellTest` holds both halves: a push on the handle leaves a fixed
view's column where it was, and still moves the trigger form's.

The strip is also the only place a compile error of the item in the editor is
said. `reportCompileError()` is the one way to `mEditorCompileMessage`, and both
the save of an item and the *opening* of one go through it - so a broken item
shows its note every time it is selected, not only in the session where the
failed save happened, and `clearDocument()` has already cleared the heading for
the item arriving before the note lands on it. The notice over the form no
longer doubles the note: it used to be raised for the same failure, and raised
again by every rebuild of a tree, for whichever broken item that rebuild came to
last - so the reader was told about an item they were not looking at. The mark
on the row is what says a broken item is broken while it is not the one open.
A save is bracketed by `beginSaveErrorCapture()` / `endSaveErrorCapture()` so
that the heading is written once when the save is over, which is what lets a
save that passed clear the note the last failed one left.

The form is given what it asks for: every pattern row and the Add pattern button
stay in view, and the code pane takes the rest. Only a form that asks for more
than the two panes have is held back, at the code pane's
`scmEditorSourcePaneFloor` - the number below which the pane stops being one
anything can be typed into, and its minimum height - and its pattern list
scrolls in what is left. Every place the seam is placed rather than dragged
reads that floor: the cap and the split in `holdFormPaneToItsContents()`,
`formPaneHeightForItsContents()`, and the dragged height a view is put back to
in `fitFormPaneToItsContents()`. No share is kept for the code pane beyond it:
the third it once kept held a long trigger's last rows and its Add pattern
button out of sight above a code pane with room to spare.

A new height for the splitter is the other way the seam goes stale: `QSplitter`
hands the whole change to the code pane, the one pane with a stretch factor, so
a taller window would leave the pattern list scrolling. The `QEvent::Resize` on
`splitter_right` therefore defers `fitFormPaneToItsContents()` whenever the
height has moved, through `queueFormPaneRefit()`, which runs one refit per turn
of the event loop however many resizes a window drag sends.

The trigger form answers to the width it is given as well as to the height, and
what changes there is how many lines its options strip has wrapped onto - a
`FlowLayout` gives each group of controls its own size and runs on to the next
line rather than squeezing them, so a narrower editor costs the options a line
instead of costing a pattern row the width it needs. That is a different height
for the column, so the `QEvent::Resize` on `mpTriggersMainArea` queues the same
refit whenever the width has moved, and the seam follows.
There is no loop in it: a refit moves the seam, which is the column's height and
never the form's width. The height itself reaches the seam because the strip
answers `heightForWidth()` - a `QGroupBox` over a `FlowLayout` does that on its
own, and `QGridLayout` carries it up - so `formColumnHeightForItsWidth()`
measures the wrapped strip without being told it is there.

What the column is measured with is `formColumnHeightForItsWidth()`: the
layout's `heightForWidth()` at the width the column actually has, not its size
hint. A hint is answered at whatever width the layout would like, and the
notice's wrapping label would like a narrow one - so the hint carried the height
those words take in a column a fraction of this one's width. The two places that
size the seam - the cap and `formPaneHeightForItsContents()` - both read that
one measurement.

### The notice over the form

What the notice carries is what has nowhere else to be said: an activation the
engine refused, a script that failed while the profile was loading and has since
been fixed, the warning that an item came out of a package, an undo toast. What
it does *not* carry is a compile error of the item in the editor - that belongs
to the heading over the code pane, which speaks for that item and takes its note
away when the item goes. `showError()` is the banner; `reportCompileError()` is
the heading.

An activation the engine refused because the script does not compile is the one
script error the notice still shows, and it leaves the error itself to the
heading: `showActivationFailure()` says what to do, "Fix the error on line 2,
then switch "Loot" on again", with "line 2" a link to it. Refused for anything
else, such as a pattern that does not compile, it gives the reason the item
gave.

The connection dialog's notice sits under the Information box in the details
column, not under the games list: notices differ in height, and under the list
one coming or going moved the game under the pointer between the two clicks of
a double-click, which connects.

`dlgSystemMessageArea` is as tall as its words are at the width it is given and
no taller: both its hints come from its own layout's height-for-width at its
current width, and it is `(Ignored, Maximum)` in the column - ignored across so
the label's guess at a good width cannot widen the column, Maximum down it so
what it asks for is also the most it can be given. The column is therefore
notice + `scmEditorColumnSpacing` + form, and a notice appearing moves the
form down by exactly that and changes nothing else about it. In the two views
whose seam the reader places, the seam moves by the same amount rather than the
column being measured afresh. A height the reader dragged is therefore kept net
of the notice - `slot_rightSplitterMoved()` writes down `sizes[0]` less the
notice's room and `fitFormPaneToItsContents()` adds whatever notice is up now
back on - so a notice that comes and goes leaves that height where it was put
instead of taking its own height off the pattern list each time.

Whenever the code pane is away the cap comes off instead: nothing is chosen, or
what is chosen has no value to edit - a Lua table - so the column has the pane
to itself and there is no seam to hold it off. Capped it would be a lone child
shorter than the splitter, and `QSplitter` centres one of those, which is what
put first the notice and then the form half way down the window. The five forms
that are a fixed set of fields carry no stretch in the column, so the trailing
stretch takes the spare room and leaves them leading the pane at their own
height; the trigger's and the button's forms keep theirs, because their pattern
list and stylesheet editor do use the room. `EditorNoticeSeamTest` holds all of
this, along with the pattern list resizing the pane when a row is added or
deleted.

The widget styles itself from the tokens in its own `slot_applyAppearance()` on
every appearance change and the editor writes no sheet onto it: two sheets on
one frame show whichever landed last, and the legacy one - a 3px border in the
window's text colour - is what came back over the accent hairline every time the
appearance changed.

## 2. Icons

- Monochrome line icons from [Lucide](https://lucide.dev), ISC licence,
  attributed in the About dialog (`src/dlgAboutDialog.cpp`).
- Shipped as SVG in `src/icons/` (`settings-general.svg`,
  `settings-appearance.svg`, ...), exactly as Lucide authors them.
- Loaded only through `glyphPixmap(file)`, which rasterises at
  `scmGlyphRasterSize` (128px) and draws every glyph at
  `scmGlyphStrokeWidth`. Lucide authors at a stroke width of 2, which reads
  heavy at the sizes this window draws at; the weight is a token here rather
  than a property of the asset, so changing it is one number and not 83
  re-rendered files. Results are held in `QPixmapCache` - parsing an SVG per use
  is what made the About dialog's chip row measurably slow when its glyphs were
  re-encoded per chip.
- The shape still lives in the alpha channel: `glyphPixmap()` draws onto a
  transparent pixmap, so everything downstream of it is unchanged.
- Tinted at runtime by `tintedGlyph(source, color)`: fill through the alpha with
  `QPainter::CompositionMode_SourceIn`, which keeps the antialiased edges that
  per-pixel recolouring would harden into a staircase.
- Selected-state variants are added explicitly via
  `QIcon::addPixmap(..., QIcon::Selected)`, otherwise the view washes the icon in
  the highlight colour.
- Inline copies for rich text go through `inlineGlyph(glyph)`, which base64s the
  tinted pixmap into a `data:` URI - a `QLabel`'s rich text can only reach a
  picture through a URL, and a runtime-tinted glyph has no path.
- Re-tinting on a theme change happens in one place, `restyleSidebarIcons()`,
  called from `applyShellStyle()`.

The main window toolbar is the third consumer, and the detached profile window
builds the same bar from the same list (`src/mudlet.cpp`,
`src/TDetachedWindow.cpp`). Its own glyphs are named `toolbar-*.svg`. The seven
editor concepts - Triggers, Aliases, Timers, Buttons, Scripts, Keys, Variables -
take the editor's `editor-*.svg` files rather than copies of them, so that a
trigger is the same picture wherever it is offered; the module manager takes
`editor-module.svg` for the same reason. Three glyphs come from outside Lucide,
from [Simple Icons](https://simpleicons.org) under CC0 1.0: the Discord mark
(`toolbar-discord.png`) and the GitHub and Patreon marks in the About dialog.
Those three stay PNG - they are filled shapes with no stroke, so there is no
weight to set, and `glyphPixmap()` reads any non-`.svg` file as a raster.

Each window keeps a `QList<uiDesign::ActionGlyph>` of action to file and a
`restyleToolBarIcons()` that re-inks every entry through
`tintedIcon(glyphOff, glyphOn, tokens)`. That builds all eight mode/state
pairs - `Normal`/Off in `mutedText`, `Normal`/On and every `Active` and
`Selected` in `accentText`, `Disabled` in `disabledText` - so a checkable action
that is currently doing something reads as lit. Full Screen and the Sound family
pass a second file for their On state. The split buttons keep Qt's own
`MenuButtonPopup` arrow and hover frame; nothing is drawn over the glyphs.

### Colored icons (experiment)

"Colored icons" on the Experiments card gives the glyphs on the main toolbar and
in the script editor the colours of the PNG pictures they replaced, for players
who still find an icon by its colour. It is app-wide - `mudlet::setColouredIcons()`,
kept in `Mudlet.ini` as `colouredIcons` - because the toolbar belongs to no one
profile, so the card shows in every build even where the 3D map experiments
beside it are hidden.

- Each colour is an `IdentityColour` in `src/uiDesign.h` (`scmIdentity_*`),
  measured from the old PNG: the strongest hue among its saturated pixels. A
  picture under 30% coloured counts as grey and gets none (Aliases, Timers,
  Scripts, Keys, Settings, Notepad, Replay, MultiView, Full Screen). About gets
  none either: its green is the update state's.
- The ledger entry carries the colour; `restyleToolBarGlyphs(glyphs, tokens,
  coloured)` draws it. The line is `identityInk()`, walked with `readableOn()` to
  `scmQuietMinimumRatio` on the page and the card, since a split button's menu
  shows the same icon. Under it, `glyphFillPixmap()` washes the glyph's own
  shapes at `scmIdentityFillOnDark` / `scmIdentityFillOnLight`.
- The colour stays at rest, under the pointer, switched on and chosen; the wash
  under the button or row already shows those states. Unavailable, the glyph is
  `disabledBarText` with no wash. `identityIcon()` builds all of these.
- The Connect button's menu takes Lucide glyphs too, so its four entries match:
  Disconnect (`unplug`) is grey, its picture being under 30% coloured, Close
  profile (`log-out`) takes Connect's orange and Close Mudlet (`circle-x`) the
  red of Delete, after the pictures they replaced.
- In the script editor: the toolbar (Add green, Add Group blue, Activate gold,
  Delete red, Import and Export green, Create Module and Save Profile As brown),
  the sidebar and each tree's heading row through `editorViewIdentity()` (Triggers,
  Buttons, Variables, Folders, Errors, Statistics, Debug), and a package's mark in
  the trees, which `EditorTreeDelegate::setColouredMarks()` turns the Packages
  brown. The editor follows the switch through `slot_colouredIconsChanged()`.
  Aliases takes the Input line page's green (`scmIdentity_input`) wherever it
  is shown, as its old picture was mostly grey.
- In Settings, the sidebar through `settingsPageIdentity()`. Those pages had no
  coloured pictures, so only a few are coloured: a page that opens what a toolbar
  button opens takes that button's colour (Mapper, Chat and sharing, Connection
  and Mudlet support), and Input line and Accessibility have colours of their
  own. The rest stay grey. The dialog follows the switch through
  `slot_changeColouredIcons()`.
- A glyph with dots of paint ships `<name>-dots.svg`, whose `dot-0`, `dot-1`, ...
  elements `addPaintDots()` draws over it in `scmIdentityPaintDots` in turn,
  whether the glyph itself is coloured or grey: Appearance's palette, grey with
  red, yellow, green and blue paint.
- A glyph whose lines leave its middle open fills in as broken pieces, so it
  ships `<name>-fill.svg` beside it with the shape to wash instead:
  `toolbar-reconnect-fill.svg` (the circle the two arrows run round),
  `editor-activate-fill.svg` (the top of the power circle, open for its line)
  and `editor-variables-fill.svg` (the space between the braces). These are Mudlet's
  own and never drawn as a line.
- `editor-variables.svg` is Lucide's `braces` with three dots in the style of
  Lucide's `ellipsis` between them, as Lucide has no `{...}`.

Everything else the editor draws as a picture comes from the same family and
the same tinting. The notice banner's three pictures are `editor-notice-info.svg`
(info), `editor-notice-warning.svg` (triangle-alert) and
`editor-notice-error.svg` (circle-x): tinting keeps only the shape the alpha
channel carries, and the old full-colour `dialog-*.png` bitmaps have a solid
alpha, so the information notice came out as a plain disc. The cross on a chip,
beside a key's binding and on the sound field is `editor-clear.svg` (x); the
plus on "Add event" is `editor-add.svg`; the variables tree's type marks and its
hidden mark are listed under "One mark, one size" below. The two a choice
indicator shows are `control-check.svg` (check) and `control-minus.svg` (minus),
named for the control rather than for a window because both windows draw them.
A new glyph is Lucide's own SVG file, copied in under the name this window knows
it by - nothing is rendered ahead of time.

A stylesheet cannot recolour a picture on the way in, so a rule that needs one
points at a PNG written into the glyph cache: `themedArrowFile()` for the field
chevrons and a menu button's, `themedGlyphFile()` for a choice's tick and dash,
`dotGlyphFile()` for a chosen radio button, `gripGlyphFile()` for the grid of
dots a draggable thing is gripped by. Each writes a 1x file and, on a screen
that doubles its pixels, an `@2x` twin beside it, and each puts the ink in the
file's name so that a theme change writes a new file rather than changing one a
stylesheet has already read and cached by path. The two that fill a control's
indicator draw the mark inside a transparent square the size of that indicator
rather than on its own: a sub-control scales the picture it is given down to its
contents and never up, so a mark carrying its own margin comes out at the
fraction of the box it was drawn at whichever of the two files the platform
picked.

### One mark, one size, on every row of an editor tree

A row in the editor's six item trees leads with a state dot, and beside it a
mark for what the dot cannot say: `editor-folder.png` for a group,
`editor-filter.png` for a trigger other triggers are matched inside of,
`editor-offset-timer.png` for a timer armed by the one above it,
`editor-errors.png` for an item that will not compile - the same glyph the
Errors view carries in the sidebar - and `editor-new-folder.png` /
`editor-new-item.png` for something the editor has made and nobody has saved.
`packages-package.svg` for a package's top folder - the same glyph the package
manager draws a package with, so the row that stands for the whole package is
told apart from the folders inside it. `EditorTreeDelegate` resolves which from
the item the row's id names and draws it; a row's own `QIcon` is never consulted,
so no call site sets one. The test for a package's top folder is the one the Lua
ancestors API makes - a folder whose `mPackageName` is its own name, and not a
module - so a module's master folder keeps the folder mark.

The dot itself is `uiDesign::treeRowDotGlyph()` rather than the delegate's own
drawing.

It has three forms, `uiDesign::TreeDotForm`, and each reading differs from the
others in shape as well as in ink - so the three are still three for a reader
who cannot separate the inks. `Filled` in `stateColor(scmStateHue_ok, darkPage)`
is running. `Ring`, a `scmTreeHollowPenWidth` hairline in `mutedText`, is
switched off. `RingWithCentre` is that same ring with a
`scmTreeDotCentreDiameter` point in the middle of it, in that same `mutedText`:
switched on, but inside a group that is not, so the user's switch says yes and
nothing is running. The emergency stop reads the same way: every trigger, alias,
timer and key the user left on shows the centred ring until the stop ends, and
the ones left off keep the plain ring. A screen reader hears it too:
`spokenRunState()` adds "held by the emergency stop" to the description of a row
that would otherwise run, and `Host::signal_emergencyStopChanged` has the trees
rebuilt so the words follow the stop as the dots do. The ok hue is a blue rather than a green
(`scmStateHue_ok` 0.58) for the same reason: it is read beside the error red all
over the editor and the package manager, and a red-green colour-blind reader
cannot tell a green from that red.

A row whose item will not compile keeps the form its state asks for and takes
`errorInk()` in place of either ink, so its dot and its errors mark say the one
thing. The delegate caches the six pictures - three forms in each of the two
inks - and the decoration key already carries `RowMark`, so a row that becomes
broken composes afresh.

Every mark is drawn at 16px, the size `SearchResultDelegate` gives the glyphs in
the results list, and a row leaves that much room whether it carries a mark or
not - so a tree's heading, a folder and a plain item are one height and the
hover fill on one row is the same shape as the selection pill on the next.
The ink is `mutedText`, or the colour the trees' stylesheet writes a chosen row's
name in (`accentText`) while the row is selected, so a mark and the name beside
it are always the one colour.

The error mark is the one exception, and it is inked in `uiDesign::errorInk()`
whether the row is chosen or not. A folder, a filter chain and an offset timer
are labels for what the item *is*, and chrome's grey is the right tone for a
label; a broken item is a state being reported, and reported in grey it could
not be told from a folder at a glance. `errorInk()` is the same value the
compile note over the code pane is written in - one function, so the two cannot
drift - and the accent wash a chosen row is filled with is light enough for it
to hold there too. `EditorTreeErrorMarkTest` reads the mark off a grab of the
tree in both appearances and holds that red to `scmQuietMinimumRatio` against
the surface the rows sit on.

The seventh tree reads the same two slots differently, on the same row.
`VariableTreeDelegate` draws a kept square where the dot stands: filled in the
blue a running dot is filled in when the variable is saved with the profile,
hollow when it is not, half when only some of a table is, and absent for a row
the profile cannot keep - a function, a reference, a table past the size limit -
which is written in `disabledText` with the reason as its tooltip. Beside it
stands a mark for the value's type: `editor-variables.png` for a table (a table
is braces everywhere, the sidebar included), `editor-type-string.png` (quote),
`editor-type-number.png` (hash), `editor-type-boolean.png` (toggle),
`editor-type-function.png` and `editor-type-other.png` (box). After the name -
an index key is drawn as `[n]` - comes `editor-hidden.png` on a row that is
hidden while hidden variables are shown, and at the trailing edge as much of the
value as the panel leaves room for, in `chipFont()`: the string in quotes, the
number, `true` or `false`, `{ n keys }` or `{ n items }`. Everything a row is
drawn from is written into the row's data roles when it is built or written back
(`setVariableRowData()`, `refreshVariableRow()`), so painting never reaches into
Lua or counts a table. A click on the square asks the editor for the toggle
(`slot_toggleVariableKept()`), which is where the rule about which members of a
table may be kept lives. The switch under the tree says how many globals it
holds back (`editorHiddenVariablesCount`).

## 3. Naming

### Object names

The object names are the dialog's test interface as well as its stylesheet
handles; keep them stable.

| Name | What it is |
| --- | --- |
| `settingsShell` | The runtime-built root widget |
| `settingsSidebar` | The category column |
| `settingsCategoryList` | The `QListWidget` of categories |
| `settingsSidebarSeparator` | Divider row inside the category list |
| `settingsSidebarToggle` | The chevron on the seam, which gives the names up and brings them back |
| `settingsWordmark` | "Settings" title at the top of the sidebar |
| `settingsContent` | The right-hand pane |
| `settingsPageTitle`, `settingsPageTitleIcon` | Title row over a page |
| `settingsStack` | The `QStackedWidget` of pages |
| `settingsPage_<key>` | One category or subpage `QScrollArea` |
| `settingsColumn_<key>` | The card column inside that scroll area |
| `settingsSearchField`, `settingsSearchBack`, `settingsSearchEmpty`, `settingsSearchHeader` | Search chrome |
| `settingsSubpageBack` | Chevron back out of a subpage |
| `settingsCardDescription` | Description label under a card title |
| `settingsHeroHeadline`, `settingsHeroDetail`, `settingsHeroLink` | Hero card parts |
| `settingsCheckBoxWrap`, `settingsWrappedLabel` | A check box whose text wraps |
| `settingsSearchDebounce` | The search timer - tests wait on it by name |

### Dynamic properties

Stylesheets select on these; setting one after the widget is shown needs an
`unpolish()`/`polish()` pair to take effect.

| Property | Meaning |
| --- | --- |
| `settingsSurface` | Shell scaffolding a profile's Lua stylesheet must not colour |
| `settingsCard` | This group box is a card |
| `settingsCardPlain` | A card without the top padding its title would need |
| `settingsHero` | The prominent lead card of a page |
| `settingsChevronRow` | A button that reads as a navigation row into a subpage |
| `settingsRail` | The sidebar is collapsed to icons only |
| `settingsFocused` | The category list has keyboard focus |
| `searchMatch` | Marker-pen highlight on a search hit |
| `searchKeywords` | Comma-separated synonyms fed into the search index |
| `settingsRichText` | The text a label was given, before the colour of any link in it was written in - see `withLinkColour()` |

...and the About dialog's own, with an `about*` prefix:

| Property | Meaning |
| --- | --- |
| `aboutCard` | This group box is a card |
| `aboutCardPlain` | A card with no title, and so no room reserved for one |
| `aboutChip` | A word in a box: the channel, the Qt version, a contact handle, a licence kind |
| `aboutChipLit` | ...and that chip filled with the accent, which the build channel is |
| `aboutButton` | An ordinary push button on this dialog |
| `aboutCopied` | ...one of them for the moment after it has copied |
| `aboutPrimaryButton` | The one button that is an invitation rather than a control |
| `aboutNavGlyph` | Which file a navigation button's glyph is re-inked from |
| `aboutRestingText` | What a Copy button reads when it is not saying "Copied" |
| `aboutRichText` | The text a label was given, before its links were inked |

...and the connection dialog's one, with a `connection*` prefix. The two tabs
of that window and every button on it have been through the design;
`applyConnectionShellStyle()` builds two sheets, one set on each tab page rather
than on the tab widget - which would take the tab bar with it - and one set on
each of the two containers that hold buttons, `profileAdminArea` (Remove, Copy,
New) and `widget_bottom` (the skip button and the button box):

| Property | Meaning |
| --- | --- |
| `connectionFieldState` | What the validator has to say about this field: `error`, and absent while the field is fine |
| `uiMenuButton` | Shared, not this window's: on Copy, which stays a `QToolButton` so its trailing half can open a menu - see "Buttons" |

The two buttons the button box builds at runtime are named so that a test can
reach them: `connectButton` and `offlineButton`. Connect keeps
`setDefault(true)`, and the one rule this window writes of its own -
`QPushButton:default:enabled` - is what fills it with the accent wash the About
dialog's primary button carries. The `:enabled` half of that selector matters:
without it a Connect the validator has switched off would sit lit rather than
falling back to the shared disabled look.

A field holding a value that cannot be changed carries no property of its own.
It is drawn by the `:read-only` pseudo-state, which Qt evaluates at paint time,
so `setReadOnly()` needs no re-polish after it - and it takes the surface and
the hairline of an unavailable field
(`uiDesign::disabledFieldColour()` / `disabledBorderColour()`) but not its ink,
since the value in it is real and is there to be read.

...and the notepad's, with a `notepad*` prefix. That window is a `QMainWindow`
per profile: a strip of tabs of notes over a bottom toolbar, with a find bar
between them. `applyNotepadShellStyle()` builds two sheets, one set on
`centralwidget` - the page, the tab recipe, the notes as fields and the find bar -
and one on the bar, whose seam is on its **top**, since the bar is at the foot of
the window (`ToolBarSeam::Top`). Neither goes on the window, for the same reason
none of the other shells' do: a profile's Lua stylesheet is assigned to the
window on every show.

| Name | What it is |
| --- | --- |
| `notepadPrefixLabel`, `notepadPrefixField` | "Before each line" and the field after it, which every line sent is prefixed with |
| `notepadPrefixWarning` | The action carried inside that field at its trailing end, whose warning glyph is the cue at any width and whose tooltip is the whole sentence |
| `notepadStripSpacer` | The expanding widget that holds the warning at the trailing end of the strip |
| `notepadNoPrefixNote`, `notepadNoPrefixDot`, `notepadNoPrefixText` | The warning reading: a dot in the warning hue and the words beside it, on show only while it applies |
| `notepadSendingDot`, `notepadSendingText`, `notepadSendingBar` | The sending reading: an accent dot, "Sending 4 of 12", and the thread of progress under the send |
| `notepadAddTab` | The corner button that starts a new note |
| `notepadFindBar` | The row between the notes and the bar, on the page with a hairline over it |
| `notepadFindField` | The field searched in |
| `notepadFindPrevious`, `notepadFindNext`, `notepadFindClose` | The three picture-only buttons beside it: chevron up, chevron down, x |

...and one dynamic property, `notepadFieldState`, which the prefix field carries
the value `"warning"` on while the reading below applies. It is drawn by a rule
written after `inputStyleSheet()` so it wins the specificity tie against that
recipe's `:focus` frame, the way the connection dialog's `"error"` rule does, and
it needs `uiDesign::repolish()` after every change.

The toolbar keeps the `.ui` file's own `toolBar` name, because `saveState()` and
`restoreState()` find a bar by it and the window's layout is kept under
`Notepad/WindowState`.

**The send strip** is that bar, and it has three readings. Idle it says how far
each send would reach: **Send line**, **Send selection (3 lines)** and **Send all
(12 lines)**, each with a Lucide glyph beside its word and the two counts live -
counted the way `slot_sendNextLine()` sends, which skips an empty line, so the
number is what the game will actually see. Then the seam, "Before each line" and
the prefix field. Sending, all of that leaves the strip (`QAction::setVisible()`,
so nothing is built or torn down) and in its place stand an accent dot, "Sending
4 of 12", the thread of progress and **Stop** - which is not on the strip at all
while there is nothing to stop, rather than sitting there unavailable. Both ends
of a send go through `mudlet::self()->announce()`. And with the prefix empty over
a note of more than `scmNotepadWarnAtLines` lines, the field takes the warning
hairline and a cue inside it, and the strip's trailing end reads, where there is
room for it, "No prefix, so 12 lines go to the game
as commands" - a dot in `stateColor(scmStateHue_warning, darkPage)` and words
walked off the page with `readableOn()`, the same shape as the editor's compile
note. The buttons stay live: it says what sending would come to rather than
refusing it.

The seam between Send all and "Before each line" parts two words, and they stand
the same distance from it. A tool button holds its word its padding in from its
edge and then some - `QToolButton::sizeHint()` asks for two spaces more than the
word measures, the stylesheet style adds three pixels, and `QCommonStyle` draws
the word left-aligned after the picture, so all of that slack lands after the
word on the seam's side - while a label holds its word at its edge. The lead
word is therefore held in by what the button's own geometry says it holds its
word in by (`alignPrefixLeadWord()`), taken again whenever the button's words
change, and the guard reads the ink either side of the line off a grab rather
than a margin.

**The warning has three levels**, because the strip's own content wants about
950px and the window opens at 800: a sentence at the trailing end of that bar is
the first thing to go, at exactly the width where a reader most needs it. So the
warning is not one thing that shrinks.

1. **The cue lives in the field.** While the warning applies the prefix field
   carries an action at its trailing end (`notepadPrefixWarning`, added the way
   the connection dialog's reveal action is added to the password field), whose
   picture is `editor-notice-warning.svg` through `tintedGlyph()` in the warning
   hue at the size the editor's banner draws its own, re-inked in the style pass.
   It costs the bar no room, so it is the cue at any width; the field's warning
   hairline stays, and the whole sentence is on the action's tooltip and on the
   field's - which is what a reader is looking at while typing the answer to it.
2. **The words come only whole.** `fitNoPrefixNote()` picks one of three: the
   sentence, the short form "No prefix" with the dot, or the reading off the bar
   altogether. Never an elided fragment - a sentence cut mid-word says less than
   two words that are whole. The measurement is the bar's own `sizeHint()` taken
   with the words off the label, which counts the separators, the spacing and the
   padding a stylesheet hands out the way Qt counts them, against what each form
   asks for as its own `sizeHint()`. It is taken again on every resize of the bar
   and on an appearance change, and the layouts under it are invalidated first,
   since a layout answers with what it worked out last time until it is told
   otherwise.
3. **Nothing.** What goes when neither form fits is `action_noPrefixNote`, not
   the widget: a `QToolBar` shows a widget it has room for again on its next
   layout, whatever the widget was told, so hiding the widget leaves a dot with
   nothing after it. Whether the warning applies is `mNoPrefixWarningApplies`,
   which is what a wider window brings the words back from - the action's own
   visibility says only what the strip is carrying at this width.

On the offscreen platform the tests run on that comes to: the strip with no
words asks for 712px, the sentence is 298px and the short form 54px, so the
sentence wants a 1010px window and the short form 766px. Below about 600px the
bar is over its own room as well and posts its tail - the field with it - into
the drop-down `QToolBar` puts what it cannot fit in.

A fold used to hide the whole of that: an "Options" disclosure with the five send
controls behind it, remembered in `Notepad/SendControlsVisible`. It went because
it saved no space - the bar was there either way - and guarded nothing: a
one-click toggle is not a guard, and a reader who opened it once had it open
forever. What guards a send is the count on the button before it is pressed and
the warning at the one moment it is dangerous, which is what replaced it. The
key is no longer written; `Host` offers no way to remove one, so an old profile
keeps a line nothing reads.

A note is a `QPlainTextEdit` and is drawn as the field it is typed into, by
`inputStyleSheet()` scoped to `#centralwidget`. The profile's Lua stylesheet still
reaches the window as a whole, but no longer each note: a sheet set on the note
itself would beat the shell's, which names a container and so wins on
specificity. The display font stays the profile's. The find bar's two marker inks
are mixed in the same pass - `tokens.marker` on the match the cursor is on, and a
wash of it towards the field on the rest - so they follow an appearance change.

The status bar the `.ui` file carries is hidden: nothing ever writes to it, and an
empty one leaves a strip of the platform's own drawing under a bar this window
draws itself.

That pass is re-run from two places rather than one. `signal_appearanceChanged`
is what a reader picking an appearance in the settings sends; a reader on "follow
the system" who changes the system's theme instead moves the application's
palette without that signal, and the only word this window gets is the style
change Qt sends every widget - so `changeEvent()` runs the pass as well, the way
the editor's does.

The editor redesign follows the same scheme with an `editor*` prefix:
`editorShell`, `editorSidebar`, `editorPage_<key>`, `editorOptionsRow`, and so
on, and the About dialog with an `about*` one: `aboutShell`, `aboutArtColumn`,
`aboutNav`, `aboutNavButton_<key>`, `aboutStack`, `aboutPage_<key>` and
`aboutColumn_<key>`.

...and the package manager's, with a `packages*` prefix on what the shell adds and the `.ui`
file's own names kept on everything else, since every other package test presses those.
`applyPackageManagerShellStyle()` builds three sheets - one on `leftPanel`, one on `packageList`
and one on `rightPanel` - and none on the dialog, for the same reason none of the other shells'
go on their window.

| Name | What it is |
| --- | --- |
| `leftPanel` | The list column, on the pane tone. No hairline down its trailing edge: the splitter's handle is the seam, and each column carries `scmProp_paneTone` so the handle draws its neighbour's own tone up to it |
| `rightPanel` | The details column, on the page tone, never dragged narrower than 320px |
| `packagesSplitter` | The `uiDesign::GripSplitter` the two columns hang in, in place of the row the `.ui` file put them in. Neither pane collapsible, the details column taking the growth (`setStretchFactor(1, 1)`), the list column keeping the `.ui` file's 300px floor and losing its 400px cap - a drag that stops dead reads as a broken handle. The sizes are saved in `closeEvent()` under `packageManagerSplitterState` in `MudletApp::getQSettings()` and put back in `showEvent()` on the first show only, never in the constructor: on macOS a native window created before it is shown drops the geometry it is handed. With nothing saved the list column takes a third of the window |
| `packagesHeadlineRow` | The row at the head of the details column holding `label_packageName` and `label_version`, then a stretch |
| `packagesViewBar` | The `QTabBar` the three views are chosen from, held to the leading edge; `headerBar` keeps the three buttons it replaced, hidden, since the group, the slots and the other package tests all press those |
| `lineEdit_searchBar` | The field the list is narrowed with, carrying `settings-search.svg` as a leading action |
| `packageList` | The rows, drawn by `PackageItemDelegate` on `itemRowStyleSheet()` |
| `PackageItemDelegate::cPackageVersionRole` (`Qt::UserRole + 1`) | Which version the row stands for, drawn right-aligned on the name's own line, flush with the row's trailing padding, at `typeSize(Caption)` and in the ok tone the head of the details column writes the version in - walked twice in `restyle()`, against the pane a plain row stands on and against `blend(pane, accent, scmAccentWashStrength)` for a chosen one, or the tone is lost on the accent's wash. Set wherever a row is filled - `dlgPackageManager::setRowVersion()` from `resetPackageList()`, `slot_setPackageList()` and `slot_searchTextChanged()` - out of `mPackageInfo` in the Installed view, out of the repository object in Explore, and as the "%1 → %2" pair in Updates, the same text the details column shows there. A package that names no version leaves the role unset and nothing is drawn. The name's elision width shrinks by the version's advance and a gap, so the two never touch; `sizeHint()` is untouched, since the version rides an existing line. `Qt::UserRole` itself is the row's one-line summary |
| `packagesMenuRemove`, `packagesMenuInstall` | The entries of the list's right-click menu, built by `buildPackageContextMenu()` on `packagesContextMenu` - the design's menu and its corner taken there rather than in the style pass, since it is built at the moment it is needed. The Installed view gets Remove; Explore and Updates get the one Install entry, worded as the view words the button. The row under the pointer is made the current one first, so the details column is showing what the menu is about; a row already inside a selection keeps that selection, since Remove and Install act on all of it |
| `packagesNotice` | The `dlgSystemMessageArea` under the list, which `showImportStatus()` brings out in its warning reading; `label_importStatus` is left hidden and unused |
| `pushButton_installFile` | Install from a file, the full width of the column under the list |
| `pushButton_installRepo`, `pushButton_remove` | Install or Update, and Remove - moved into the details column, and hidden rather than disabled in the view they do not apply to. One chosen row is the bare word; more than one says how many as a sentence ("Remove %n packages"), since "Remove 1" is a count of something nobody asked to count |
| `pushButton_website`, `pushButton_report` | Website and Report an issue, at the trailing end of that row |
| `label_icon`, `label_packageName`, `label_title`, `label_author`, `label_version` | The head of the details column: the picture cut to `scmRadiusInput` at 48px (or the package glyph on a card-tone box), the name at `TypeStep::Title` with the version beside it on `packagesHeadlineRow`, the one-line summary, and the author alone on the caption line. The version is the bare number - "1.3.3", and "1.2 → 1.4" in the Updates view - at the window's body font, written in `readableOn(page, stateColor(scmStateHue_ok, darkPage), text, scmTextMinimumRatio)`: the same blue a running item's dot in the editor's trees is filled in, walked until it reads on the page. Its words are held to the foot of a box the row makes as deep as the name's, and lifted by the difference between the two fonts' descents, which puts the number on the name's own baseline. The name is elided to what the row leaves it beside the version, taken again on every resize and on every drag of the seam |
| `packageDescription` | The notes, as a field, with a document stylesheet mixed from the tokens |

...and the package exporter's, with an `exporter*` prefix on what the shell adds and the
`.ui` file's own names kept on everything else, since the other two exporter tests press
those. `applyPackageExporterShellStyle()` builds four sheets - one on the name row, one on
each of the two columns and one on the foot - and none on the dialog, for the same reason
none of the other shells' go on their window. The `.ui` file's vertical fold is gone: the
disclosure button and the two splitters it opened are hidden with their contents pulled out,
so the package's details are always on show rather than behind a button most readers never
pressed.

| Name | What it is |
| --- | --- |
| `widget_newOrExistingPackageName` | The row across the top of the window, holding two controls and nothing else: `exporterStartOverButton` and then `packageList`, whose first entry starts the form off from a package that is already installed. The button stands at the leading edge and the picker at the trailing one, with a stretch between them, both over the column of fields they act on. `lineEdit_packageName` is no longer here - it is the first field of `exporterDetailsColumn`, so every field the reader fills in is read in one place - and `label_or` is hidden, since two controls side by side already say it. The row's sheet still carries `inputStyleSheet()` for the picker and `buttonStyleSheet()` for the button, and no rule of its own for the name |
| `exporterStartOverButton` | The way back to the form as the window opened it: every field, every tick, the picture and the files, but not the folder the package is written to - that is a preference the reader set once. Reads "Clear all fields and selections", with no glyph: the words say the whole of it. Always on show and disabled while `formIsFresh()`, which `refreshStartOverButton()` settles the moment anything differs - a control that stays put says what the window can do, and nothing on the row moves on the first keystroke. On show while a module is being made too, where the picker beside it is not. `packageList`'s first entry does the same act through the same `requestStartOver()`, which puts a `QMessageBox` up first whenever `formHasTypedWords()` - words are what nobody has another copy of, while a tick, a chip, a picture and a file are one gesture to make again. Declining from the picker puts it back on the entry whose words are still on the form |
| `exporterSplitter` | The `uiDesign::GripSplitter` the two columns hang in. Neither pane collapsible, the form taking the growth (`setStretchFactor(1, 1)`), the sizes saved in `closeEvent()` under `packageExporterSplitterState` in `MudletApp::getQSettings()` and put back on the first `showEvent()` only - never in the constructor, since on macOS a native window made before it is shown drops the geometry it is handed. With nothing saved the tree takes a third of the window |
| `exporterSelectionColumn` | The tree's column, on the pane tone at `scmTabPaneInset` |
| `exporterSelectionHeading` | The words the hidden `groupBox_exportSelection` was titled with, as a heading in `mutedText`. That box is still titled, count and all: `PackageExporterDialogTest` reads the count off it |
| `exporterSelectionCount` | ...and the count as a word in a box beside it, at `chipFont()` and `scmRadiusChip`: `accentText` on `accentSoft` while something is ticked, and `mutedText` on a `border` hairline with no fill while nothing is, which the `exporterCountEmpty` property switches between |
| `treeWidget_exportSelection` | The rows through `itemRowStyleSheet()` on the pane, the tick on each through `itemCheckMarkStyleSheet()`, the chevron that folds a group away through `treeBranchStyleSheet()`, and the accent bar on the chosen row from `ExporterTreeDelegate`. The six top-level rows carry the editor's own unit glyphs and every folder under them `editor-folder.svg`, re-inked by walking the tree in the style pass - under a `QSignalBlocker`, since `QTreeWidget::itemChanged` fires for a picture as well as for a tick and the slot it is wired to hands a row's tick down to its children. A folder's own box says what is ticked inside it: the dash (`:indeterminate`) while anything under it at any depth is ticked, a full tick only when the reader ticked the folder itself. A folder at the root of a unit carrying the package's own name - the name typed on the form, or the package chosen in the picker - stands for its contents instead: its box reads all, some or none of them, clicking it ticks or clears everything inside it, and it is never written into the file or counted, since `XMLimport` makes that folder again on install |
| `exporterSelectionHint` | The caption under the tree saying that a tick on a folder takes everything in it, that a right-click takes the folder alone, and that a folder named after the package is made again on install so only what is inside it goes in |
| `exporterDetailsColumn` | The form, on the page tone: every word over its own field rather than in a column of labels beside them, since the grid the `.ui` file used held every field to the width of the longest word. `lineEdit_packageName` is its first row, under the `exporterNameLabel` caption - "Package name", or "Module name" from `showModeWords()` - at the ordinary `inputStyleSheet()` height the fields under it stand at, and named as that field's buddy |
| `exporterDetailsHeading`, `exporterDetailsHint` | "About the package" and, beside it, that none of it is required |
| `Icon` | The package's picture as a 48px well on the card tone with a `border` hairline and `scmRadiusInput`, showing `packages-package.svg` in `mutedText` until one is chosen. Always on show: the `.ui` file hid it until there was a picture, which left the button beside it with nothing to say what it would do |
| `pushButton_removeIcon` | ...and the cross that takes that picture off again, `editor-clear.svg` at the field's own height, with its words on its accessible name and its tooltip. On show only while there is a picture to take off |
| `exporterDependencies` | The packages this one needs, as a `uiDesign::ChipRow` offering the installed ones - and the modules as well while a module is being made. It is a view over `comboBox_dependencies`, which is what the export writes into the package and stays the one record of what it needs; that box, `DependencyList` and the two buttons beside them are hidden but alive and connected, since `PackageExporterDialogTest` and `PackageExporterTest` drive them by name |
| `exporterDescriptionHint`, `exporterAssetsHint`, `exporterAssetsDropHint` | The captions beside Description, Files and Add files |
| `listWidget_addedFiles` | The files shipped with the package, drawn as a zone they are dropped on: the field tone, a 1px **dashed** hairline and `scmRadiusInput`. The one rule on this form that is not the input recipe's, and written in the column's own sheet for that reason - a dashed edge is the whole of what says a box takes a drop, and the recipe draws one solid hairline for every field. Its height is `fitAssetsList()`: every file plus one empty line, which is where the next drop lands and what says the list would take another, and the room for it comes off `textEdit_description` above - the two of them are the only rows of this column that flex, so what they hold between them is all there is to share. The description keeps four lines of its own font whatever the list wants; once it is down to those the list stops growing and scrolls instead. The zone is never shorter than two lines, so an empty one still reads as somewhere to drop something. Measured off the rows and the edges rather than written down: a row's height is `sizeHintForRow(0)`, and the stylesheet's dashed edge is read as the difference between the control and its viewport, since `frameWidth()` does not answer for a styled border. `widget_assets` is `QSizePolicy::Fixed` vertically for the same rule - the `.ui` file let it grow, which took a share of the column's spare room and let the zone grow without the description giving anything up |
| `exporterAssetsEmpty` | ...and the prompt over that zone while it is empty, laid on the list's own viewport and taken away the moment a file arrives, in `readableOn(field, disabledText, text, scmTextMinimumRatio)` |
| `exporterFoot` | The row across the foot, on the page with a 1px `separator` on its top edge |
| `exporterSaveCaption`, `exporterSavePath` | "Saves to" and the path the package is written to, at `fixedPitchFont()` a shade smaller and elided from its leading end to the room the row leaves it, with the whole of it on the tooltip. `lineEdit_filePath` keeps its placeholder and is hidden |
| `exporterNotice` | The `dlgSystemMessageArea` under that row, which `displayResultMessage()` brings out in its error reading for a refusal and its information one otherwise. `infoLabel` is hidden and still written to, as plain html: both other exporter tests read a result off it, and which of the two readings it is, is said by the notice's own picture rather than by a colour written into the text |
| `buttonBox` | Moved into the foot. `mExportButton` carries `uiDesign::scmProp_primaryButton` and `editor-export.svg`; Cancel and Close keep the visibility logic they had |

...and one dynamic property of its own, `exporterCountEmpty`, which the count chip carries
while nothing is ticked and which needs `uiDesign::repolish()` after every change.

Three of the `settings*` names are not the settings dialog's alone.
`settingsRail` and `settingsFocused` are the contract `SidebarItemDelegate` and
`sidebarStyleSheet()` are written against, and `settingsSurface` - which
`markAsShellSurface()` puts on - is what every shell's stylesheet keeps its own
scaffolding transparent by. All three keep these names in every window.

### Anchors are inked in the text, not in a palette

A `QLabel` parses its rich text the moment it is set and bakes the colour of
every anchor into the document then and there, taking it from the *application*
palette. Writing `QPalette::Link` to the widget afterwards does nothing, and
re-polishing it does nothing either - the link stays at Qt's own blue, which is
2.4:1 on a dark page. `uiDesign::withLinkColour(richText, colour)` writes the
colour onto the anchor instead, which a document does honour; the text a label
was given is kept in a dynamic property (`aboutRichText`, `settingsRichText`) so
that an appearance change can ink it again. A `QTextDocument` ignores an
`a { color: ... }` rule in a `<style>` head for the same reason, so the licence
browser's own anchors go through the same helper.

The editor's forms add these, which its tests reach it by:

| Name | What it is |
| --- | --- |
| `widget_top` | The head row of a form: name, command, ID pill |
| `frameId` | The ID pill on every form |
| `editorScriptEvents` | The `ChipRow` of a script's events |
| `editorChip`, `editorChipLabel`, `editorChipRemove` | One chip, its name and its cross |
| `editorChipAdd`, `editorChipEditor`, `editorChipNote` | The dashed add button, the inline field, the "already listed" note |
| `editorTimerInterval` | The sentence row holding a timer's four fields |
| `editorKeyBindingRow`, `editorKeyHint`, `editorKeyClear` | The key binding field's row, the hint beside it, the cross that forgets the keystroke |
| `editorOptionsRow` | The trigger form's options strip |
| `editorMatchAny`, `editorMatchAll`, `editorMatchWithinLines` | The two segments a matching mode is chosen with, and the lines the All mode matches within |
| `editorSoundFile` | The read-only field that names a trigger's sound file and opens the chooser |
| `editorVariableTypes` | The row holding a variable's key and value pickers |
| `editorHiddenVariablesCount` | The count beside the "Show hidden variables" switch |
| `editorCodeHeaderTitle` | The word on the code pane's heading: "Lua script", or "Value" in the variables view |
| `editorNotice*` glyph files | The banner's three pictures |

| Property | Meaning |
| --- | --- |
| `editorRowLabel` | A word leading or joining a form row, written in the quiet ink |
| `editorIdChip` | The frame drawn as the ID pill |
| `editorPanelSurface` | A row widget that shows the form through, so a profile stylesheet cannot paint a band across it |
| `editorOptionRow` | The trigger form's options strip: a group box drawn as nothing, so that a screen reader has a grouping to announce |
| `editorSegment` | A radio button drawn as one segment of a joined two-part control rather than as a dot beside a word |
| `editorSegmentSide` | Which end of that pair it is, `first` or `last`, and so which corners it rounds |
| `editorChipSystem` | A chip holding an event Mudlet raises itself, read in the quiet ink |
| `editorListening` | The key binding field while it waits for a keystroke |

## 4. Search over a widget tree

`buildSearchIndex()` (`src/dlgProfilePreferences.cpp`) walks the real widget tree
rather than a hand-written list, so a control added to the `.ui` file later is
searchable without anyone saying so.

- **Index unit is the card.** Each entry records the card, its category, its
  subpage, its folded text, and its home layout plus index.
- **Normalization** is `foldForSearch()`: strips rich-text tags and `&`
  accelerators, NFKD-decomposes, drops non-spacing marks, simplifies whitespace,
  case-folds. So "fonte" finds "Fonté" and "save" finds "&Save".
- **Text sources** per widget (`collectSearchText()`): visible text, tooltip and
  the `searchKeywords` property. Combo box items are indexed; `QFontComboBox`
  items deliberately are not, or every card with a font picker matches "mono".
- **Synonyms** are set as `searchKeywords` with a `//:` translator note telling
  translators to write what a player would type rather than transliterate - see
  the `synonyms` list in `setSearchKeywords()`.
- **Highlighting** sets the `searchMatch` property and re-polishes
  (`setSearchMatch()`).
- **Results** are physically reparented into the results page and returned to
  their recorded home layout and index on the next keystroke
  (`returnSearchedCardsHome()`). Reparenting clears focus, so the category list's
  selection is cleared without clearing its current row - otherwise the focus-in
  that follows selects the first category and ends the search.
- Search input is debounced 150ms; the timer is named `settingsSearchDebounce`.

## 5. Instant apply and live sync

Available as a pattern; the editor keeps its explicit save for now.

- `connectApplyTriggers()` (`src/dlgProfilePreferences.cpp`) wires every control
  *by type* - `QAbstractButton::toggled`,
  `QCheckBox::checkStateChanged` (tri-state boxes),
  checkable `QGroupBox::toggled`, combo/spin/date-time value changes,
  `QLineEdit::editingFinished` - all with `Qt::UniqueConnection`, so it can be
  re-run after new controls appear.
- Applies run off a 400ms debounce timer.
- `SettingsSnapshot` tracks per-control values (`controlValue()` mirrors the same
  type list) and answers `dirty()`, `anyDirty()`, `shortcutsDirty()`,
  `pendingEdits()`, so an apply writes only what actually changed. A line edit
  being typed into is not a setting yet (`beingTypedInto()`).
- `guardScrollWheel()` plus the `eventFilter()` wheel branch stop a wheel passing
  over a spin box or combo box from silently changing a setting: unfocused
  controls ignore the wheel and do not take focus from it.
- An appearance change restyles the shell off what that shell was last styled
  for (`mShellStyledForDarkPage`, written by `applyShellStyle()` from the tokens
  it mixed the sheet from) rather than off a before/after reading of
  `MudletApp::darkMode()` around the dialog's own `mudlet::setAppearance()` call - so a
  change arriving through `mudlet::signal_appearanceChanged`, which is emitted
  after the mode has already moved, restyles a second profile's settings dialog
  too instead of leaving it in the previous theme.

## 6. Testing

Each shell gets a test helper header plus focused test files, following
`test/functional_tests/SettingsTestHelper.h` and the `Settings*Test.cpp` set
(`SettingsSearchTest`, `SettingsShellNavigationTest`, `SettingsInstantApplyTest`,
`SettingsDirtyApplyTest`, `SettingsLiveSyncTest`, ...).

- The helper is free inline functions, not a `QObject`: a header is not listed in
  `test/functional_tests/CMakeLists.txt`, so nothing runs moc over it.
- Nothing in such a helper may use `QVERIFY`/`QFAIL` - both expand to a bare
  `return`.
- Tests reach into the dialog by the object names above.
- Waits are on named debounce timers and `QSignalSpy`, never `QTest::qSleep()`.
  An apply may already have run inside the interaction that scheduled it, so
  check before waiting.
- Prefer joining a grouped per-subsystem test binary over adding a standalone
  one - see `*_GROUP_TEST_SOURCES` in `test/functional_tests/CMakeLists.txt`.

### Verifying a stylesheet extraction

Moving rules into a shared builder is the one refactor here that no test
catches when it goes wrong. The sheets are built by `.arg()` chains, so
pulling a block out renumbers every placeholder after it, and a sheet that
is one substitution out of step is still a valid sheet - it just paints the
border colour where the background belongs. Nothing asserts on a stylesheet
string, so the suite stays green.

So verify by comparison, not by reading: dump every generated sheet from
every consumer before the change, make the change, dump again, and diff.
Do it in both themes - a wrong substitution can land on two colours that
happen to match on dark and diverge on light.

The harness that does it is not kept in the tree. It was a functional test
that opened the settings dialog and the editor and wrote every generated
stylesheet to the file named by `MUDLET_STYLESHEET_DUMP`, so two trees could
be compared byte for byte. It asserted nothing and always passed, which is
why it is not carried in `test/functional_tests/CMakeLists.txt`: wiring it
into the suite would buy a profile boot per run and prove nothing. Rewrite it from
that description, add it to the group sources by hand for the comparison, take the two dumps,
and drop it again. What ships is the diff being empty, not the dumps.

`toolBarStyleSheet()` and `disclosureButtonStyleSheet()` were extracted this
way: the editor's toolbar sheet came out byte-identical in both appearances,
and the trigger form's differed in one declaration - the disclosure button's
`background-color`, which was the field tone only because the lift
`buttonStyleSheet()` mixes a face from is file-local to `uiDesign.cpp`, and is
now the button face the shared builder reads there.

### Guards

A handful of tests hold the line the rest of this document describes. Text that cannot
be read in one of the two appearances, or that is written in a tone the design
does not use there, now fails a run rather than waiting for somebody to switch
theme and notice.

**`test/DesignColourLiteralTest.cpp`** reads the sources of every surface that
has adopted the design language and fails on a colour written out rather than
taken from `themeTokens()`. It links nothing - the `src/` path arrives through
`MUDLET_SRC_DIR`, the way `CMakeListsConsistencyTest` takes it - so it costs a
compile and a tenth of a second.

Per line, in C++ it flags a hex colour inside a string literal (a raw string
literal included), an `rgb(` or `rgba(` with its channels written out in one,
`QColor(Qt::name)`, a `Qt::` colour name reaching
`QColor`/`QBrush`/`QPen`/`setColor`/`setForeground`/`setBackground`,
`QColor("...")`, `QColor::fromRgb(...)` or `QColor(r, g, b)` with written
numbers, `QColorConstants::` anything but `Transparent`, a CSS colour
keyword inside a string that also says `color:` or `background`, and a literal
that is a CSS colour keyword and nothing else - which is how a colour reaches a
sheet through `.arg()`, with the declaration it lands in written somewhere
else. In a `.ui` file it flags a `<color>` element and a `styleSheet` property
containing any of those.

Five things are deliberately not flagged. `QColor::fromHslF` and `fromHsvF`
build the semantic state hues, whose lightness already comes off the page.
`Qt::transparent` is never a theme colour. `Qt::white` and `Qt::black` are the
ends of the lightness axis rather than colours, so they pass inside a `blend()`
call or a `fill()` - by context, not globally, so `Qt::white` as an ink is still
caught. A format template such as `"rgba(%1, %2, %3, %4)"`, which is what
`uiDesign::rgba()` fills in, has no digit after the parenthesis and so is not a
written colour. And a colour keyword that is the whole of a *translated*
literal - `tr("Black")` naming one of the console's ANSI colours - is a word a
person reads rather than a value on its way into a sheet; a declaration written
inside a `tr()` string is still caught.

**`// theme-fixed: <why>`** exempts a line, and the reason travels with the
code rather than living in a list inside the test - in a `.ui` file inside an
XML comment, and everywhere else as a `//` one. On a line of its own the
marker covers the run of lines under it, up to the next blank one, which is how
a table - a console's ANSI defaults, a map's palette - is marked without
repeating the reason on every row; and a colour inside a raw string literal,
which can carry no comment of its own, is marked where the literal opens. It is
legitimate for a colour that is a *value being shown* rather than chrome: a well
filled with the colour the user picked, a console or map palette the profile
owns, another application's brand in a picture of its window. It is not a way
to keep a colour somebody has not got round to mixing from the tokens.

**`test/functional_tests/ReadabilityAuditTest.cpp`** opens the editor - on a
trigger with three pattern rows, one of each shape, a sound file set and the
highlight switched on, so that the options strip is walked with something beside
its switches to read - and the settings dialog, moves the appearance to dark
and then to light, and
for every visible thing that shows words compares the ink its palette answers
with against the colour most of the pixels behind it are. A stylesheet's
`color:` rule reaches the widget through `QStyleSheetStyle::polish()`, which is
why the palette is what is read; a case of its own proves that on a styled label
rather than assuming it. Floors are `scmTextMinimumRatio` (4.5:1) for text and
`scmQuietMinimumRatio` (3:1) for what is unavailable or not yet typed, both in
`src/uiDesign.h`. It reports how many things it read per window and per
appearance and fails if that count drops below a pinned floor, so a walk that
silently stopped finding widgets cannot pass as a clean one.

It skips three things, each for a reason rather than for convenience: the edbee
code pane, which carries a syntax theme with a background of its own; a colour
well, whose fill is a value and whose words are chosen against that fill by
`generateButtonStyleSheet()` - skipped by object name, listed in `wellNames()`;
and a *disabled* push button, the one control neither window draws the surface
of, whose bevel switched off is a grey the platform picked and which WCAG
exempts as an inactive component. The ink on that button is still the design's.

It also walks each of the editor's five field-only forms from the form widget
itself, with an item on show, so the words a form adds - the sentence round a
timer's fields, the hint beside a key's binding, a script's chips, a variable's
pickers - are read against their own floors rather than lost in the count for
the whole window. The connection dialog is walked the same way and for the same
reason: its two tabs are the whole of that window the design has reached, and
only one of them is on show at a time, so each is made current and read against
a floor of its own.

The tones the audit holds are walked rather than picked: `mutedText`,
`disabledText` and `accentText` each start at a weight and are moved until they
clear their floor on every surface they can be drawn on - the three depths for
the first two, and for `accentText` a wash of the accent at
`scmAccentWashStrength`, which is what a chosen row, a sidebar pill and a lit
chip all are.

**`test/functional_tests/EditorChromeInkTest.cpp`** holds the editor to one ink
for its chrome - see "One ink for the editor's chrome" above for what it walks,
what it leaves out and why, and the painted probe it ends with. The audit asks
whether a word can be read; this asks whether it is the right grey, which a
readable-but-wrong tone would otherwise pass.

**`test/functional_tests/ConnectionDialogStyleTest.cpp`** holds the
connection dialog's two tabs and its buttons to the recipes: both containers
holding a button carry a sheet with the push button rules in it, Copy carries
`uiMenuButton` and is named by a rule of its own, Connect is still the dialog's
default button and the sheet says what a default button is filled with, and the
New button's top edge is read off a grab of the window and has to be the
design's hairline rather than a platform bevel. Both pages carry a sheet built on
the `field` surface, the three Connect-to fields are at the dialog's own font
rather than the 9pt the `.ui` pinned them at, the port field is wide enough for
five digits and what the recipe leaves either side of them, and a port or a
server address the validator objects to carries `connectionFieldState` while it
is wrong and loses it once it is right. A case of its own moves the appearance
and reads the page's sheet again, since a window that builds its sheet once and
never re-runs it looks correct in whichever appearance it was opened in.

**`test/functional_tests/EditorMenuStyleTest.cpp`** holds the editor's two menus
to the menu recipe: the sheets standing over the one Save Profile drops name a
`QMenu` and the card surface, that menu is see-through so its corner is cut away
on a grab that keeps its alpha, and the surface a couple of pixels inside the
hairline is the card's. The search options menu is read the same way and, with
one option switched on, the square its mark stands in is found by walking in
from the row's edge and has to hold the ink every other choice is marked in -
which is nothing at all while the option is off.

**`test/functional_tests/NotepadShellTest.cpp`** holds the notepad to the same
kind of thing. Both sheets are set on widgets rather than on the window, and the
one on the central widget names the tab rules and the field tone while the bar's
names the bar, the warning hairline and the thread a send's progress is drawn as.
The send strip is read in each of its three readings: idle, no widget is called
`notepadOptionsToggle` any more, the three send buttons carry a glyph and a
tooltip each, a twelve-line note with three lines selected reads "Send all (12
lines)" and "Send selection (3 lines)", Send selection is unavailable with
nothing selected and Stop is not on the strip at all; sending, the buttons and
the field leave it while "Sending 0 of 12" and the bar's value follow the send,
and stopping puts it back within a tick; and with the prefix empty over that same
note the warning is on show, names twelve lines, puts `notepadFieldState` and a
cue carrying the whole sentence on the field and leaves the sends live - while
three lines with no prefix say nothing and leave the field bare. A case walks the
window through three widths and reads which of the warning's forms the strip
took: the whole sentence at 1400, "No prefix" at 800, no words at all at 520 with
the action off the bar and the cue still on the field, and the sentence back when
the window is widened again - with no reading at any width holding an ellipsis.
A fourth case reads the inks: the sending dot's own sheet names `tokens.accent`,
and the warning's words clear 4.5:1 on the page in both appearances, read off the
label's palette, which is where a stylesheet's `color:` lands. The add-tab button
and the three the find bar is worked from carry a glyph each and no word, and the tab rules
point the close button at a picture. A case reads pixels: with two notes open,
the chosen tab has to be within a few levels of the accent wash on the page and
the other has to read as the page itself. And an appearance change has to rebuild
both sheets with the new field tone - nothing outside this window restyles it.

**The marks and the buttons** are measured off pixels rather than off palettes,
since neither carries a word: `SettingsShellNavigationTest` reads a page's check
box and radio button in both appearances - the outline against the card behind
it at `scmQuietMinimumRatio`, and the pixels inside the box on an empty, a
checked and a `Qt::PartiallyChecked` one - and a page's push button for a
hairline within a few levels of `border`. `EditorTriggerOptionsStripTest` reads
the strip's check box the same way, so the two windows are held to the same
mark. Both grab the *window* rather than the control: a control that paints no
background of its own is grabbed against its own palette, and the card or column
that actually shows through beside the mark is only in a grab that holds it too.

**Adding a surface to the design language means adding it to both**: its files
to `scannedFiles()` in the scan, and its window to the walk in the audit.
Neither list can be inferred, and a surface in neither is a surface nothing is
checking.

## 7. Proposing a change

A window or a form is proposed as one hand-written HTML page published with
the Artifact tool, not as a multi-artboard design canvas (the `design` skill).
The reader wants every variant on one page behind its own controls - a shell
switch, a light/dark toggle, a size picker - and working interaction wherever
the proposal is about interaction: a chip row that takes a name, a key field
that listens. Screens laid out side by side on a canvas ask the reader to
scroll around them and to hold the comparison in their head; a toggle puts the
two states on the same pixels.

- Both appearances are computed, not picked. The page carries a JavaScript
  transcription of `themeTokens()` (section 1) run against the app's real
  palettes - `DarkTheme.cpp` for dark, a Fusion light palette for light - so
  the tones on the page are the tones the app will derive. A colour chosen by
  eye on a mockup is a colour the tokens will not reproduce.
- Glyphs are the Lucide SVGs the app ships (section 2), drawn at the token
  stroke width, so what is approved is what `glyphPixmap()` will draw.
- One page per round. A later round is a new page rather than an edit of the
  old one, so each decision stays attached to what was looked at when it was
  taken.
- The page is a proposal, not a specification: once approved, the measurements
  come from the tokens and the recipes, and the tests read the window rather
  than the mockup.
