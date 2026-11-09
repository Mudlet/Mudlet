---
name: design-language
description: >-
  Read before adding, changing or restyling any window, dialog, panel, form, toolbar, button,
  field, icon or colour in Mudlet's C++ UI. Gives the shared recipes in src/uiDesign.h, the
  rules every styled surface follows, which windows have adopted the design and which have not,
  and the checklist for bringing a surface in. docs/design-language.md holds the reasoning.
license: GPL-2.0-or-later
---

## When to use

Any task that touches what a user sees in a Qt widget: a new dialog, a new row on a form, a
new button, an icon, a colour, a stylesheet, a `.ui` change, a palette tweak, "make X look like
Y". Read this first, then the sections of `docs/design-language.md` this file points at. The
script editor (`src/dlgTriggerEditor.cpp`) is the reference look; the settings dialog
(`src/dlgProfilePreferences.cpp`) is the worked example of shelling an existing `.ui` file.

## The rules that never bend

1. **No colour literal.** Every colour is mixed at runtime from `uiDesign::themeTokens()`
   with `blend()`, `rgba()`, `readableOn()` or `stateColor()`. `test/DesignColourLiteralTest.cpp`
   scans every adopted file and fails on a hex string, an `rgb(`/`rgba(` with its channels
   written out, `Qt::red`, `QColor(r, g, b)`, CSS colour words in a declaration and a literal
   that is a colour word on its own - `qsl("black")` fed to an `.arg()` counts. A colour that
   is a *value being shown* (a colour well, a console palette)
   is marked `// theme-fixed: <why>` on its line. Never use that marker to skip mixing chrome.
2. **Read `qApp`'s palette, not the widget's.** Assigning a stylesheet freezes a widget's
   palette. `themeTokens()` reads the application palette; `darkPage` is measured from the
   page colour, never from `MudletApp::darkMode()`.
3. **One recipe per control kind.** A field, a check mark, a button, a card, a sidebar, a
   scrollbar each have exactly one stylesheet builder in `src/uiDesign.h`. Call it. Never write
   a second rule for the same control in a window's own sheet, and never copy a builder's rules
   out into another file.
4. **Scope input and control rules, never set them on a window.** `inputStyleSheet()`,
   `choiceStyleSheet()` and `buttonStyleSheet()` take a `selectorPrefix`. The settings dialog
   passes `#settingsStack`, the editor sets them on each form widget. Unscoped on a window they
   reach search fields, tree inline editors and the code pane's find bar.
5. **Glyphs are Lucide SVGs, loaded only through `uiDesign::glyphPixmap()`** and tinted with
   `tintedGlyph()` / `tintedIcon()`. Stroke width is the token `scmGlyphStrokeWidth`. A
   stylesheet that needs a picture points at a PNG written into the glyph cache by a helper
   shaped like `themedArrowFile()` in `src/uiDesign.cpp` (1x file plus an `@2x` twin). Never
   `QPixmap(":/icons/foo.png")` for chrome; never a new raster icon. See `docs/design-language.md`
   section 2.
6. **Radius follows control size** (`scmRadiusChip` 4, `scmRadiusInput` 5, `scmRadiusPanel` 8,
   `scmRadiusProminentInput` 8). No rule writes a radius of its own.
7. **A sheet names a size only through `uiDesign::typeSize()`**: four whole-point steps,
   `Caption` / `Body` / `Title` / `Display`, never a percentage (Qt's parser ignores it, so the
   rule silently sets nothing) and never a written number. `DesignColourLiteralTest` fails on both.
8. **Chrome words are `mutedText`; typed values are `text`; chosen or hovered is `accentText`.**
   Every ink is walked to a measured contrast floor (`scmTextMinimumRatio` 4.5:1,
   `scmQuietMinimumRatio` 3:1) and `ReadabilityAuditTest` checks both appearances.
9. **Shell over `.ui`.** Do not rewrite a `.ui` file to restyle it. Detach widgets with
   `detachFromLayout()` / `removeFromLayoutTree()` and move them into a runtime-built shell, so
   object names, connections and translations survive. The exception is a form whose layout is
   itself wrong (the Buttons form was rewritten as one grid).
10. **Object names and dynamic properties are the test interface.** Prefix them per window
    (`settings*`, `editor*`, `about*`); three are shared by contract: `settingsRail`,
    `settingsFocused`, `settingsSurface`. Setting a property after show needs
    `uiDesign::repolish()`. Full tables: `docs/design-language.md` section 3.
11. **Human copy.** Controls say what happens in a player's words; every user-visible string
    is `tr()` with a `//:` translator note on the line above.
12. **Measured responsiveness.** Breakpoints come from font metrics, the style's own metrics
    and `sizeHint()`s, with a different collapse and restore threshold so nothing oscillates.
    What a style leaves round what it draws is the style's, not a constant: it changes with the
    appearance, so anything measured against it is taken again on `QEvent::StyleChange`.

## Tokens

`uiDesign::ThemeTokens` (`src/uiDesign.h`). Pick by what the widget *is*:

| Token | Use for |
| --- | --- |
| `page` | The window and its scaffolding: toolbar, status bar, sidebar pane, columns, scroll areas |
| `card` | A panel raised off the page; menus and popups |
| `pane` | A column told apart from the page beside it (a fifth of the card lift) |
| `field` | Anything typed into or picked in; a check mark's fill |
| `border` | The hairline round a card, field, button or mark |
| `separator` | The seam between two panes |
| `text` | Typed values and body copy |
| `mutedText` | Every word of chrome: labels, headings, status, chips |
| `disabledText` | An unavailable word |
| `accent`, `accentText`, `accentSoft`, `hoverSoft` | The chosen thing, its ink, its wash, the hover wash |
| `marker` | The search highlight pen |
| `darkPage` | Which of the two treatments a rule takes |

State colours (ok, warning, error) come from `stateColor(scmStateHue_*, darkPage)`.

## Recipes (all in `src/uiDesign.h`)

| Need | Call | Notes |
| --- | --- | --- |
| Line edit, text edit, combo box, spin box | `inputStyleSheet(tokens, prefix)` | Claims combos and spin boxes only once the chevron PNGs exist; accent frame on focus. Then `letPopupsTakeTheFieldsCorner(container)` on the container the sheet was set on, and again wherever a combo box is built after that pass, or a dropped-down list's corner is cut out over the square corner of the window it lives in |
| Check box, radio button, checkable card mark | `choiceStyleSheet(tokens, prefix)` | One mark: field fill, hairline, accent when set, a dash for `Qt::PartiallyChecked`; cards get it through `cardIndicatorStyleSheet()`. Then `keepClickFocusOffControls(container)` on the container the sheet was set on, or a click leaves the focus accent behind on every base style that answers `SH_Button_FocusPolicy` with `Qt::StrongFocus` |
| Push button, button with a menu | `buttonStyleSheet(tokens, prefix)` | Same height and radius as a field; Lucide chevron as the menu indicator. A tool button that has to keep a split - the body acts, the trailing half opens a menu - is drawn by the same rule once it carries `scmProp_menuButton` (`uiMenuButton`); opt-in, so a window's self-painting tool buttons are untouched. Colour wells keep their own per-widget sheet. Same follow-up call as the row above - one `keepClickFocusOffControls()` covers both recipes |
| The trailing half of a split button | `splitButtonMenuHalfStyleSheet(buttonSelector, cornerRadius, tokens)` | Two click areas have to read as two: the seam on the half's leading edge, its own wash a step ahead of the body's, and the Lucide chevron in place of the platform triangle. `buttonStyleSheet()` calls it for a `uiMenuButton`; a window drawing a split button of its own face - the editor toolbar's Save Profile - calls it with that button's selector and its own radius, and holds the words clear itself with `padding-right` of `scmInputDropDownWidth` |
| Flat toolbar | `toolBarStyleSheet(toolBarSelector, seam, tokens)` | The bar on the page's own surface, the hairline seam on the edge `ToolBarSeam` names (`Bottom` for a bar across the top of a window, `Top` for one at its foot), the `::separator`, the `::handle` grip and the buttons - flat and frameless at rest, `hoverSoft` under the pointer, `accentSoft` while held or switched on, the word in `accentText` throughout so it lights with the glyph beside it. Scoped to the bar, not the window, so a window holding two bars draws each. `scmToolBarGripExtent` and `scmToolBarButtonRadius` / `scmToolBarButtonPadding*` are the measurements a caller reads for anything of its own that has to line up. What one bar carries on top - a split button - follows the shared part in that window's sheet. Never `buttonStyleSheet()` here: it gives every button a face and a frame |
| Disclosure button (a strip or row that opens and closes) | `disclosureButtonStyleSheet(buttonSelector, tokens)` | A checkable button carrying a glyph and a word: a button's own face, the chrome tone and the border hairline at rest, `accentText` on the accent's wash and hairline while checked, so it says the thing it opened is on show. The selector names the one button, or every tool button on the form is claimed. Set the height from the caller - the padding is air round the contents, not the height |
| Menu, context menu | `menuStyleSheet(tokens, prefix)` + `letPopupsTakeTheFieldsCorner(container)` | The list a combo box drops, on the card tone: the field's hairline, `scmRadiusInput`, the chip's corner on a row, `accentSoft`/`accentText` under the pointer's row, and the one choice mark on a checkable one. Set it on whatever the menu hangs off - a button's menu is reached by the bar's sheet - and open the corner on that same container. A menu built at the moment it is needed gets both from the code that builds it, before `exec()` |
| Tab strip | `tabBarStyleSheet(tabWidgetSelector, tokens)` | A `QTabWidget`'s tabs as a row of chips on the page rather than the folder tabs a platform cuts: `mutedText` on nothing at rest, `hoverSoft`/`accentText` under the pointer, `accentSoft`/`accentText` while chosen, `scmRadiusChip` throughout and the accent on the border from the keyboard. The pane takes `scmTabPaneInset` of margin and no border, so a field filling it opens its corner onto the page; the strip starts at the same inset. A closable tab's cross is `editor-clear.svg` through `themedGlyphFile()`, left out where the cache could not be written. Scoped to the one tab widget, so a window's other tab bars keep what they had. What it does not draw: the scroll buttons a crowded strip shows, whose arrows are a sub-control of a tool button the bar makes itself. `prepareTabStrip(pTabBar)` is the caller's one job - the base off, or the macOS style fills the whole bar with a band of its own behind the tabs, and every cross kept to `scmTabCloseBoxSize` and inside the chip's padding, since Qt's close button sizes itself from `PM_TabCloseIndicatorWidth` in its constructor and no `::close-button` rule is asked (20 under Fusion, 14 under the macOS style), which stretches the mark inside the picture by the same ratio, and Qt places that box on the tab's raw rectangle rather than in the box the word is laid in, which puts it outside the chip altogether. A bare `QTabBar` that has to keep painting of its own - a font per tab, an indicator - cannot take this sheet at all, since a `::tab` rule hands the tab to Qt's stylesheet style: see `TTabBar`, which paints the same chip from these same constants |
| Notice / banner | `noticeStyleSheet(frameSelector, tokens)` + `applyNoticeGlyph(label, kind, tokens)` | A line or two of words a window has to say something with: the accent's wash, a 1px accent hairline, `scmRadiusPanel` and `mutedText` words at the window's own font, with one 20px tinted Lucide glyph (`NoticeKind::Information` / `Warning` / `Error`) beside them. The selector names the one frame - the editor's `frame_notificationArea`, the connection dialog's `notificationArea` - and the caller sets `scmNoticePaddingHorizontal`/`Vertical` and `scmNoticeSpacing` on that frame's layout. Both `.ui` files ship a 3px box round 64px full-colour bitmaps at 16pt; undo those in code (rule 9), including `autoFillBackground(false)` on the frame and its labels, or the palette's window colour is painted over the wash and through the rounded corner. The words also take a `color:` sheet of their own on the message label: the area is hidden while the window is styled and the polish that brings it out writes the application's ink into the label's palette. Never place one under or over a list a double-click acts on: notices differ in height and move its rows between the two clicks |
| Card with a title inside the frame | `cardStyleSheet(CardMetrics, tokens)` + `cardIndicatorStyleSheet()` + `measuredCardTitleHeight()` | Measure the title height with the indicator rules in force, or the first control paints over the title |
| Sidebar list with rail collapse | `sidebarStyleSheet(...)` + `setSidebarCollapsed(...)` + `sidebarRowWidth(...)` + `SidebarItemDelegate` + `SidebarToggle` | The rail width, the paddings, the row height and the glyph are the component's own `scmSidebar*` constants; `SidebarMetrics` carries only what a window has a reason to differ by - the expanded width, which each measures off its widest row, and the vertical padding. A row is never a constant: `sidebarRowWidth()` asks the *base* style what an item of that name in bold and that icon needs (`CT_ItemViewItem`, widget `nullptr`, so a stylesheet style defers to the style underneath) and adds the accent bar and `::item` padding the sheet writes - a style leaves its own margins round an item's text, four pixels either side on macOS light against two under the dark theme, and a row measured for one elides in the other. `SidebarToggle` is the chevron on the seam, a child of the shell holding both panes, kept under `<window>SidebarLabelsShown` |
| Scrollbars on a surface | `scrollBarStyleSheet(prefix, tokens, surface)` | A scroll area's bars answer only to a descendant selector |
| Primary button | `buttonStyleSheet(tokens, prefix)` + `scmProp_primaryButton` (`uiPrimaryButton`) | The one act a window exists for, and never two on one window: `accentSoft` behind `accentText` on a 1px accent hairline, `hoverSoft` over it under the pointer, and the ordinary unavailable look while it is switched off rather than an invitation that cannot be taken. Opt-in, so every other button on the form keeps the plain face. The package exporter's Export carries it; the About dialog's own `aboutPrimaryButton` predates it and is that window's rule |
| Check mark on a row of a view | `itemCheckMarkStyleSheet(viewSelector, tokens)` | The very mark `choiceStyleSheet()` draws, written for a view's `::indicator` instead - same size, same field fill, same hairline, same accent once it is set, same tick and same dash for a part-checked row. Apart only because a view's indicator is a sub-control of the view rather than of a control on a form; a ticked row and a ticked option must not come to look like two different acts. `viewSelector` is that view's own selector |
| The arrow that folds a group of a tree away | `treeBranchStyleSheet(viewSelector, tokens, surface)` | For a tree keeping its own indentation and branch column, where the editor's item trees give both up to `EditorTreeDelegate`: the design's chevrons in `mutedText`, right while closed and down while open, and no picture at all on a row with nothing inside it. The column is held to the view's surface rather than washed with the row - a tree draws one square branch cell per level of indentation, so a wash carried through them reads as a block glued to the leading edge of the rounded pill rather than as part of it, and the chosen row is filled across the whole of its width by the palette's highlight before a branch is drawn, so a rule painting nothing there leaves that band on show. Empty where the chevrons could not be cached |
| Rows of a list or a tree | `itemRowStyleSheet(viewSelector, tokens, surface, rowGutter)` | The view on its own surface with no frame and no focus outline, every row cut to `scmRadiusPanel`, washed in `hoverSoft` under the pointer and filled with `blend(surface, accent, scmAccentWashStrength)` and inked `accentText` while chosen. The `border-left` it writes is the gutter the accent bar stands in - transparent on every row, never coloured, and given back out of the row's leading padding, so what a delegate draws at the leading edge stays where it was. The bar itself is `paintAccentBar()` in that view's delegate, which cuts it to the pill's own corner rather than to the arc a `border-left` is bent into. `viewSelector` is `QTreeWidget` for the editor's seven item trees and `QListWidget#packageList` for the package manager's list; `surface` is the pane in both |
| A toolbar too narrow for its buttons | `ToolBarOverflow::restyle(pBar, tokens)` (`src/ToolBarOverflow.h`) | Turns Qt's overflow button into a chevron and "3 more". Call it from every restyle of the bar's window; the first call sets it up, and the rest follows the bar by itself. Never a `qt_toolbar_ext_button` rule of a window's own |
| Toolbar or sidebar glyphs | `tintedIcon(glyph, tokens)` / `restyleActionGlyphs(ActionGlyph ledger, tokens)` | Re-run on every appearance change |
| A glyph in its old picture's colour | `ActionGlyph::identity` (`scmIdentity_*`) + `restyleToolBarGlyphs(ledger, tokens, coloured)`, or `identityIcon()` | The "Colored icons" experiment, app-wide, on the main toolbar and in the script editor. Line from `identityInk()`, wash from `glyphFillPixmap()`; a glyph whose lines leave its middle open ships a `<name>-fill.svg`, and one with dots of paint a `<name>-dots.svg`. See `docs/design-language.md` section 2 |
| A word in a box (chip) | `uiDesign::ChipRow` / `Chip` (`src/ChipRow.h`), `chipFont()` | `scmRadiusChip` |
| Segmented two-way control | Editor's `QRadioButton[editorSegment]` rules | Read before adding a third |
| A row of "word field word" | `buildControlSentenceRow(row, sentence, controls)` | Placeholders reorder per language |
| Grip dots on a draggable thing | `gripGlyphFile(colour, alongTheBar)` | Background image, not `image:` (it stretches) |
| Focus ring for a deep link | `spotlightStyleSheet(accent, strength)` | |
| A link in rich text | `withLinkColour(richText, colour)` and keep the raw text in a property | A label bakes link colour at `setText` time |
| Instant apply and dirty tracking | `SettingsSnapshot`, `controlValue()`, `beingTypedInto()` | See `docs/design-language.md` section 5 |
| Search over a widget tree | `collectSearchText()`, `foldForSearch()`, `setSearchMatch()` | Section 4 |

## Which surfaces are in

Adopted (styled through `uiDesign`, guarded by the tests below):

- Settings dialog `dlgProfilePreferences` (shell, sidebar, cards, fields, marks, buttons, search)
- Script editor `dlgTriggerEditor` (toolbar, sidebar, trees via `EditorTreeDelegate` /
  `VariableTreeDelegate` - the Folders view's tree included, see `docs/design-language.md`
  "One sidebar for both windows" - seven forms, pattern rows, options strip, chips, code heading, notice).
  A package's top folder carries `RowMark::Package` - the package manager's own glyph - so it
  reads as the package rather than as a folder somebody made
- The editor's notice `dlgSystemMessageArea`, which draws itself from `noticeStyleSheet()` and
  `applyNoticeGlyph()` on every appearance change - its own frame, its own three glyphs - rather
  than being written to by the window holding it; the editor keeps only the cross that dismisses it
- About dialog `dlgAboutDialog` (`AboutLinkButton`, `AboutSupporterBanner`)
- Connection dialog `dlgConnectionProfiles` - the two tabs' fields, marks and labels, every
  button on the window (`profileAdminArea` and `widget_bottom`, Copy through
  `scmProp_menuButton`), the notice under the games list through the shared notice recipe, and
  the games-list chips cut to `scmRadiusChip` at the screen's pixel ratio - the generated
  ones, every shipped game banner and a user's own picture - which carry a halo on that same
  corner: under the pointer a 2px accent ring standing 2px off the picture, and while it is the
  chosen one the same ring in the update green (`scmStateHue_update`) - a solid frame filling the
  gap met the picture's rounded corner, and the list showed through both antialiased edges there.
  Both inks are walked with `readableOn()` to `scmQuietMinimumRatio` against the list's field
  tone, since on the light appearance the platform's accent is a pastel that reads as nothing on
  a white list. Both come
  from `ProfileChipDelegate`, which paints the pictures itself rather than calling the base
  delegate - that is what drew the platform's own selection - and is painted rather than written
  as `::item:hover` / `::item:selected` rules, since those hand the whole list to Qt's stylesheet
  style. The item's rectangle is the delegate's own `sizeHint()`, 4px larger than the picture on
  every side, or the view leaves halo fragments behind when the pointer moves off a row. The
  list's own surface and the group box frame round the information box are still platform-drawn; the
  description inside that box is a field through `inputStyleSheet()` scoped to `#informationArea`
- Notepad `dlgNotepad` - the strip of tabs as chips, the notes as fields, the find
  bar, and the bottom toolbar with the seam on its top, carrying the send strip in
  its three readings (reach, sending, no prefix)
- Package manager `dlgPackageManager` - the three views as a row of chips, the search field
  with its glyph inside it, the list's rows through `PackageItemDelegate` on the shared row
  recipe, the notice under it, the buttons of both columns, the details head at the type scale
  and the package's notes as a field. The two columns hang in a `uiDesign::GripSplitter`
  (`packagesSplitter`) rather than in a fixed row, neither pane collapsible, its sizes saved in
  `closeEvent()` under `packageManagerSplitterState` and put back on the first `showEvent()` -
  not in the constructor, since cocoa drops the geometry handed to a native window made before
  it is shown - so the hairline down the list column's trailing edge is gone and the handle's
  own seam is what parts the two. At the head of the details column the version stands beside
  the name rather than on the caption line under it, as the bare number at the window's body
  font, on the name's baseline and inked
  `readableOn(page, stateColor(scmStateHue_ok, darkPage), text, scmTextMinimumRatio)` - the same
  blue a running item's dot in the editor's trees is filled in. Every row in the list says the
  same thing about the package it stands for: the version right-aligned on the name's own line,
  flush with the row's trailing padding, at the caption step and in that same blue - walked
  against the pane for a plain row and against the chosen row's accent wash for the chosen one.
  Install, Update and Remove say the bare word for one chosen row and "%n packages" for more.
  The list also carries a
  right-click menu built at the moment it is needed - `menuStyleSheet()` and
  `letPopupsTakeTheFieldsCorner()` from the code that builds it - holding the acts of the view
  being looked at and nothing else: `packagesMenuRemove` in the Installed view,
  `packagesMenuInstall` in Explore and Updates.
  The window is dragged down to what the two columns need and no further - the `.ui` file's
  900x600 pin is cleared in `buildShell()`, and no label at the head of the details column is
  left with its own words as a floor. The action row gives way at a measured breakpoint instead
  of holding the window open: below the width the row needs with every button worded -
  measured from the sizeHints of the buttons the current view and selection show, the gaps and
  the margins, taken again wherever those words change - `pushButton_website` and
  `pushButton_report` drop to their glyphs alone, keeping their words in their accessible names
  and their tooltips, and take them back one gap's worth above that same width, so a drag
  across the line settles. Each is given the floor of its own icon-only sizeHint, measured on
  every pass of the style, or the layout would never squeeze the row far enough to reach the
  breakpoint
- Package exporter `dlgPackageExporter` - the package manager's sibling, and the same shape: a
  row across the top carrying the clear button at its leading edge and the installed-package picker at its trailing one, then
  two columns in a `uiDesign::GripSplitter` (`exporterSplitter`, sizes under
  `packageExporterSplitterState`, restored on the first `showEvent()`) - the tree of things to
  export on the pane, the package's details on the page - over a foot carrying where it saves
  to, the shared notice and the buttons. The `.ui` file's vertical fold is gone:
  `pushButton_openInfos` and the two splitters it opened are hidden with their contents pulled
  out, so a package is no longer exported with no author, no summary and no version because
  nobody pressed a button. The tree's rows are `itemRowStyleSheet()` on the pane with
  `itemCheckMarkStyleSheet()` for the tick, `treeBranchStyleSheet()` for the chevron and
  `ExporterTreeDelegate` for the accent bar; the six top-level rows carry the editor's own unit
  glyphs and every folder under them the folder one, re-inked by a walk of the tree under a
  `QSignalBlocker` - `itemChanged` fires for a picture as well as for a tick, and the slot it is
  wired to hands a row's tick down to its children. A folder's box says what is ticked inside it:
  the dash while anything under it is ticked, a full tick only when the folder itself was ticked. A
  folder at the root of a unit carrying the package's own name - typed on the form or chosen in the
  picker - stands for its contents, and is never written into the file or counted, since install
  makes that folder again. The heading over the tree keeps the words
  the group box was titled with and the count moves to a chip beside it
  (`exporterSelectionCount`, `exporterCountEmpty`), while that box is still titled with the
  count, since the dialog's own test reads it there. The packages this one needs are a
  `uiDesign::ChipRow` over the hidden `comboBox_dependencies` the export actually writes into,
  and the files it ships are a drop zone - the one control on the form drawn with a dashed
  hairline rather than by the input recipe, written as a single rule in that column's sheet.
  `fitAssetsList()` gives that zone every file plus one empty line and takes the room off the
  description above it, keeping four lines of description and letting the zone scroll once it is
  down to those, never shorter than two lines. The
  result is said in a `dlgSystemMessageArea` (`exporterNotice`) rather than in a colour baked
  into `infoLabel`, which is hidden and still written to as plain html for the two tests that
  read it. The package is named in `lineEdit_packageName`, which is the first field of the
  details column under the `exporterNameLabel` caption at the ordinary field height, so every
  field the reader fills in is in one place. Export is the one `scmProp_primaryButton` on the
  window. `exporterStartOverButton` stands before the picker on the top row, reads "Clear all
  fields and selections" and puts the form back to how the window opened - always on show and
  disabled while there is nothing on it to clear; the picker's first entry does the same act,
  and both ask first when words were typed by hand
- Profile tab strip `TTabBar` (main window and detached windows) - chips, the chosen one filled on
  a light page with the accent walked dark enough for `field`'s white to read on it and washed with
  the sidebar's accent bar and an outline on a dark one, its word in bold as the sidebar's chosen
  row is, with every tab measured bold so the strip does not shift when the choice moves; that
  chip's word, cross and ring in that white on the fill, and `accentText` walked to 7:1 on the
  wash; cross and connection indicators painted
  by its own style from the tokens and the shared tab constants; no stylesheet
  of its own, so a profile's Lua sheet keeps the last word; guarded by `ProfileTabBarStyleTest`
- Main window toolbar, the replay bar beside it and every detached window's toolbar
  (`mudlet.cpp`, `TDetachedWindow.cpp`) - the flat bar recipe through
  `mudlet::toolBarShellStyleSheet()`, `uiDesign::ToolBarOverflow` on Qt's overflow button, the split halves on
  every button whose popup mode says it has one (`QToolButton[popupMode="1"]` - Connect,
  Sound, Packages and an addon's command button), a checked action lit in the accent's wash
  rather than sunk, and the profile's own Lua sheet appended after the design's on the same
  widget; guarded by `MainToolBarStyleTest`. While an update waits, About is `aboutUpdateButton`:
  the Lucide "!" in a circle and the word "Update", both in `scmStateHue_update` walked to
  `scmTextMinimumRatio`, with no menu arrow; guarded by `AboutMenuReleaseNoticeTest`
- Debug filter bar `TDebugFilterBar`
- The console's foot `TConsole` (main window, user windows and miniconsoles with a command
  line) - one 1px seam along its top mixed from the profile's command-line colours at
  `scmConsoleFootSeamStrength`, no frame on the input; guarded by `ConsoleFootSeamTest`

Not yet adopted (platform or Fusion drawn; a new feature there still uses the recipes above
for anything it adds, and adoption is one window per pull request):

- Main window body: search field, bottom icon buttons, dock title bars
- Mapper dock `dlgMapper` and its map controls; `dlgRoomProperties`, `dlgRoomExits`, `dlgMapLabel`
- Module manager `dlgModuleManager`
- Composer `dlgComposer`, IRC `dlgIRC`
- Colour trigger picker `dlgColorTrigger`, UI tour
- User-made toolbars `TToolBar` / `TEasyButtonBar` (profile-owned look, mostly out of scope)

## Mocking it up first

Propose a window or a form as one hand-written HTML page published with the Artifact tool,
not as a multi-artboard design canvas (the `design` skill). The reader wants every variant on
one page behind its own controls - a shell switch, a light/dark toggle, a size picker - and
working interaction where the proposal is about interaction (a chip row, a key-capture
field), rather than screens laid out side by side to scroll around.

- Compute both appearances from a JavaScript transcription of `themeTokens()` run against
  the app's real palettes (`DarkTheme.cpp` and a Fusion light palette), so the tones on the
  page are the tones the app will derive; do not pick colours by eye.
- Draw glyphs from the same Lucide SVGs the app ships, at the token stroke width.
- One page per round; a later round is a new page, so decisions stay attached to what was
  looked at.

## Bringing a surface in: the checklist

1. Screenshot it first, both appearances, with the harness recipe under "Seeing the result".
2. Build the shell at runtime over the existing `.ui` (rule 9). Name every new widget with
   the window's prefix; put `markAsShellSurface()` on scaffolding a profile stylesheet must not
   paint.
3. In one `apply<Window>ShellStyle()` compose the sheet from the recipes: page/pane surfaces,
   `cardStyleSheet` if it has cards, `inputStyleSheet` / `choiceStyleSheet` /
   `buttonStyleSheet` scoped to the content container, `scrollBarStyleSheet` per scroll area,
   glyph ledgers through `restyleActionGlyphs`. Set it on the shell widget, never on the
   dialog: `mudlet` assigns the profile's Lua stylesheet to dialogs on show.
4. Re-run that function on `mudlet::signal_appearanceChanged` and on font or language change.
5. Add the window's files to `scannedFiles()` in `test/DesignColourLiteralTest.cpp` and the
   window to the walk in `test/functional_tests/ReadabilityAuditTest.cpp`. A surface in
   neither list is a surface nothing checks.
6. Add a shell test to the right grouped binary (`*_GROUP_TEST_SOURCES` in
   `test/functional_tests/CMakeLists.txt`), reaching widgets by object name. Prove it fails
   without the change.
7. Document the window's names and properties in `docs/design-language.md` section 3, and its
   adoption in this file's roster.

## Seeing the result

Nothing about a stylesheet can be judged from the source. Grab the window:

- The functional tests link `mudlet_app` and `mudlet_core` statically; a **standalone** test
  binary drops the `mudlet.qrc` resources, so call `Q_INIT_RESOURCE(mudlet)` first or every SVG
  glyph comes back null and every combo box falls back to the platform's drawing. The grouped
  binaries init resources in `GroupedTestMain.cpp`.
- Build the dialog the way the app does (`ReadabilityAuditTest::initTestCase()` is the
  template: XDG dirs, `TelnetServerStub`, `TestProfile::create`, then the dialog), `show()`
  it, switch appearance through the settings dialog's `comboBox_appearance` (the path that
  restyles every open window), then `pWindow->grab().toImage().save(...)`.
- Run on the real platform, not `QT_QPA_PLATFORM=offscreen`, when the question is how a
  control looks: macOS dark uses the Fusion-based `DarkTheme` proxy, macOS light the native
  style, and offscreen is Fusion for both.
- Sample pixels rather than trusting your eye for a contrast claim; the audit tests show how.

## Guards to keep green

`DesignColourLiteralTest`, `ReadabilityAuditTest`, `EditorChromeInkTest`, `EditorSurfaceToneTest`,
`SettingsAppearanceTest`, `SettingsShellNavigationTest`, and the shell test of whichever window
you touched. When a stylesheet builder is refactored, diff the generated sheets before and after
in both appearances. The harness for that is not kept in the tree: it was a functional test that
opened the settings dialog and the editor and wrote every generated stylesheet to the file named
by `MUDLET_STYLESHEET_DUMP`, so two trees could be compared byte for byte. Rewrite it from that description, wire it in by
hand, then remove it again - see "Verifying a stylesheet extraction" in `docs/design-language.md`.
