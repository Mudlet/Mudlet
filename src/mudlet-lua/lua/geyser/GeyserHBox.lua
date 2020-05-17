--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--        HBox by Beliar            --
--                                  --
--------------------------------------
Geyser.HBox = Geyser.Container:new({
  name = "HBoxClass"
})

function Geyser.HBox:add (window, cons)
  -- VBox/HBox have their own add function therefore passing off add2 should be possible without
  -- overwriting their add functions
  if self.useAdd2 then
    Geyser.add2(self, window, cons)
  else
    Geyser.add(self, window, cons)
  end
  if not self.defer_updates then
    self:organize()
  end
end

--- Responsible for organizing the elements inside the HBox
-- Called when a new element is added
function Geyser.HBox:organize()
  self.parent:reposition()
  local window_width = (self:calculate_dynamic_window_size().width / self:get_width()) * 100
  local start_x = 0
  for _, window_name in ipairs(self.windows) do
    local window = self.windowList[window_name]
    local width = (window:get_width() / self:get_width()) * 100
    local height = (window:get_height() / self:get_height()) * 100
    window:move(start_x.."%", "0%")
    if window.h_policy == Geyser.Dynamic then
      width = window_width * window.h_stretch_factor
    end
    if window.v_policy == Geyser.Dynamic then
      height = 100
    end
    window:resize(width.."%", height.."%")
    start_x = start_x + (window:get_width() / self:get_width()) * 100
  end
end

Geyser.HBox.parent = Geyser.Container

function Geyser.HBox:new(cons, container)
  -- Initiate and set Window specific things
  cons = cons or {}
  cons.type = cons.type or "hbox"

  -- Call parent's constructor
  local me = self.parent:new(cons, container)
  setmetatable(me, self)
  self.__index = self
  me:organize()
  return me
end

--- Overridden constructor to use add2
function Geyser.HBox:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end

--- Overridden constructor to use add2
function Geyser.HBox:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
