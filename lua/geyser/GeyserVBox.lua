Geyser.VBox = Geyser.Container:new({
      name = "VBoxClass"
   })

function Geyser.VBox:calculate_dynamic_window_height()
   local total_count = #self.windows
   local fixed_count = 0
   local fixed_height_sum  = 0
   if total_count <= 1 then
      --If there is only one window it can have all the height, if there are none then it doesn't matter
      return self:get_height()
   end
   for _, window_name in ipairs(self.windows) do
      window = self.windowList[window_name]
      if window.vertical_policy == Geyser.Fixed then
         fixed_count = fixed_count + 1
         fixed_height_sum = fixed_height_sum + window.get_height()
      end
   end
   return (self:get_height() - fixed_height_sum) / (total_count - fixed_count)
end

--- Responsible for placing/moving/resizing this window to the correct place/size.
-- Called on window resize events.
function Geyser.VBox:reposition()
   local window_height = self:calculate_dynamic_window_height()
   start_y = 0
   for _, window_name in ipairs(self.windows) do
      window = self.windowList[window_name]
      window:move(0, start_x)
      local width = nil
      local height = nil
      if window.horizontal_policy == Geyser.Expand then
         width = self.get_width()
      end
      if window.vertical_policy == Geyser.Expand then
         height = window_height
      end
      window:resize(width, height)
      window:reposition()
      start_y = start_y + window.get_height()
   end
   self.parent:reposition()
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