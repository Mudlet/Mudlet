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
  Geyser.Window.reposition(self)
  if self.autoWrap then
    self:resetAutoWrap()
  end
end

--- Replaces the currently selected text.
-- @param with The text to use as a replacement.
function Geyser.MiniConsole:replace (with)
  replace(self.name, with)
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
  setFont(self.name, font)
end

--- Returns the font family in use by this miniconsole.
function Geyser.MiniConsole:getFont()
  return getFont(self.name)
end

--- Sets the point at which text is wrapped in this miniconsole
-- @param wrapAt The number of characters to start wrapping.
function Geyser.MiniConsole:setWrap (wrapAt)
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

-- Enables the scroll bar for this window
-- @param isVisible boolean to set visibility.
function Geyser.MiniConsole:enableScrollBar()
  enableScrollBar(self.name)
end

-- Disables the scroll bar for this window
-- @param isVisible boolean to set visibility.
function Geyser.MiniConsole:disableScrollBar()
  disableScrollBar(self.name)
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
  self.parent.setFontSize(self, size)
  setMiniConsoleFontSize(self.name, size)
end

--- Appends copied selection to this miniconsole.
function Geyser.MiniConsole:appendBuffer()
  appendBuffer(self.name)
end

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

--- inserts clickable text into the miniconsole at the end of the current line
function Geyser.MiniConsole:echoLink(...)
  echoLink(self.name, ...)
end

--- inserts clickable text into the miniconsole at the current cursor position
function Geyser.MiniConsole:insertLink(...)
  insertLink(self.name, ...)
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

--- Set auto wrap on for the miniconsole
function Geyser.MiniConsole:setAutoWrap()
  self.autoWrap = true
  self:resetAutoWrap()
end

--- Set the wrap based on how wide the console is
function Geyser.MiniConsole:resetAutoWrap()
  local fontWidth, fontHeight = calcFontSize(self.fontSize)
  local consoleWidth = self.get_width()
  local charactersWidth = math.floor(consoleWidth / fontWidth)

  self:setWrap(charactersWidth)
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
  createMiniConsole(me.name, me:get_x(), me:get_y(),
  me:get_width(), me:get_height())

  -- Set any defined colors
  Geyser.Color.applyColors(me)

  if cons.wrapAt then
    me:setWrap(cons.wrapAt)
  end
  if cons.fontSize then
    me:setFontSize(cons.fontSize)
  elseif container then
    me:setFontSize(container.fontSize)
  else
    me:setFontSize(8)
  end
  if cons.scrollBar then
    me:enableScrollBar()
  end
  if cons.font then
    me:setFont(cons.font)
  end
  --print("  New in " .. self.name .. " : " .. me.name)
  return me
end
