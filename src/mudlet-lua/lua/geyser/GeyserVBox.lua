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
  -- Workaround for issue with width/height being 0 at creation
  if self:get_width() == 0 then
    self:resize("0.9px", nil)
  end
  if self:get_height() == 0 then
    self:resize(nil, "0.9px")
  end
  local window_height = (self:calculate_dynamic_window_size().height / self:get_height()) * 100
  local start_y = 0
  self.contains_fixed = false
  for _, window_name in ipairs(self.windows) do
    local window = self.windowList[window_name]
    window:move("0%", start_y.."%")
    local width = (window:get_width() / self:get_width()) * 100
    local height = (window:get_height() / self:get_height()) * 100
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

function Geyser.VBox:reposition()
  Geyser.Container.reposition(self)
  if self.contains_fixed then -- prevent gaps when items have fixed size
    self:organize()
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
