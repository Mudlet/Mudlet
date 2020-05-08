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
  if self.container and (self.container.add == Geyser.add2 or self.container.passAdd2) then
    Geyser.add2(self, window, cons)
  else
    Geyser.add(self, window, cons)
  end
  if not self.defer_updates then
    self:reposition()
  end
end

--- Responsible for placing/moving/resizing this window to the correct place/size.
-- Called on window resize events.
function Geyser.HBox:reposition()
  self.parent:reposition()

  local window_width = self:calculate_dynamic_window_size().width
  local start_x = 0
  for _, window_name in ipairs(self.windows) do
    local window = self.windowList[window_name]
    local width = window.width
    local height = window.height
    window:move(start_x, 0)
    if window.h_policy == Geyser.Dynamic then
      width = window_width * window.h_stretch_factor
    end
    if window.v_policy == Geyser.Dynamic then
      height = self:get_height()
    end
    window:resize(width, height)
    start_x = start_x + window:get_width()
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
  me:reposition()
  return me
end
