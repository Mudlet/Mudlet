Geyser.HBox = Geyser.Container:new({
      name = "HBoxClass"
   })

function Geyser.HBox:add (window, cons)
   Geyser.add(self, window, cons)
   self:reposition()
end
   
function Geyser.HBox:calculate_dynamic_window_width()
   local total_count = #self.windows
   local fixed_count = 0
   local fixed_width_sum  = 0
   if total_count <= 1 then
      --If there is only one window it can have all the width, if there are none then it doesn't matter
      return self:get_width()
   end
   for _, window_name in ipairs(self.windows) do
      window = self.windowList[window_name]
      if window.horizontal_policy == Geyser.Fixed then
         fixed_count = fixed_count + 1
         fixed_width_sum = fixed_width_sum + window:get_width()
      end
   end
   return (self:get_width() - fixed_width_sum) / (total_count - fixed_count)
end
   
--- Responsible for placing/moving/resizing this window to the correct place/size.
-- Called on window resize events.
function Geyser.HBox:reposition()
   local window_width = self:calculate_dynamic_window_width()
   start_x = 0
   for _, window_name in ipairs(self.windows) do
      window = self.windowList[window_name]
      local width = nil
      local height = nil
      window:move(start_x, nil)
      if window.horizontal_policy == Geyser.Expand then
         width = window_width
      end
      if window.vertical_policy == Geyser.Expand then
         height = self.get_height()
      end
      window:resize(width, height)
      start_x = start_x + window:get_width()
   end
   self.parent:reposition()
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
   
   return me
end