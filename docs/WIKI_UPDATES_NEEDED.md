# Wiki Updates Needed for Issue #1885

This document contains detailed content that was removed from UI tooltips and should be added to the Mudlet wiki.

## Manual:Unicode - Ambiguous Width Characters Section

**URL:** https://wiki.mudlet.org/w/Manual:Unicode#Ambiguous_width_characters

**Content to add:**

### Ambiguous Width Characters

Some East Asian MUDs may use glyphs (characters) that Unicode classifies as being of *Ambiguous* width when drawn in a font with a so-called *fixed* pitch; in fact such text is *duo-spaced* when not using a proportional font. These symbols can be drawn using either a half or the whole space of a full character.

By default Mudlet tries to choose the right width automatically but you can override the setting for each profile in **Preferences → General**.

#### Settings

The control has three settings:

- **Unchecked** '*narrow*' = Draw ambiguous width characters in a single 'space'.
- **Checked** '*wide*' = Draw ambiguous width characters two 'spaces' wide.
- **Partly checked** *(Default) 'auto'* = Use 'wide' setting for MUD Server encodings of **Big5**/**Big5-HKSCS**, **GBK**, **GBK18030** or **EUC-KR** and 'narrow' for all others.

*Note: This is a temporary arrangement and will probably change when Mudlet gains full support for languages other than English.*

---

## Manual:Mapper - Map Symbol Font Section

**URL:** https://wiki.mudlet.org/w/Manual:Mapper#Map_symbol_font

**Content to add:**

### Map Symbol Font

You can select which font is used to display room symbols on the 2D mapper. This setting is found in **Preferences → Mapper**.

#### Font Settings

**Only use symbols (glyphs) from chosen font:** Using a single font is likely to produce a more consistent style but may cause the *font replacement character* '**�**' to show if the font does not have a needed glyph (a font's individual character/symbol) to represent the grapheme (what is to be represented). Clearing this checkbox will allow the best alternative glyph from another font to be used to draw that grapheme.

---

## Manual:Mapper - Glyph Usage Section

**URL:** https://wiki.mudlet.org/w/Manual:Mapper#Glyph_usage

**Content to add:**

### Glyph Usage

The **Show Glyph Usage** button (found in **Preferences → Mapper**) brings up a display showing:

- All the symbols used in the current map
- Whether they can be drawn using just the specified font, any other font, or not at all
- The sequence of Unicode *code-points* that make up each symbol, so they can be identified even if they cannot be displayed
- Up to the first thirty-two rooms that are using each symbol, which may help identify any unexpected or odd cases

This is useful for troubleshooting symbol display issues and ensuring your chosen font can display all the symbols you're using in your map.

---

## Manual:Keybindings - Multiple Matches Section

**URL:** https://wiki.mudlet.org/w/Manual:Keybindings#Multiple_matches

**Content to add:**

### Multiple Matching Keybindings

By default, Mudlet only reacts to the **first** matching keybinding (combination of key and modifiers) even if more than one keybinding is set to be active. This means that a temporary keybinding (not visible in the Editor) created by a script or package may be used in preference to a permanent one that is shown and is set to be active.

#### Run All Keybindings Option

You can enable the **Run all keybindings** option in **Preferences → General** to make Mudlet run all matching keybindings instead of just the first one.

**Compatibility note:** It is recommended to **not** enable this option if you need to maintain compatibility with scripts or packages for Mudlet versions prior to **3.9.0**.

---

## Manual:Timers - Debugging Section

**URL:** https://wiki.mudlet.org/w/Manual:Timers#Debugging

**Content to add:**

### Debugging Timers

A timer with a short interval will quickly fill up the *Central Debug Console* window with messages that it ran correctly on *each* occasion it is called.

#### Timer Debug Output Threshold

In **Preferences → General**, the **Timer debug output minimum interval** control (per profile) adjusts a threshold that will hide success messages in the Central Debug Console for timers which run **correctly** when the timer's interval is less than this setting.

**Important:** Any timer script that has errors will still have its error messages reported in the debug console, whatever this setting is.

---

## Manual:Technical Manual - Text Analyzer Section

**URL:** https://wiki.mudlet.org/w/Manual:Technical_Manual#Text_Analyzer

**Content to add:**

### Text Analyzer

The Text Analyzer is a utility feature that can be enabled in **Preferences → General**.

When enabled, it adds a context (right click) menu action on any console/user window that, when the mouse cursor is hovered over it, will display the UTF-16 and UTF-8 items that make up each Unicode codepoint on the **first** line of any selection.

#### Purpose

This utility feature is intended to help users identify any grapheme (visual equivalent to a *character*) that a game server may send, even if it is composed of multiple bytes—as any non-ASCII character will be in the Lua sub-system, which uses the UTF-8 encoding system.

This is particularly useful when:
- Debugging trigger patterns that should match special characters
- Understanding how multi-byte characters are encoded
- Identifying unexpected characters sent by the MUD server

---

## Manual:Preferences - Appearance Section

**URL:** https://wiki.mudlet.org/w/Manual:Preferences#Appearance *(page may need to be created)*

**Content to add:**

### Appearance Settings

#### Show Icons on Menus

Some Desktop Environments tell Qt applications like Mudlet whether they should show icons on menus; others, however, do not. This control allows you to override the setting if needed.

**Options:**

- **Unchecked** '*off*' = Prevent menus from being drawn with icons.
- **Checked** '*on*' = Allow menus to be drawn with icons.
- **Partly checked** *(Default) 'auto'* = Use the setting that the system provides.

**Note:** This setting is only processed when individual menus are created and changes may not propagate everywhere until Mudlet is restarted.

---

## Summary

These sections contain the detailed explanations that were previously embedded in UI tooltips. Adding them to the wiki will:

1. Reduce translation burden by ~3,000+ characters
2. Allow users to access detailed information without cluttering the UI
3. Make it easier to maintain and update documentation
4. Improve tooltip usability by keeping them brief and scannable

**Related:** Issue #1885 - https://github.com/Mudlet/Mudlet/issues/1885
