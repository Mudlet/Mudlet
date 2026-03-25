--- @module Geyser.TextEdit

Geyser.TextEdit = Geyser.Window:new({
  name = "TextEditClass"
})

Geyser.TextEdit.parent = Geyser.Window

--- @return string the current text
function Geyser.TextEdit:getText()
  return getTextEditText(self.name)
end

--- @param text the text to set
function Geyser.TextEdit:setText(text)
  setTextEditText(self.name, text)
end

function Geyser.TextEdit:clear()
  clearTextEdit(self.name)
end

--- @param bool true to make read-only, false to make editable
function Geyser.TextEdit:setReadOnly(bool)
  setTextEditReadOnly(self.name, bool)
end

--- @param text the placeholder text
function Geyser.TextEdit:setPlaceholder(text)
  setTextEditPlaceholder(self.name, text)
end

--- @param css the style sheet string
function Geyser.TextEdit:setStyleSheet(css)
  css = css or self.stylesheet
  setTextEditStyleSheet(self.name, css)
  self.stylesheet = css
end

--- @param font the font family name
function Geyser.TextEdit:setFont(font)
  setTextEditFont(self.name, font)
end

--- @param size the font size in points
function Geyser.TextEdit:setFontSize(size)
  setTextEditFontSize(self.name, size)
end

--- @param bool true to have Tab move focus, false to insert tab characters
function Geyser.TextEdit:setTabMovesFocus(bool)
  setTextEditTabMovesFocus(self.name, bool)
end

function Geyser.TextEdit:new (cons, container)
  cons = cons or {}
  cons.type = cons.type or "textEdit"

  local me = self.parent:new(cons, container)
  me.windowname = me.windowname or me.container.windowname or "main"

  setmetatable(me, self)
  self.__index = self

  createTextEdit(me.windowname, me.name, me:get_x(), me:get_y(), me:get_width(), me:get_height())
  if me.stylesheet then
    me:setStyleSheet()
  end
  -- This only has an effect if add2 is being used as for the standard add method me.hidden and me.auto_hidden is always false at creation/initialisation
  if me.hidden or me.auto_hidden then
    hideWindow(me.name)
  end

  return me
end

function Geyser.TextEdit:type_delete()
  deleteTextEdit(self.name)
end

function Geyser.TextEdit:new2(cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
