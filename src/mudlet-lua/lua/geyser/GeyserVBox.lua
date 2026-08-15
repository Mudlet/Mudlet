--- A vertical box container.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#HBox.2FVBox">Mudlet Manual</a>
-- @author guy
-- @author Beliar
-- @module Geyser.VBox

Geyser.VBox = Geyser.Container:new({
  name = "VBoxClass"
})

-- Internal function: lays the box out, or remembers that it still has to be laid
-- out when updates are being deferred, so that the reposition end_update() runs
-- picks the work up again. A box being deleted defers as well and never gets
-- that reposition, which is deliberate - it has no layout left worth doing.
-- @param box the VBox to organize
local function organizeOrDefer(box)
  if box.defer_updates then
    box.pending_organize = true
  else
    box:organize()
  end
end

function Geyser.VBox:add (window, cons)
  -- VBox/HBox have their own add function therefore passing off add2 should be possible without
  -- overwriting their add functions
  if self.useAdd2 then
    Geyser.add2(self, window, cons)
  else
    Geyser.add(self, window, cons)
  end

  organizeOrDefer(self)
end

-- add2 has to be overridden as well, otherwise children created with new2 reach
-- Geyser.add2 directly and the box never lays them out
function Geyser.VBox:add2 (window, cons, passAdd2, exclude)
  Geyser.add2(self, window, cons, passAdd2, exclude)
  organizeOrDefer(self)
end

-- The base remove only edits the bookkeeping, so without this the survivors
-- keep the geometry that was worked out for the old child count and the box is
-- left with a hole. Every removal path - delete, changeContainer, adding a
-- child to another container - comes through here.
function Geyser.VBox:remove (window)
  Geyser.remove(self, window)
  organizeOrDefer(self)
end

--- Responsible for organizing the elements inside the VBox
-- Called when an element is added or removed
function Geyser.VBox:organize()
  local self_height = self:get_height()
  local self_width = self:get_width()
  local calculated_height = self:calculate_dynamic_window_size().height
  -- Workaround for issue with width/height being 0 at creation
  self_height = self_height <= 0 and #self.windows or self_height
  self_width = self_width <= 0 and 0.9 or self_width
  calculated_height = calculated_height <= 0 and 1 or calculated_height
  
  local window_height = (calculated_height / self_height) * 100
  local start_y = 0
  self.contains_fixed = false
  for _, window_name in ipairs(self.windows) do
    local window = self.windowList[window_name]
    window:move("0%", start_y.."%")
    local width = (window:get_width() / self_width) * 100
    local height = (window:get_height() / self_height) * 100
    if window.h_policy == Geyser.Fixed or window.v_policy == Geyser.Fixed then
      self.contains_fixed = true
    end
    if window.h_policy == Geyser.Dynamic then
      width = 100
      if window.width ~= width .. "%" then
        window:resize(width .. "%", nil)
      end
    end
    if window.v_policy == Geyser.Dynamic then
      height = window_height * window.v_stretch_factor
      if window.height ~= height .. "%" then
        window:resize(nil, height .. "%")
      end
    end
    start_y = start_y + height
  end
end

function Geyser.VBox:reposition(skipChildren)
  Geyser.Container.reposition(self, skipChildren)
  -- contains_fixed prevents gaps when items have fixed size and is deliberately
  -- not deferred, pending_organize
  -- flushes a layout that was skipped while updates were deferred. Clearing it
  -- only after organize() keeps the work queued if organize() throws.
  if self.contains_fixed or (self.pending_organize and not self.defer_updates) then
    self:organize()
    self.pending_organize = nil
  end
end

Geyser.VBox.parent = Geyser.Container

function Geyser.VBox:new(cons, container)
  -- Initiate and set Window specific things
  cons = cons or {}
  cons.type = cons.type or "VBox"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)
  setmetatable(me, self)
  self.__index = self
  return me
end

--- Overridden constructor to use add2
function Geyser.VBox:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
