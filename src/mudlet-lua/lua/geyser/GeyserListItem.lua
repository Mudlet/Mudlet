--- A single row of a Geyser.List: a Geyser.Label that is one line tall unless
-- told otherwise, and that lets its owner have a say in how it arrives and goes.
--
-- `delete()` is immediate, because a container deleting its children counts on
-- them being gone when it returns. `dismiss()` is the one that can take its time.
-- @author gesslar
-- @module Geyser.ListItem

--- Represents a single row of a Geyser.List.
-- @field onInsert A function called with the item once it is built.
-- @field onDismiss A function `dismiss()` calls with the item and a `done` function, which deletes the item.
-- @field deleted True once the item has been deleted, for anything still working on it to check.
Geyser.ListItem = {
  name = "ListItemClass",
}
Geyser.ListItem.parent = Geyser.Label
setmetatable(Geyser.ListItem, Geyser.Label)
Geyser.ListItem.__index = Geyser.ListItem

--- Creates a new list item. Unless `cons` says otherwise, it is a fixed 1
-- character tall, so that a list stacks items from the top instead of
-- stretching them to fill it. Returns the item, or nil and why if its name was
-- refused.
--
-- `onInsert` is called with the item once it is built, so a `message` or
-- `stylesheet` it needs should come in `cons` too. `onDismiss` is called with the
-- item and a `done` function by `dismiss()`.
-- @param[opt] cons Constraints, as for `Geyser.Label:new`, plus `onInsert` and `onDismiss`.
-- @param[opt] container The Geyser.List, or other container, to put it in.
function Geyser.ListItem:new(cons, container)
  cons = cons or {}
  cons.type = cons.type or "listitem"
  cons.height = cons.height or "1c"
  cons.v_policy = cons.v_policy or Geyser.Fixed

  local me, taken = self.parent:new(cons, container)
  if not me then
    return nil, taken
  end

  setmetatable(me, self)

  me.ownStyleSheet = me.stylesheet

  local list = me:getList()
  if list then
    list:connectItem(me)
    list:restripe()
  end

  if me.onInsert then
    me:onInsert()
  end

  return me
end

--- Overridden constructor to use add2. Returns what `new` does.
-- @param[opt] cons Constraints, as for `Geyser.ListItem:new`.
-- @param[opt] container The Geyser.List, or other container, to put it in.
function Geyser.ListItem:new2(cons, container)
  cons = cons or {}
  cons.useAdd2 = true

  return self:new(cons, container)
end

--- The Geyser.List the item is in.
-- @return The list, or nil if it isn't in one.
function Geyser.ListItem:getList()
  return self.container and self.container.list
end

--- The item's position in its list.
-- @return The position, from 1, or nil if it isn't in a list.
function Geyser.ListItem:getIndex()
  local list = self:getList()

  return list and list:indexOf(self)
end

--- Moves the item to a position in its list. See `Geyser.List:moveItem`.
-- @param index The position to move it to, from 1.
-- @return True, or nil and why not.
-- @return Why it wasn't moved.
function Geyser.ListItem:moveTo(index)
  local list = self:getList()
  if not list then
    return nil, "the item isn't in a list"
  end

  return list:moveItem(self, index)
end

--- Whether the item is selected in its list.
-- @return Whether it is selected.
function Geyser.ListItem:isSelected()
  local list = self:getList()

  return list ~= nil and list:isSelected(self)
end

--- Sets what is called with the item once it is built. Only a constraint can be in
-- time for its own item, so this is for a prototype to pass on.
-- @param[opt] callback Called with the item.
function Geyser.ListItem:setOnInsert(callback)
  self.onInsert = callback
end

--- Sets what `dismiss()` consults.
-- @param[opt] callback Called with the item and a `done` function, which deletes the item.
function Geyser.ListItem:setOnDismiss(callback)
  self.onDismiss = callback
end

--- Sets the item's stylesheet, as `Geyser.Label:setStyleSheet` does, keeping any
-- stripe its list has given it underneath. `stylesheet` holds the two together.
-- @param[opt] css The Qt stylesheet to apply. Without one, the current one is applied again.
function Geyser.ListItem:setStyleSheet(css)
  self.ownStyleSheet = css or self.ownStyleSheet

  local stripe = self.stripe or ""
  if stripe ~= "" and not stripe:match(";%s*$") then
    stripe = stripe .. ";"
  end

  return Geyser.Label.setStyleSheet(self, stripe .. (self.ownStyleSheet or ""))
end

--- Sets the stylesheet that goes under the item's own. Its list calls this.
-- @param[opt] stripe The Qt stylesheet.
function Geyser.ListItem:setStripe(stripe)
  if stripe == self.stripe then return end

  self.stripe = stripe
  self:setStyleSheet()
end

--- Prints a message to the item, as `Geyser.Label:echo` does. Does nothing when
-- asked to show the plain text it already shows.
-- @param[opt] message The message to print.
-- @param[opt] color The colour to use.
-- @param[opt] format The format list to use.
function Geyser.ListItem:echo(message, color, format)
  local unchanged = message == self.message and not self.echoType
  if unchanged and not color and not format then return end

  return Geyser.Label.echo(self, message, color, format)
end

-- Internal function: whether a flavoured echo would show what the item already shows.
-- @param item The item.
-- @param flavour "cecho", "decho" or "hecho".
-- @param[opt] message The message it is being asked to show.
-- @return Whether nothing would change.
local function showing(item, flavour, message)
  return message ~= nil and item.echoType == flavour and item.echoMessage == message
    and item.message == message
end

--- Prints a message as `Geyser.Label:cecho` does. Does nothing when asked to show
-- the text it already shows.
-- @param[opt] message The message to print.
-- @return True, or nil and an error message if the label no longer exists.
-- @return The error message.
function Geyser.ListItem:cecho(message)
  if showing(self, "cecho", message) then return true end

  return Geyser.Label.cecho(self, message)
end

--- Prints a message as `Geyser.Label:decho` does. Does nothing when asked to show
-- the text it already shows.
-- @param[opt] message The message to print.
-- @return True, or nil and an error message if the label no longer exists.
-- @return The error message.
function Geyser.ListItem:decho(message)
  if showing(self, "decho", message) then return true end

  return Geyser.Label.decho(self, message)
end

--- Prints a message as `Geyser.Label:hecho` does. Does nothing when asked to show
-- the text it already shows.
-- @param[opt] message The message to print.
-- @return True, or nil and an error message if the label no longer exists.
-- @return The error message.
function Geyser.ListItem:hecho(message)
  if showing(self, "hecho", message) then return true end

  return Geyser.Label.hecho(self, message)
end

--- Resizes the item, as `Geyser.Container:resize` does, and has its list fit a new
-- height. An auto-sized label's height comes through here too.
-- @param[opt] width The new width constraint.
-- @param[opt] height The new height constraint.
function Geyser.ListItem:resize(width, height)
  local before = self:get_height()

  Geyser.Label.resize(self, width, height)

  local list = self:getList()
  if list and self:get_height() ~= before then
    list:fit()
  end
end

--- Raises the item above other windows, as `Geyser.Container:raise` does, but keeps
-- its place in the list, which that would change for a VBox's next layout.
function Geyser.ListItem:raise()
  return Geyser.Label.raise(self, false)
end

--- Lowers the item below other windows, as `Geyser.Container:lower` does, but keeps
-- its place in the list.
function Geyser.ListItem:lower()
  return Geyser.Label.lower(self, false)
end

--- Takes the item out of its list. With an `onDismiss` constraint, that gets the
-- first say, and the item is deleted when it calls the `done` it was given.
-- Without one, the item is deleted now. Calling this again while `onDismiss` is
-- at work, or once the item is deleted, does nothing.
function Geyser.ListItem:dismiss()
  if self.dismissing or self.deleted then return end

  if not self.onDismiss then
    return self:delete()
  end

  self.dismissing = true
  self:onDismiss(function() self:delete() end)
end

--- Deletes the item now, without consulting `onDismiss`. Anything still working on
-- the item, such as an `onDismiss` animation, can check `deleted` to know to stop.
function Geyser.ListItem:delete()
  if self.deleted then return end
  self.deleted = true

  -- deleteLabel only schedules the widget's removal, which a scroll box doesn't
  -- notice; hiding it first makes the scroll box re-fit.
  self:hide()

  return Geyser.Label.delete(self)
end
