## Upcoming Release

edbee.lib:

- fix, mouse double click didn't select wordt anymore. (Issue with newer Qt version??)
- fix, moveCaret after the last character didn't work correctly on the if the last line didn't end with a newline
- fix, Syntax highlighting didn't work on the last line of the document. (First highlight after the first enter) 
- fix, onig.pri, it contained strange references to qslog 
- fix, edbee-lib.pri (correct references to vendor .pri's)
- BREAKING CHANGE, moved all source/headers files under the folder 'edbee/' to prevent filename collisions when embedding it in other projects.

- add #121, Insert line before and insert line after commands
- add #108, #111, Added a DynamicTextRangeSet, a change aware rangeset, that automatically gets adjusted when the document changes.
- add #107, Implemented scoped/unscoped environment variable support
- add #33, Toggle comment line and comment block support using the TM_COMMENT_* environment variable structure.
- add #41, Duplicate Line support (Cmd+Shift+D)
- add #96, Multiple Cut line operations should append the lines together and grow the clipboard
- add #95, Command Delete should delete everything to the end of the current line
- add #93, Control Delete should delete the current 'word'
- add #94, Command Backspace should delete everything to the start of the current line
- add #91, Control Backspace should delete the current word
- add #85, Added scroll-past-end feature
- add #78, Added language independent smart tab support (enabled by default)
- add #79, Double clicking a selection with the control key again should remove the given selection and caret
- add #74, Added coalescing support for indenting / inserting tabs
- add #58, Pressing shift-delete now deletes the selected text
- add #43, Added right-click context-menu support to edbee. With default operations, cut, copy, paste and select all.
- add #36, Pressing shift-enter now inserts a newline
- add #31, Textsearching now also works with other ranges then textselection ranges
- fix, memory leak detection contained an incorrect iterator->second dereference. Causing crashes (Thanks Blackstar)
- fix #124, Line breaks (\n) are rendered with QTextLayout, which results in a strange character on a Linux environment (github issue 2)
- fix #123, Updated oniguruma to 5.13.5 to solve a segfault on Ubuntu 13 (64bits)
- fix #122, Library can't be compiled on Linux, unix  is a predefined word on unix
- fix #118, The width of the editor component should add an extra spacing so the caret isn't placed agains the right window border
- fix #117, The last line doesn't show the caret marker in the line-number column
- fix #116, The linenumber column isn't updated properly when the number of digets increases.
- fix #114, Added a factory keymap so the editor works out of the box
- fix #103, Renamed the *TextChange related classes. So it's more clear what the changes represent
- fix #73, Complete rewrite of coalescing (change-merging) algorithm, so all textchanges are mergable. (Fixes #98,#97,#95,#41,#99,#100,#101)
- fix #86, Pressing left or right when a selection is active shouldn't move the caret (current behavior is not-standard)
- fix #68, Adding a selection with Cmd+Mouse Double click shouldn't expand existing word selections
- fix #77, Pressing end of line on the last line, sometimes goes to the wrong location
- fix #76, Pressing enter doesn't scroll the horizontal window back to the first column
- fix #75, Goto pathname in treemenu doesn't display extension
- fix #72, TextDocument replaceRanges should calculate the ranges in stead of the TextChange event.
- fix #69, Plain Text was included twice by the grammar manager
- fix #57, Tab behaviour didn't work as expected when using space in stead of tab characters
- fix #48, Improved paste support with multiple lines, making it possible to copy/paste text per caret
- fix #61, Indent shouldn't indent the line after the text 
- fix #66, Grammar type detection (by filename) detected the wrong grammars. (it forgot to check the '.' )
- fix #40, text now is by default case insensitive
- fix #30, #32, Searching selection via the findcommand now result in soft undoable changes
- fix #20, Changing TextEditorConfig now automaticly updates the state of edbee.
- fix #21, Improved fallback pallette when a theme cannot be loaded. (fixes complete black screen)
- fix #16, linespacing issue, the space always was at least 1 pixel
- fix #2, made it possible to configure TextEditorConfig. (was hardcoded)

## v0.1.0 Initial Release

The initial release on Github. 
