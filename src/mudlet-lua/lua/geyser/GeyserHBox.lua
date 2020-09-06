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
  local self_height = self:get_height()
  local self_width = self:get_width()
  -- Workaround for issue with width/height being 0 at creation
  self_height = self_height <= 0 and 0.9 or self_height
  self_width = self_width <= 0 and 0.9 or self_width

  local window_width = (self:calculate_dynamic_window_size().width / self_width) * 100
  local start_x = 0
  self.contains_fixed = false
  for _, window_name in ipairs(self.windows) do
    local window = self.windowList[window_name]
    local width = (window:get_width() / self_width) * 100
    local height = (window:get_height() / self_height) * 100
    if window.h_policy == Geyser.Fixed or window.v_policy == Geyser.Fixed then
      self.contains_fixed = true
    end
    window:move(start_x.."%", "0%")
    if window.h_policy == Geyser.Dynamic then
      width = window_width * window.h_stretch_factor
      if window.width ~= width .. "%" then
        window:resize(width .. "%", nil)
      end
    end
    if window.v_policy == Geyser.Dynamic then
      height = 100
      if window.height ~= height .. "%" then
        window:resize(nil, height .. "%")
      end
    end
    start_x = start_x + width
  end
end

function Geyser.HBox:reposition()
  Geyser.Container.reposition(self)
  if self.contains_fixed then
    self:organize()
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
