--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a miniconsole primitive
-- @class table
-- @name Geyser.UserWindow
-- @field wrapAt Where line wrapping occurs. Default is 300 characters.
Geyser.UserWindow = Geyser.Window:new({
  name = "UserWindowClass",
  wrapAt = 300, })

--- Override reposition to reset autowrap
function Geyser.UserWindow:reposition()
  self.parent.reposition(self)
  if self.autoWrap then
    self:resetAutoWrap()
  end
end

--- Replaces the currently selected text.
-- @param with The text to use as a replacement.
function Geyser.UserWindow:replace (with)
  replace(self.name, with)
end

--- Sets the size of this miniconsole's buffer.
-- @param linesLimit The number of lines to store - same limitations as Mudlet
--                   function of the same name.
-- @param sizeOfBatchDeletion See Mudlet documentation. (I don't know offhand =)
function Geyser.UserWindow:setBufferSize (linesLimit, sizeOfBatchDeletion)
  setConsoleBufferSize(self.name, linesLimit, sizeOfBatchDeletion)
end

--- Sets the new font to use - use a monospaced font, non-monospaced fonts aren't supported by Mudlet
-- and won't give the best results.
-- @param font Font family name to use (see https://doc.qt.io/qt-5/qfont.html#setFamily for details)
function Geyser.UserWindow:setFont (font)
  if font then
    self.font = font
  end
  setFont(self.name, font)
end

--- Returns the font family in use by this miniconsole.
function Geyser.UserWindow:getFont()
  self.font = getFont(self.name)
  return self.font
end

--- Sets the point at which text is wrapped in this miniconsole unless autoWrap is on
-- @param wrapAt The number of characters to start wrapping.
function Geyser.UserWindow:setWrap (wrapAt)
  if self.autoWrap then return nil, "autoWrap is enabled in this UserWindow and that overrides manual wrapping" end

  if wrapAt then
    self.wrapAt = wrapAt
  end
  setWindowWrap(self.name, self.wrapAt)
end

function Geyser.UserWindow:resetFormat()
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
function Geyser.UserWindow:setTextFormat(r1, g1, b1, r2, g2, b2, bold, underline, italics)
  setTextFormat(self.name, r1, g1, b1, r2, g2, b2, bold, underline, italics)
end

function Geyser.UserWindow:calcFontSize()
  return calcFontSize(self.name)
end

-- Enables the scroll bar for this window
-- @param isVisible boolean to set visibility.
function Geyser.UserWindow:enableScrollBar()
  enableScrollBar(self.name)
end

-- Disables the scroll bar for this window
-- @param isVisible boolean to set visibility.
function Geyser.UserWindow:disableScrollBar()
  disableScrollBar(self.name)
end

--- Sets bold status for this miniconsole
-- @param bool True for bolded
function Geyser.UserWindow:setBold(bool)
  setBold(self.name, bool)
end

--- Sets underline status for this miniconsole
-- @param bool True for underlined
function Geyser.UserWindow:setUnderline(bool)
  setUnderline(self.name, bool)
end

--- Sets italics status for this miniconsole
-- @param bool True for italicized
function Geyser.UserWindow:setItalics(bool)
  setItalics(self.name, bool)
end

--- Sets the font size for this miniconsole.
-- @param size The font size.
function Geyser.UserWindow:setFontSize(size)
  if size then
    self.parent.setFontSize(self, size)
  end

  setFontSize(self.name, size)
end

--- Appends copied selection to this miniconsole.
function Geyser.UserWindow:appendBuffer()
  appendBuffer(self.name)
end

function Geyser.UserWindow:clear()
  clearWindow(self.name)
end

--- sets the current foreground color of cursor in this miniconsole.
function Geyser.UserWindow:fg(color)
  fg(self.name, color)
end

--- sets the current background color of cursor in this miniconsole.
function Geyser.UserWindow:bg(color)
  bg(self.name, color)
end

--- inserts clickable text into the miniconsole at the end of the current line
function Geyser.UserWindow:echoLink(...)
  echoLink(self.name, ...)
end

--- inserts clickable text into the miniconsole at the current cursor position
function Geyser.UserWindow:insertLink(...)
  insertLink(self.name, ...)
end

--- turns selected text info clickable text into the miniconsole
function Geyser.UserWindow:setLink(...)
  setLink(self.name, ...)
end

--- Returns the number of simultaneous rows that this miniconsole can show at once
function Geyser.UserWindow:getRowCount()
    return getRowCount(self.name)
end

--- Returns the number of simultaneous columns (characters) that this miniconsole can show at once on a single row
function Geyser.UserWindow:getColumnCount()
    return getColumnCount(self.name)
end

--- Turn on auto wrap for the miniconsole
function Geyser.UserWindow:enableAutoWrap()
  self.autoWrap = true
  self:resetAutoWrap()
end

--- Turn off auto wrap for the miniconsole, after disabling you should immediately
-- call setWrap with your desired wrap
function Geyser.UserWindow:disableAutoWrap()
  self.autoWrap = false
end

--- Set the wrap based on how wide the console is
function Geyser.UserWindow:resetAutoWrap()
  local fontWidth, fontHeight = calcFontSize(self.name)
  local consoleWidth = self.get_width()
  local charactersWidth = math.floor(consoleWidth / fontWidth)

  self.wrapAt = charactersWidth
  setWindowWrap(self.name, self.wrapAt)
end

-- Save a reference to our parent constructor
Geyser.UserWindow.parent = Geyser.Window

-- Overridden constructor
function Geyser.UserWindow:new (cons, container)
  -- Initiate and set gauge specific things
  cons = cons or {}
  cons.type = cons.type or "miniConsole"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)

  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self

  -----------------------------------------------------------
  -- Now create the UserWindow using primitives
  openUserWindow(me.name)
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
  if cons.font then
    me:setFont(cons.font)
  end
  if cons.wrapAt == "auto" then
    me:setAutoWrap()
  elseif cons.wrapAt then
    me:setWrap(cons.wrapAt)
  end
  --print("  New in " .. self.name .. " : " .. me.name)
  return me
end
