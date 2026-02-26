--- Represents an embeddable multi-line text editor primitive.
-- @author Mudlet Makers
-- @module Geyser.TextEdit

--- Represents an embeddable multi-line text editor primitive.
Geyser.TextEdit = Geyser.Window:new({
  name = "TextEditClass"
})

-- Save a reference to our parent constructor
Geyser.TextEdit.parent = Geyser.Window

--- Returns the text content of the text edit.
-- @return string the current text
function Geyser.TextEdit:getText()
  return getTextEditText(self.name)
end

--- Sets the text content of the text edit.
-- @param text the text to set
function Geyser.TextEdit:setText(text)
  setTextEditText(self.name, text)
end

--- Clears the text edit content.
function Geyser.TextEdit:clear()
  clearTextEdit(self.name)
end

--- Sets whether the text edit is read-only.
-- @param bool true to make read-only, false to make editable
function Geyser.TextEdit:setReadOnly(bool)
  setTextEditReadOnly(self.name, bool)
end

--- Sets the placeholder text shown when the text edit is empty.
-- @param text the placeholder text
function Geyser.TextEdit:setPlaceholder(text)
  setTextEditPlaceholder(self.name, text)
end

--- Sets the style sheet of the text edit.
-- @param css the style sheet string
function Geyser.TextEdit:setStyleSheet(css)
  css = css or self.stylesheet
  setTextEditStyleSheet(self.name, css)
  self.stylesheet = css
end

--- Sets the font of the text edit.
-- @param font the font family name
function Geyser.TextEdit:setFont(font)
  setTextEditFont(self.name, font)
end

--- Sets the font size of the text edit.
-- @param size the font size in points
function Geyser.TextEdit:setFontSize(size)
  setTextEditFontSize(self.name, size)
end

-- Overridden constructor
function Geyser.TextEdit:new(cons, container)
  cons = cons or {}
  cons.type = cons.type or "textEdit"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)
  me.windowname = me.windowname or me.container.windowname or "main"

  -- Set the metatable
  setmetatable(me, self)
  self.__index = self

  createTextEdit(me.windowname, me.name, me:get_x(), me:get_y(), me:get_width(), me:get_height())
  if me.stylesheet then
    me:setStyleSheet()
  end
  if me.hidden or me.auto_hidden then
    hideWindow(me.name)
  end

  return me
end

--- Deletes the text edit using the C++ deleteTextEdit function.
function Geyser.TextEdit:type_delete()
  deleteTextEdit(self.name)
end

--- Overridden constructor to use add2.
function Geyser.TextEdit:new2(cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
