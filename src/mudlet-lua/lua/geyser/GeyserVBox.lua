--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--        VBox by Beliar            --
--                                  --
--------------------------------------
Geyser.VBox = Geyser.Container:new({
  name = "VBoxClass"
})

function Geyser.VBox:add (window, cons)
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

--- Responsible for organizing the elements inside the VBox
-- Called when a new element is added
function Geyser.VBox:organize()
  self.parent:reposition()

  local window_height = (self:calculate_dynamic_window_size().height / self.get_height()) * 100
  local start_y = 0
  for _, window_name in ipairs(self.windows) do
    local window = self.windowList[window_name]
    window:move("0%", start_y.."%")
    local width = (window:get_width() / self:get_width()) * 100
    local height = (window:get_height() / self:get_height()) * 100
    if window.h_policy == Geyser.Dynamic then
      width = 100
    end
    if window.v_policy == Geyser.Dynamic then
      height = window_height * window.v_stretch_factor
    end
    window:resize(width.."%", height.."%")
    start_y = start_y + (window:get_height() / self:get_height()) * 100
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
