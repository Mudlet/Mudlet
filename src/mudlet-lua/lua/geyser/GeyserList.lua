--- A scrolling vertical list. Anything created in it, a Geyser.ListItem or any
-- other Geyser window, joins the list's content as a row. Rows keep their own
-- height, and the content is kept as tall as they add up to. Once that is taller
-- than the list, the list scrolls.
--
-- Mudlet has no call to scroll a scroll box or style its scrollbars, so neither can
-- a list.
--
-- `begin_update()` and `end_update()` hold back the layout, for adding or removing
-- many rows at once. Rows are ordered as they were added unless the list has a
-- `sortBy`. Clicking a Geyser.ListItem selects it if the list's `selectionMode` lets
-- it, and `onSelectionChange` is told, which is the place to show that it is.
--
-- `stripes` is a list of stylesheets for Geyser.ListItems to take in turn by position,
-- two of them for zebra striping. They are the list's to give out, as a row's
-- position changes whenever another row comes or goes. A stylesheet set on the item
-- goes after its stripe, so it wins wherever the two disagree.
-- @author gesslar
-- @module Geyser.List

--- Represents a scrolling vertical list.
-- @field scrollbarWidth Pixels kept clear for the vertical scrollbar, while there is one. Default is 20.
-- @field frameWidth Pixels the scroll box's frame takes from the room inside it, edges together. A box reports its outer size, so content that size would be too big for it. Default is 4.
-- @field selectionMode How much of the list can be selected: "none", "single" or "multi". Default is "none".
-- @field sortBy A function the list keeps itself in order by, given two rows and saying whether the first goes first.
-- @field stripes A list of stylesheets for rows to take in turn by position.
-- @field onSelectionChange A function called with the list, the row, and whether it is now selected.
-- @field scrollBox The Geyser.ScrollBox the list is built on.
-- @field content The Geyser.VBox holding the rows.
Geyser.List = Geyser.Container:new({
  name = "ListClass",
  scrollbarWidth = 20,
  frameWidth = 4,
  selectionMode = "none",
})

Geyser.List.parent = Geyser.Container

-- Internal function: the width constraint for stopping short of the right edge by some pixels. A
-- constraint of -0 would be read as no width at all.
-- @param pixels How far short to stop.
-- @return The width constraint.
local function widthInset(pixels)
  return pixels > 0 and -pixels or "100%"
end

-- Internal function: keeps the content clear of the vertical scrollbar while there is one, so the
-- scrollbar never makes it too wide and brings in a horizontal one, and gives it
-- the whole width while there isn't. Mudlet has no call to ask a scroll box about
-- its scrollbars, or to turn them off, but the vertical one is only there when
-- the content is taller than the box.
-- @param list The list to settle.
-- @return Whether the content's width changed.
local function settleWidth(list)
  local room = list.scrollBox:get_height() - list.frameWidth
  local overflowing = list.content:get_height() > room

  if overflowing == list.overflowing then return false end
  list.overflowing = overflowing

  list.content:resize(widthInset(overflowing and list.scrollbarWidth or list.frameWidth), nil)

  return true
end

-- Internal function: whether layouts are being held back, by the list, by Geyser as a whole, or by
-- the content, which does so while it is being deleted.
-- @param list The list to ask about.
-- @return Whether layouts are being held back.
local function deferring(list)
  return list.defer_updates or list.content.defer_updates
end

-- Internal function: the name of a row, given the row or its name.
-- @param item The row, or its name.
-- @return The name.
local function nameOf(item)
  if type(item) == "table" then
    return item.name
  end

  return item
end

-- Internal function: sets a row's selected state and tells `onSelectionChange`, if that changes it.
-- @param list The list.
-- @param name The row's name.
-- @param selected Whether it is to be selected.
-- @return Whether that was a change.
local function setSelected(list, name, selected)
  if (list.selected[name] or false) == selected then return false end

  list.selected[name] = selected or nil

  if list.onSelectionChange then
    list.onSelectionChange(list, list.content.windowList[name], selected)
  end

  return true
end

-- Internal function: puts the rows in order, leaving the layout to the caller.
-- @param list The list.
-- @param[opt] comparator Given two rows, says whether the first goes first. Without one the `sortBy` is used, or failing that the rows' names.
local function sortRows(list, comparator)
  local box = list.content
  local compare = comparator or list.sortBy
  local byName

  if compare then
    byName = function(a, b)
      return compare(box.windowList[a], box.windowList[b])
    end
  end

  table.sort(box.windows, byName)
  list.pendingSort = nil
end

-- Internal function: lays out the rows as they stand, or leaves that for when layouts aren't being
-- held back.
-- @param list The list.
local function organize(list)
  if deferring(list) then
    list.pendingFit = true
    return
  end

  list.content:organize()
end

-- Internal function: takes in a row that has just come in, and lays the list out.
-- @param list The list.
-- @param window The row.
local function rowAdded(list, window)
  -- Only a built Geyser.ListItem has this, and one being built connects itself once it
  -- is, so this is one moving in from elsewhere.
  if window.setStripe then
    list:connectItem(window)
  end

  if deferring(list) then
    list.pendingSort = list.sortBy ~= nil
    list.pendingFit = true
    return
  end

  if list.sortBy then
    sortRows(list)
  end

  list:fit()
end

--- Creates a new list.
-- @param[opt] cons Constraints, as for `Geyser.Container:new`, plus `scrollbarWidth`, `frameWidth`, `sortBy`, `selectionMode`, `onSelectionChange` and `stripes`.
-- @param[opt] container The Geyser container to put the list in.
-- @return The new list, or nil if a name was refused.
-- @return Why the name was refused.
function Geyser.List:new(cons, container)
  cons = cons or {}
  cons.type = cons.type or "list"
  cons.name = cons.name or Geyser.nameGen("list")

  local me, refused = self.parent:new(cons, container)
  if not me then
    return nil, refused
  end

  setmetatable(me, self)
  self.__index = self

  -- With new2, add2 gave the window its own add while it was still a plain
  -- container, which would hide the one here. It knows to leave a VBox or HBox
  -- alone, by type, but has no way to know about this one.
  me.add = nil

  me.selected = {}

  -- Until `content` is set below, add() puts these in the list itself.
  local scrollBox, taken = Geyser.ScrollBox:new({
    name = me.name .. ".ScrollBox",
    x = 0,
    y = 0,
    width = "100%",
    height = "100%",
  }, me)

  if not scrollBox then
    me:delete()
    return nil, taken
  end

  me.scrollBox = scrollBox

  -- Mudlet's scroll box starts its content at the size of its parent window
  -- and only re-fits it when a child moves, resizes, shows or hides. A Geyser
  -- container has no widget to trigger that, so this invisible label does,
  -- and keeps the content from being left at the old size once the last item
  -- is deleted.
  local spacer
  spacer, taken = Geyser.Label:new({
    name = me.name .. ".ScrollSpacer",
    x = 0,
    y = 0,
    width = 1,
    height = 1,
  }, me.scrollBox)

  if not spacer then
    me:delete()
    return nil, taken
  end

  spacer:setStyleSheet("background-color: transparent;")
  spacer:enableClickthrough()

  -- Sized to its items by fit() rather than to the scroll box, so that the
  -- scroll box gets something taller than itself to scroll.
  local content = Geyser.VBox:new({
    name = me.name .. ".Content",
    x = 0,
    y = 0,
    width = widthInset(me.frameWidth),
    height = 1,
  }, me.scrollBox)

  content.list = me

  -- Every change of order or of who is here ends in a layout.
  function content:organize()
    Geyser.VBox.organize(self)
    me:restripe()
  end

  -- A fit that is waiting lays the rows out itself, right after this.
  function content:reposition(skipChildren)
    if me.pendingFit then
      return Geyser.Container.reposition(self, skipChildren)
    end

    Geyser.VBox.reposition(self, skipChildren)
  end

  -- Every way into the list comes through these, changeContainer included. They
  -- skip the VBox's own layout, as rowAdded() lays the rows out anyway.
  function content:add(window, cons)
    window.v_policy = Geyser.Fixed

    if self.useAdd2 then
      Geyser.add2(self, window, cons)
    else
      Geyser.add(self, window, cons)
    end

    rowAdded(me, window)
  end

  function content:add2(window, cons, passAdd2, exclude)
    window.v_policy = Geyser.Fixed
    Geyser.add2(self, window, cons, passAdd2, exclude)
    rowAdded(me, window)
  end

  -- Every way out of the list comes through here: delete, changeContainer, and
  -- adding the item to another container.
  function content:remove(window)
    me.selected[window.name] = nil

    -- The stripe was this list's; a row going elsewhere leaves it behind.
    if window.setStripe and not window.deleted then
      window:setStripe(nil)
    end

    Geyser.remove(self, window)
    me:fit()
  end

  me.content = content
  me.childContainer = content

  return me
end

--- Overridden constructor to use add2.
-- @param[opt] cons Constraints, as for `Geyser.List:new`.
-- @param[opt] container The Geyser container to put the list in.
-- @return The new list, or nil if a name was refused.
-- @return Why the name was refused.
function Geyser.List:new2(cons, container)
  cons = cons or {}
  cons.useAdd2 = true

  return self:new(cons, container)
end

--- Adds a window to the list as a row, at the end unless the list has a `sortBy`.
-- A row keeps its own height rather than sharing out the list's. An item's
-- constructor adds it before it is fully built, so a `sortBy` can rely on what the
-- item was given in its constraints, but not on anything that comes after.
-- @param window The window to add.
-- @param[opt] cons Constraints for the window.
function Geyser.List:add(window, cons)
  if self.useAdd2 then
    return self:add2(window, cons)
  end

  if not self.content then
    return Geyser.add(self, window, cons)
  end

  self.content:add(window, cons)
end

--- Adds a window to the list as a row, by way of `Geyser:add2`. See `add`.
-- @param window The window to add.
-- @param[opt] cons Constraints for the window.
-- @param[opt] passAdd2 Whether the window's children inherit add2.
-- @param[opt] exclude Window types that keep their own add.
function Geyser.List:add2(window, cons, passAdd2, exclude)
  if not self.content then
    return Geyser.add2(self, window, cons, passAdd2, exclude)
  end

  self.content:add2(window, cons, passAdd2, exclude)
end

--- Creates a Geyser.ListItem in the list.
-- @param[opt] cons Constraints, as for `Geyser.ListItem:new`.
-- @return The new item, or nil if its name was refused.
-- @return Why the name was refused.
function Geyser.List:addItem(cons)
  return Geyser.ListItem:new(cons, self)
end

--- Creates a Geyser.ListItem at a position in the list. A list with a `sortBy` puts
-- it where that says instead.
-- @param[opt] cons Constraints, as for `Geyser.ListItem:new`.
-- @param index The position to put it at, from 1.
-- @return The new item, or nil if its name was refused.
-- @return Why the name was refused.
function Geyser.List:insertItem(cons, index)
  local item, taken = self:addItem(cons)

  if item and not self.sortBy then
    self:moveItem(item, index)
  end

  return item, taken
end

--- Moves a row to a position in the list. A list with a `sortBy` keeps its own
-- order, and refuses.
-- @param item The row, or its name.
-- @param index The position to move it to, from 1. Past either end means that end.
-- @return True, or nil and why not.
-- @return Why it wasn't moved.
function Geyser.List:moveItem(item, index)
  if self.sortBy then
    return nil, "the list is ordered by its sortBy"
  end

  local from = self:indexOf(item)
  if not from then
    return nil, "that isn't in this list"
  end

  local windows = self.content.windows
  local name = table.remove(windows, from)

  table.insert(windows, math.max(1, math.min(index, #windows + 1)), name)
  organize(self)

  return true
end

--- The rows of the list, in order. The table is a copy, so deleting rows while going
-- through it is fine.
-- @return A table of the rows.
function Geyser.List:getItems()
  local items = {}

  for i, name in ipairs(self.content.windows) do
    items[i] = self.content.windowList[name]
  end

  return items
end

--- A row of the list.
-- @param index The row's position, from 1, or its name.
-- @return The row, or nil if there is no such row.
function Geyser.List:getItem(index)
  if type(index) == "number" then
    index = self.content.windows[index]
  end

  return index and self.content.windowList[index] or nil
end

--- The number of rows in the list.
-- @return The number of rows.
function Geyser.List:getItemCount()
  return #self.content.windows
end

--- A row's position in the list.
-- @param item The row, or its name.
-- @return The position, from 1, or nil if it isn't in this list.
function Geyser.List:indexOf(item)
  return table.index_of(self.content.windows, nameOf(item))
end

--- Deletes every row now, without consulting any `onDismiss`, and without telling
-- `onSelectionChange` about the selection going with them.
function Geyser.List:clear()
  -- rawget: what it inherits from Geyser is Geyser's to end.
  local wasDeferring = rawget(self, "defer_updates")

  self:begin_update()

  for _, item in ipairs(self:getItems()) do
    item:delete()
  end

  if not wasDeferring then
    self:end_update()
  end
end

--- Dismisses every row: `dismiss()` for those that have it, so that an `onDismiss`
-- gets its say, and `delete()` for those that don't.
function Geyser.List:dismissAll()
  -- rawget: what it inherits from Geyser is Geyser's to end.
  local wasDeferring = rawget(self, "defer_updates")

  self:begin_update()

  for _, item in ipairs(self:getItems()) do
    if item.dismiss then
      item:dismiss()
    else
      item:delete()
    end
  end

  if not wasDeferring then
    self:end_update()
  end
end

--- Sorts the list and lays it out again.
-- @param[opt] comparator Given two rows, says whether the first goes first. It is used this once, and without one the `sortBy` is used, or failing that the rows' names.
function Geyser.List:sort(comparator)
  sortRows(self, comparator)
  organize(self)
end

--- Sets the order the list keeps itself in from now on, and sorts it.
-- @param[opt] comparator Given two rows, says whether the first goes first. Without one, the list stops keeping an order, and the rows stay as they are.
function Geyser.List:setSortBy(comparator)
  self.sortBy = comparator

  if comparator then
    self:sort()
  else
    self.pendingSort = nil
  end
end

--- Resizes the content to the total height of the rows and lays them out again.
-- Adding and removing rows does this already, and so does a Geyser.ListItem
-- changing height; it is for any other row whose height has changed. While layouts
-- are held back it waits until they aren't.
function Geyser.List:fit()
  local box = self.content

  if deferring(self) then
    self.pendingFit = true
    return
  end

  self.pendingFit = nil

  local height = 0

  for _, name in ipairs(box.windows) do
    height = height + box.windowList[name]:get_height()
  end

  height = math.max(height, 1)

  -- Not resize(), which places every row only for organize() to place them again.
  if height ~= box:get_height() then
    box.height = height
    Geyser.calc_constraints(box, box, box.container)
  end

  settleWidth(self)
  box:organize()
end

--- Gives each Geyser.ListItem its stripe for its current position. The list does this
-- itself whenever it lays out, and Geyser.ListItem calls it once its label exists.
function Geyser.List:restripe()
  local stripes = self.stripes

  if not stripes or not self.content then return end

  for i, item in ipairs(self:getItems()) do
    if item.setStripe then
      item:setStripe(stripes[(i - 1) % #stripes + 1])
    end
  end
end

--- Sets the stylesheets rows take in turn by position.
-- @param[opt] stripes The stylesheets. Without any, the rows lose their stripes.
function Geyser.List:setStripes(stripes)
  self.stripes = stripes

  if stripes then
    self:restripe()
    return
  end

  for _, item in ipairs(self:getItems()) do
    if item.setStripe then
      item:setStripe(nil)
    end
  end
end

--- Sets the pixels kept clear for the vertical scrollbar, while there is one.
-- @param width The width in pixels.
function Geyser.List:setScrollbarWidth(width)
  self.scrollbarWidth = width
  self.overflowing = nil
  self:fit()
end

--- Sets the pixels the scroll box's frame takes from the room inside it.
-- @param width The width in pixels, edges together.
function Geyser.List:setFrameWidth(width)
  self.frameWidth = width
  self.overflowing = nil
  self:fit()
end

--- Starts holding back the layout. Rows can come and go without the list being laid
-- out for each one.
function Geyser.List:begin_update()
  Geyser.begin_update(self)
  self.content.defer_updates = true
end

--- Stops holding back the layout, and catches up on it.
function Geyser.List:end_update()
  -- Not false: without its own value it goes back to following Geyser's.
  self.content.defer_updates = nil
  Geyser.end_update(self)
end

--- Places the list, as `Geyser.Container:reposition` does, and catches up on any
-- layout that was held back. Resizing the list can start or end the need for a
-- scrollbar without any row coming or going.
-- @param[opt] skipChildren Place only this window.
function Geyser.List:reposition(skipChildren)
  Geyser.Container.reposition(self, skipChildren)

  if not self.content or deferring(self) then return end

  if self.pendingSort then
    self:sort()
  end

  if self.pendingFit then
    self:fit()
  elseif settleWidth(self) then
    self.content:organize()
  end
end

--- Makes clicking an item select it, unless it has a click callback of its own,
-- which can call `toggle` or `select` itself. Geyser.ListItem calls this once its
-- label exists; other windows have to do their own clicking.
-- @param item The item.
function Geyser.List:connectItem(item)
  -- rawget: one an item built from another inherits is that other's.
  if self.selectionMode == "none" or rawget(item, "clickCallback") then return end

  item:setClickCallback(function()
    -- A Geyser.ListItem can have moved to another list since.
    local list = self

    if item.getList then
      list = item:getList()
    end

    if not list then return end

    if list.selectionMode == "multi" then
      list:toggle(item)
    elseif list.selectionMode == "single" then
      list:select(item)
    end
  end)
end

--- Sets how much of the list can be selected. Going to "none" clears the selection,
-- and going to "single" keeps only the first of it.
-- @param mode "none", "single" or "multi".
-- @return True, or nil and why not.
-- @return Why it wasn't set.
function Geyser.List:setSelectionMode(mode)
  if mode ~= "none" and mode ~= "single" and mode ~= "multi" then
    return nil, "selection mode must be none, single or multi"
  end

  self.selectionMode = mode

  local kept = false

  for _, item in ipairs(self:getSelected()) do
    if mode == "none" or (mode == "single" and kept) then
      setSelected(self, item.name, false)
    end

    kept = true
  end

  for _, item in ipairs(self:getItems()) do
    if item.setStripe then
      self:connectItem(item)
    end
  end

  return true
end

--- Selects a row. In "single" mode, that deselects whichever row was selected.
-- @param item The row, or its name.
-- @return True, or nil and why not.
-- @return Why it wasn't selected.
function Geyser.List:select(item)
  if self.selectionMode == "none" then
    return nil, "the list's selection mode is none"
  end

  if not self:indexOf(item) then
    return nil, "that isn't in this list"
  end

  local name = nameOf(item)

  if self.selectionMode == "single" then
    -- A copy, as `onSelectionChange` is free to change the selection.
    for _, other in ipairs(self:getSelected()) do
      if other.name ~= name then
        setSelected(self, other.name, false)
      end
    end

    -- Those calls can take the row away, or select another, which then stands.
    if not self:indexOf(name) then
      return nil, "that isn't in this list"
    end

    for other in pairs(self.selected) do
      if other ~= name then
        return nil, "onSelectionChange selected another row"
      end
    end
  end

  setSelected(self, name, true)

  return true
end

--- Deselects a row.
-- @param item The row, or its name.
function Geyser.List:deselect(item)
  setSelected(self, nameOf(item), false)
end

--- Selects a row if it isn't selected, and deselects it if it is.
-- @param item The row, or its name.
-- @return Whether it now is, or nil and why not.
-- @return Why it couldn't be selected.
function Geyser.List:toggle(item)
  if self:isSelected(item) then
    self:deselect(item)
    return false
  end

  return self:select(item)
end

--- Deselects every row.
function Geyser.List:clearSelection()
  for _, item in ipairs(self:getSelected()) do
    setSelected(self, item.name, false)
  end
end

--- Whether a row is selected.
-- @param item The row, or its name.
-- @return Whether it is selected.
function Geyser.List:isSelected(item)
  return self.selected[nameOf(item)] == true
end

--- The selected rows, in list order.
-- @return A table of the selected rows.
function Geyser.List:getSelected()
  local items = {}

  for _, item in ipairs(self:getItems()) do
    if self.selected[item.name] then
      items[#items + 1] = item
    end
  end

  return items
end
