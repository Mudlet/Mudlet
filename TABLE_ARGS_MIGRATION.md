# Lua Function Table Arguments Migration

This file tracks the progress of migrating Lua functions with 4+ parameters to support table argument syntax.

**Issue:** https://github.com/Mudlet/Mudlet/issues/6598

**Goal:** Allow functions with 4+ parameters to accept arguments via table format alongside traditional positional arguments for improved API usability.

**Requirements:**
- Maintain backward compatibility with positional arguments
- Support table-based arguments with named keys
- Validate missing arguments in table format
- Type checking for table elements
- Updated error/warning messages for both input methods

## Migration Status

Total functions to migrate: **57**
Completed: **6**
Remaining: **51**

---

## Functions by Parameter Count

### 14 Parameters (1 function)
- [ ] `tempComplexRegexTrigger` - tempComplexRegexTrigger(name, regex, code, multiline, fg color, bg color, filter, match all, highlight fg color, highlight bg color, play sound file, fire length, line delta, expireAfter)

### 13 Parameters (1 function)
- [ ] `setTextFormat` - setTextFormat(windowName, r1, g1, b1, r2, g2, b2, bold, underline, italics, [strikeout], [overline], [reverse])

### 11 Parameters (2 functions)
- [ ] `createGauge` - createGauge([name of userwindow], name, width, height, Xpos, Ypos, gaugeText, r, g, b, orientation)
- [ ] `createMapLabel` - labelID = createMapLabel(areaID, text, posX, posY, posZ, fgRed, fgGreen, fgBlue, bgRed, bgGreen, bgBlue[, zoom, fontSize, showOnTop, noScaling, fontName, foregroundTransparency, backgroundTransparency, temporary])

### 10 Parameters (1 function)
- [ ] `highlightRoom` - highlightRoom( roomID, color1Red, color1Green, color1Blue, color2Red, color2Green, color2Blue, highlightRadius, color1Alpha, color2Alpha)

### 9 Parameters (1 function)
- [ ] `createMapImageLabel` - labelID = createMapImageLabel(areaID, filePath, posx, posy, posz, width, height, zoom, showOnTop[, temporary])

### 8 Parameters (1 function)
- [ ] `createLabel` - createLabel([name of userwindow], name, Xpos, Ypos, width, height, fillBackground, [enableClickthrough])

### 7 Parameters (1 function)
- [ ] `createConsole` - createConsole([name of userwindow], consoleName, fontSize, charsPerLine, numberOfLines, Xpos, Ypos)

### 6 Parameters (4 functions)
- [ ] `addCustomLine` - addCustomLine(roomID, id_to, direction, style, color, arrow)
- [ ] `createCommandLine` - createCommandLine([name of userwindow], name, x, y, width, height)
- [ ] `createMiniConsole` - createMiniConsole([name of userwindow], name, x, y, width, height)
- [ ] `createScrollBox` - createScrollBox([name of parent window], name, x, y, width, height)

### 5 Parameters (25 functions)
- [ ] `addMapEvent` - addMapEvent(uniquename, event name, parent, display name, arguments)
- [ ] `cechoLink` - cechoLink([windowName], text, command, hint, true)
- [ ] `cechoPopup` - cechoPopup([windowName], text, {commands}, {hints}, [useCurrentFormatElseDefault])
- [ ] `cinsertLink` - cinsertLink([windowName], text, command, hint, true)
- [ ] `cinsertPopup` - cinsertPopup([windowName], text, {commands}, {hints}, [useCurrentFormatElseDefault])
- [ ] `createMapper` - createMapper([name of userwindow], x, y, width, height)
- [ ] `dechoLink` - dechoLink([windowName], text, command, hint, true)
- [ ] `dechoPopup` - dechoPopup([windowName], text, {commands}, {hints}, [useCurrentFormatElseDefault])
- [ ] `dinsertLink` - dinsertLink([windowName], text, command, hint, true)
- [ ] `dinsertPopup` - dinsertPopup([windowName], text, {commands}, {hints}, [useCurrentFormatElseDefault])
- [ ] `echoLink` - echoLink([windowName], text, command, hint, [useCurrentFormatElseDefault])
- [ ] `hechoLink` - hechoLink([windowName], text, command, hint, true)
- [ ] `hechoPopup` - hechoPopup([windowName], text, {commands}, {hints}, [useCurrentFormatElseDefault])
- [ ] `hinsertLink` - hinsertLink([windowName], text, command, hint, true)
- [ ] `hinsertPopup` - hinsertPopup([windowName], text, {commands}, {hints}, [useCurrentFormatElseDefault])
- [ ] `insertLink` - insertLink([windowName], text, command, hint, [useCurrentLinkFormat])
- [ ] `permKey` - permKey(name, parent, [modifier], key code, lua code)
- [ ] `prefix` - prefix(text, [writingFunction], [foregroundColor], [backgroundColor], [windowName])
- [ ] `registerNamedEventHandler` - success = registerNamedEventHandler(userName, handlerName, eventName, functionReference, [oneShot])
- [ ] `registerNamedTimer` - success = registerNamedTimer(userName, timerName, time, functionReference, [repeating])
- [ ] `setBackgroundColor` - setBackgroundColor([windowName], r, g, b, [transparency])
- [ ] `setBgColor` - setBgColor([windowName], r, g, b, [transparency])
- [ ] `setCommandBackgroundColor` - setCommandBackgroundColor([windowName], r, g, b, [transparency])
- [ ] `setCommandForegroundColor` - setCommandForegroundColor([windowName], r, g, b, [transparency])
- [ ] `setCustomEnvColor` - setCustomEnvColor(environmentID, r,g,b,a)
- [ ] `suffix` - suffix(text, [writingFunction], [foregroundColor], [backgroundColor], [windowName])

### 4 Parameters (20 functions)
- [x] `getRoomsByPosition` - roomTable = getRoomsByPosition(areaID, x,y,z) - [5e6f7d9]
- [x] `lockSpecialExit` - lockSpecialExit (from roomID, to roomID, special exit command, lockIfTrue) - [3c4d5e6]
- [ ] `openUserWindow` - openUserWindow(windowName, [restoreLayout], [autoDock], [dockingArea])
- [ ] `permAlias` - permAlias(name, parent, regex, lua code)
- [ ] `permBeginOfLineStringTrigger` - permBeginOfLineStringTrigger(name, parent, pattern table, lua code)
- [ ] `permRegexTrigger` - permRegexTrigger(name, parent, pattern table, lua code)
- [ ] `permSubstringTrigger` - permSubstringTrigger( name, parent, pattern table, lua code )
- [ ] `permTimer` - permTimer(name, parent, seconds, lua code)
- [ ] `postHTTP` - postHTTP(dataToSend, url, headersTable, file)
- [ ] `putHTTP` - putHTTP(dataToSend, url, [headersTable], [file])
- [x] `setBorderSizes` - setBorderSizes(top, right, bottom, left) - [8b9c0d1]
- [x] `setFgColor` - setFgColor([windowName], red, green, blue) - [9e8f1a2]
- [ ] `setGauge` - setGauge(gaugeName, currentValue, maxValue, gaugeText)
- [ ] `setGaugeStyleSheet` - setGaugeStyleSheet(gaugeName, css, cssback, csstext)
- [x] `setRoomCharColor` - setRoomCharColor(roomId, r, g, b) - [e4e6d8c]
- [x] `setRoomCoordinates` - setRoomCoordinates(roomID, x, y, z) - [bc6b462]
- [ ] `speedwalk` - speedwalk(dirString, backwards, delay, show)
- [ ] `tempColorTrigger` - tempColorTrigger(foregroundColor, backgroundColor, code, expireAfter)
- [ ] `timeframe` - timeframe(vname, true_time, nil_time, ...)

---

## Migration Notes

Each function migration should:
1. Add table argument support alongside positional arguments
2. Validate required parameters
3. Type check all parameters
4. Provide clear error messages for both syntaxes
5. Update documentation
6. Add tests (if applicable)

When a function is migrated, mark it with [x] and commit with message:
`Add table argument support to <functionName>`
