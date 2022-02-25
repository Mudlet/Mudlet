--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a miniconsole primitive
-- @class table
-- @name Geyser.MiniConsole
-- @field wrapAt Where line wrapping occurs. Default is 300 characters.
Geyser.MiniConsole = Geyser.Window:new({
  name = "MiniConsoleClass",
  wrapAt = 300, })

--- Override reposition to reset autowrap
function Geyser.MiniConsole:reposition()
  self.parent.reposition(self)
  if self.autoWrap then
    self:resetAutoWrap()
  end
end

--- Replaces the currently selected text.
-- @param with The text to use as a replacement.
function Geyser.MiniConsole:replace (with)
  replace(self.name, with)
end

--- Replaces the entire line the cursor is on
-- @param with The text to use as a replacement
function Geyser.MiniConsole:replaceLine (with)
  replaceLine(self.name, with)
end

--- Replaces the entire line the cursor is on, with color like cecho
-- @param with The text to use as a replacement
function Geyser.MiniConsole:creplaceLine (with)
  creplaceLine(self.name, with)
end

--- Replaces the entire line the cursor is on, with color like decho
-- @param with The text to use as a replacement
function Geyser.MiniConsole:dreplaceLine (with)
  dreplaceLine(self.name, with)
end

--- Replaces the entire line the cursor is on, with color like hecho
-- @param with The text to use as a replacement
function Geyser.MiniConsole:hreplaceLine (with)
  hreplaceLine(self.name, with)
end

--- Sets the size of this miniconsole's buffer.
-- @param linesLimit The number of lines to store - same limitations as Mudlet
--                   function of the same name.
-- @param sizeOfBatchDeletion See Mudlet documentation. (I don't know offhand =)
function Geyser.MiniConsole:setBufferSize (linesLimit, sizeOfBatchDeletion)
  setConsoleBufferSize(self.name, linesLimit, sizeOfBatchDeletion)
end

--- Sets the new font to use - use a monospaced font, non-monospaced fonts aren't supported by Mudlet
-- and won't give the best results.
-- @param font Font family name to use (see https://doc.qt.io/qt-5/qfont.html#setFamily for details)
function Geyser.MiniConsole:setFont (font)
  if font then
    self.font = font
  end
  setFont(self.name, font)
end

--- Returns the font family in use by this miniconsole.
function Geyser.MiniConsole:getFont()
  self.font = getFont(self.name)
  return self.font
end

--- Sets the point at which text is wrapped in this miniconsole unless autoWrap is on
-- @param wrapAt The number of characters to start wrapping.
function Geyser.MiniConsole:setWrap (wrapAt)
  if self.autoWrap then return nil, "autoWrap is enabled in this MiniConsole and that overrides manual wrapping" end

  if wrapAt then
    self.wrapAt = wrapAt
  end
  setWindowWrap(self.name, self.wrapAt)
end

function Geyser.MiniConsole:resetFormat()
  resetFormat(self.name)
end

--- Sets the text format for this window. Note that the *echo()
-- functions will override these settings.
-- @param r1 The red foreground component.
-- @param g1 The green foreground component.
-- @param b1 The blue foreground component.
-- @param r2 The red background component.
-- @param g2 The green background component.
-- @param b2 The blue background component.
-- @param bold The bolded status. 1 is bold, 0 is normal.
-- @param underline The underlined status. 1 is underlined, 0 is normal.
-- @param italics The italicized status. 1 is italicized, 0 is normal.
function Geyser.MiniConsole:setTextFormat(r1, g1, b1, r2, g2, b2, bold, underline, italics)
  setTextFormat(self.name, r1, g1, b1, r2, g2, b2, bold, underline, italics)
end

function Geyser.MiniConsole:calcFontSize()
  return calcFontSize(self.name)
end

--- Enables the scroll bar for this window
function Geyser.MiniConsole:enableScrollBar()
  enableScrollBar(self.name)
end

--- Disables the scroll bar for this window
function Geyser.MiniConsole:disableScrollBar()
  disableScrollBar(self.name)
end

--- Enables the horizontal scroll bar for this window
function Geyser.MiniConsole:enableHorizontalScrollBar()
  enableHorizontalScrollBar(self.name)
end

--- Disables the horizontal scroll bar for this window
function Geyser.MiniConsole:disableHorizontalScrollBar()
  disableHorizontalScrollBar(self.name)
end

-- Start commandLine functions

--- Enables the command-line for this window
function Geyser.MiniConsole:enableCommandLine()
  enableCommandLine(self.name)
end

--- Disables the command-line for this window
function Geyser.MiniConsole:disableCommandLine()
  disableCommandLine(self.name)
end

--- Sets an action to be used when text is send in this commandline. When this
-- function is called by the event system, text the commandline sends will be
-- appended as the final argument (see @{sysCmdLineEvent}) and also in Geyser.Label
-- the setClickCallback events
-- @param func The function to use.
-- @param ... Parameters to pass to the function.
function Geyser.MiniConsole:setCmdAction(func, ...)
  setCmdLineAction(self.name, func, ...)
  self.actionFunc = func
  self.actionArgs = { ... }
end

--- Resets the action the command will be send to the game
function Geyser.MiniConsole:resetCmdAction()
  resetCmdLineAction(self.name)
  self.actionFunc = nil
  self.actionArgs = nil
end

--- Clears the cmdLine
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearCmdLine
function Geyser.MiniConsole:clearCmd()
  clearCmdLine(self.name)
end

--- prints text to the commandline and clears text if there was one previously
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#printCmdLine(text)
function Geyser.MiniConsole:printCmd(text)
  printCmdLine(self.name, text)
end

--- appends text to the commandline
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#appendCmdLine
function Geyser.MiniConsole:appendCmd(text)
  appendCmdLine(self.name, text)
end

--- returns the text in the commandline
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#getCmdLine
function Geyser.MiniConsole:getCmdLine()
  return getCmdLine(self.name)
end

--- Sets the style sheet of the command-line
-- @param css The style sheet string
function Geyser.MiniConsole:setCmdLineStyleSheet(css)
  css = css or self.cmdLineStylesheet
  setCmdLineStyleSheet(self.name, css)
  self.cmdLineStylesheet = css
end

--- Sets bold status for this miniconsole
-- @param bool True for bolded
function Geyser.MiniConsole:setBold(bool)
  setBold(self.name, bool)
end

--- Sets underline status for this miniconsole
-- @param bool True for underlined
function Geyser.MiniConsole:setUnderline(bool)
  setUnderline(self.name, bool)
end

--- Sets italics status for this miniconsole
-- @param bool True for italicized
function Geyser.MiniConsole:setItalics(bool)
  setItalics(self.name, bool)
end

--- Sets the font size for this miniconsole.
-- @param size The font size.
function Geyser.MiniConsole:setFontSize(size)
  if size then
    self.parent.setFontSize(self, size)
  end
  setMiniConsoleFontSize(self.name, size)
  if self.autoWrap then
    self:resetAutoWrap()
  end
end

--- Appends copied selection to this miniconsole.
function Geyser.MiniConsole:appendBuffer()
  appendBuffer(self.name)
end

--- Clears the miniconsole
-- see: https://wiki.mudlet.org/w/Manual:Lua_Functions#clearWindow
function Geyser.MiniConsole:clear()
  clearWindow(self.name)
end

--- sets the current foreground color of cursor in this miniconsole.
function Geyser.MiniConsole:fg(color)
  fg(self.name, color)
end

--- sets the current background color of cursor in this miniconsole.
function Geyser.MiniConsole:bg(color)
  bg(self.name, color)
end

--- sets the background image of this miniconsole
-- @param imgPath image path or stylesheet option (if mode is set to "style") as string
-- @param mode background image mode (1 "border", 2 "center", 3 "tile", 4 "style")
function Geyser.MiniConsole:setBackgroundImage(imgPath, mode)
  self.imgPath = imgPath
  return setBackgroundImage(self.name, imgPath, mode)
end

--- resets the background image of this miniconsole
function Geyser.MiniConsole:resetBackgroundImage()
  self.imgPath = nil
  return resetBackgroundImage(self.name)
end

--- inserts clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#echoLink
function Geyser.MiniConsole:echoLink(...)
  echoLink(self.name, ...)
end

--- inserts clickable text into the miniconsole at the current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#insertLink
function Geyser.MiniConsole:insertLink(...)
  insertLink(self.name, ...)
end

--- inserts clickable text with right-click menu into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#echoPopup
function Geyser.MiniConsole:echoPopup(...)
  echoPopup(self.name, ...)
end

--- inserts clickable text with right-click menu into the miniconsole at the end of the current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#insertPopup
function Geyser.MiniConsole:insertPopup(...)
  insertPopup(self.name, ...)
end

--- inserts color name formatted clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#cechoLink
function Geyser.MiniConsole:cechoLink(...)
  cechoLink(self.name, ...)
end

--- inserts decimal color formatted clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#dechoLink
function Geyser.MiniConsole:dechoLink(...)
  dechoLink(self.name, ...)
end

--- inserts hexadecimal color formatted clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#hechoLink
function Geyser.MiniConsole:hechoLink(...)
  hechoLink(self.name, ...)
end

--- inserts color name formatted clickable text into the miniconsole at the end of the current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#cinsertLink
function Geyser.MiniConsole:cinsertLink(...)
  cinsertLink(self.name, ...)
end

--- inserts decimal color formatted clickable text into the miniconsole at the end of the current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#dinsertLink
function Geyser.MiniConsole:dinsertLink(...)
  dinsertLink(self.name, ...)
end

--- inserts hexadecimal color formatted clickable text into the miniconsole at the end of the current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#hinsertLink
function Geyser.MiniConsole:hinsertLink(...)
  hinsertLink(self.name, ...)
end

--- inserts color name formatted clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#cechoLink
function Geyser.MiniConsole:cechoLink(...)
  cechoLink(self.name, ...)
end

--- inserts decimal color formatted clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#dechoLink
function Geyser.MiniConsole:dechoLink(...)
  dechoLink(self.name, ...)
end

--- inserts hexadecimal color formatted clickable text into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#hechoLink
function Geyser.MiniConsole:hechoLink(...)
  hechoLink(self.name, ...)
end

--- inserts color name formatted clickable text with right-click menu into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#cechoPopup
function Geyser.MiniConsole:cechoPopup(...)
  cechoPopup(self.name, ...)
end

--- inserts decimal color formatted clickable text with right-click menu into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#dechoPopup
function Geyser.MiniConsole:dechoPopup(...)
  dechoPopup(self.name, ...)
end

--- inserts hexadecimal color formatted clickable text with right-click menu into the miniconsole at the end of the current line.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#hechoPopup
function Geyser.MiniConsole:hechoPopup(...)
  hechoPopup(self.name, ...)
end

--- inserts color name formatted clickable text with right-click menu into the miniconsole at the end of the current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#cinsertPopup
function Geyser.MiniConsole:cinsertPopup(...)
  cinsertPopup(self.name, ...)
end

--- inserts decimal color formatted clickable text with right-click menu into the miniconsole at the end of the current current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#dinsertPopup
function Geyser.MiniConsole:dinsertPopup(...)
  dinsertPopup(self.name, ...)
end

--- inserts hexadecimal color formatted clickable text with right-click menu into the miniconsole at the end of the current current cursor position.
-- see: https://wiki.mudlet.org/w/Manual:UI_Functions#hinsertPopup
function Geyser.MiniConsole:hinsertPopup(...)
  hinsertPopup(self.name, ...)
end

--- turns selected text info clickable text into the miniconsole
function Geyser.MiniConsole:setLink(...)
  setLink(self.name, ...)
end

--- Returns the number of simultaneous rows that this miniconsole can show at once
function Geyser.MiniConsole:getRowCount()
    return getRowCount(self.name)
end

--- Returns the number of simultaneous columns (characters) that this miniconsole can show at once on a single row
function Geyser.MiniConsole:getColumnCount()
    return getColumnCount(self.name)
end

--- Turn on auto wrap for the miniconsole
function Geyser.MiniConsole:enableAutoWrap()
  self.autoWrap = true
  self:resetAutoWrap()
end

--- Turn off auto wrap for the miniconsole, after disabling you should immediately
-- call setWrap with your desired wrap
function Geyser.MiniConsole:disableAutoWrap()
  self.autoWrap = false
end

--- Set the wrap based on how wide the console is
function Geyser.MiniConsole:resetAutoWrap()
  local fontWidth, fontHeight = calcFontSize(self.name)
  local consoleWidth = self.get_width()
  local charactersWidth = math.floor(consoleWidth / fontWidth)

  self.wrapAt = charactersWidth
  setWindowWrap(self.name, self.wrapAt)
end

--- The same as Mudlet's base display(), but outputs to the miniconsole instead of the main window.
function Geyser.MiniConsole:display(...)
  local arg = {...}
  arg.n = table.maxn(arg)
  if arg.n > 1 then
    for i = 1, arg.n do
      self:display(arg[i])
    end
  else
    self:echo((inspect(arg[1]) or 'nil') .. '\n')
  end
end

--- gets the font size for the miniconsole
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getFontSize
function Geyser.MiniConsole:getFontSize()
  return getFontSize(self.name)
end

--- Moves the virtual cursor within the miniconsole
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#moveCursor
-- @param x the horizontal position for the cursor
-- @param y the vertical position (line) for the cursor
function Geyser.MiniConsole:moveCursor(x, y)
  moveCursor(self.name, x, y)
end

--- Moves the virtual cursor up 1 or more lines
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#moveCursorUp
-- @param lines the number of lines up to move the cursor. Defaults to 1 if not provided
-- @param keepColumn if true, will maintain the x/horizontal/columnar position of the cursor as well. Otherwise moves the cursor to the front of the line
function Geyser.MiniConsole:moveCursorUp(lines, keepColumn)
  moveCursorUp(self.name, lines, keepColumn)
end

--- Moves the virtual cursor down 1 or more lines
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#moveCursorDown
-- @param lines the number of lines down to move the cursor. Defaults to 1 if not provided
-- @param keepColumn if true, will maintain the x/horizontal/columnar position of the cursor as well. Otherwise moves the cursor to the front of the line
function Geyser.MiniConsole:moveCursorDown(lines, keepColumn)
  moveCursorDown(self.name, lines, keepColumn)
end

--- Moves the virtual cursor to the end of the miniconsole
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#moveCursorEnd
function Geyser.MiniConsole:moveCursorEnd()
  moveCursorEnd(self.name)
end

--- Returns the absolute line number the cursor is on in the miniconsole.
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getLineNumber
function Geyser.MiniConsole:getLineNumber()
  return getLineNumber(self.name)
end

--- Returns the number of lines in the miniconsole
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getLineCount
function Geyser.MiniConsole:getLineCount()
  return getLineCount(self.name)
end

--- Returns the absolute column number the virtual cursor is on
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getColumnNumber
function Geyser.MiniConsole:getColumnNumber()
  return getColumnNumber(self.name)
end

--- Returns the latest line's number in the miniconsole
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getLastLineNumber
function Geyser.MiniConsole:getLastLineNumber()
  return getLastLineNumber(self.name)
end

--- returns a section of the content of the miniconsole text buffer. Returns a Lua table with the content of the lines on a per line basis. The form value is result = {relative_linenumber = line}.
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getLines
-- @param fromLine the absolute line number to start getting lines at
-- @param toLine the absolute line number to stop getting lines at
function Geyser.MiniConsole:getLines(fromLine, toLine)
  return getLines(self.name, fromLine, toLine)
end

--- returns the content of the current line under the virtual cursor
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getCurrentLine
function Geyser.MiniConsole:getCurrentLine()
  return getCurrentLine(self.name)
end

--- select the text within the miniconsole's command line
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#selectCmdLineText
function Geyser.MiniConsole:selectCmdLinetext()
  return selectCmdLineText(self.name)
end

--- Select the current line the cursor is on in the miniconsole
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#selectCurrentLine
function Geyser.MiniConsole:selectCurrentLine()
  return selectCurrentLine(self.name)
end

--- Selects the specified parts of the line starting from the left and extending to the right for however how long. The line starts from 0.
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#selectSection
-- @param fromPosition the column to start selecting from
-- @param length the number of columns to select
function Geyser.MiniConsole:selectSection(fromPosition, length)
  return selectSection(self.name, fromPosition, length)
end

--- Selects a substring from the line where the user cursor is currently positioned - allowing you to edit selected text (apply colour, make it be a link, copy to other windows or other things).
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#selectString
function Geyser.MiniConsole:selectString(text, number_of_match)
  return selectString(self.name, text, number_of_match)
end

--- Returns the text currently selected by the virtual cursor using :selectString, :selectSection, etc (not selected by the mouse)
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getSelection
function Geyser.MiniConsole:getSelection()
  return getSelection(self.name)
end

--- Gets the current text format of the currently selected text.
-- see https://wiki.mudlet.org/w/Manual:UI_Functions#getTextFormat
function Geyser.MiniConsole:getTextFormat()
  return getTextFormat(self.name)
end

--- Gets the number of columns the window is configured to wrap at
function Geyser.MiniConsole:getWindowWrap()
  return getWindowWrap(self.name)
end

-- Save a reference to our parent constructor
Geyser.MiniConsole.parent = Geyser.Window

-- Overridden constructor
function Geyser.MiniConsole:new (cons, container)
  -- Initiate and set gauge specific things
  cons = cons or {}
  cons.type = cons.type or "miniConsole"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)

  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self
  -----------------------------------------------------------
  -- Now create the MiniConsole using primitives
  if not string.find(me.name, ".*Class") then
    me.windowname = me.windowname or me.container.windowname or "main"
    createMiniConsole(me.windowname,me.name, me:get_x(), me:get_y(),
    me:get_width(), me:get_height())

-- This only has an effect if add2 is being used as for the standard add method me.hidden and me.auto_hidden is always false at creation/initialisation
    if me.hidden or me.auto_hidden then
      hideWindow(me.name)
    end

    -- Set any defined colors
    Geyser.Color.applyColors(me)

    if cons.fontSize then
      me:setFontSize(cons.fontSize)
    elseif container then
      me:setFontSize(container.fontSize)
      cons.fontSize = container.fontSize
    else
      me:setFontSize(8)
      cons.fontSize = 8
    end
    if cons.scrollBar then
      me:enableScrollBar()
    else
      me:disableScrollBar()
    end
    if cons.horizontalScrollBar then
      me:enableHorizontalScrollBar()
    else
      me:disableHorizontalScrollBar()
    end
    if cons.font then
      me:setFont(cons.font)
    end
    if cons.wrapAt == "auto" then
      me:enableAutoWrap()
    elseif cons.wrapAt then
      me:setWrap(cons.wrapAt)
    end
    if me.commandLine then
      me:enableCommandLine()
    else
      me:disableCommandLine()
    end
    if me.cmdLineStylesheet and me.commandLine then
      me:setCmdLineStyleSheet()
    end
    --print("  New in " .. self.name .. " : " .. me.name)
  end
  return me
end

--- Overridden constructor to use add2
function Geyser.MiniConsole:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
