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
  Geyser.add(self, window, cons)
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
