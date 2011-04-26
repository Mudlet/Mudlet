Geyser.VBox = Geyser.Box:new({
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
	for _, layout in ipairs(self.windows) do
		if layout.policy == self.Fixed then
			fixed_count = fixed_count + 1
			fixed_height_sum = fixed_height_sum + layout.window.get_height()
		end
	end
	return (self:get_height() - fixed_height_sum) / (total_count - fixed_count)
end
	
function Geyser.VBox:resize_windows()
	local windows = self.windows
	if not windows then return end
	local window_height = self:calculate_dynamic_window_height()
	start_y = 0
	for _, layout in ipairs(windows) do
		layout.window:move(0, start_x)
		if Geyser.policy == self.Expand then
			layout.window:resize(self.get_width(), window_height)
		end
		layout.window:reposition()
		start_y = start_y + layout.window.getHeight()
	end
end

Geyser.VBox.parent = Geyser.Box

function Geyser.VBox:new(cons, container)
	-- Initiate and set Window specific things
	cons = cons or {}
	cons.type = cons.type or "VBox"
	
	-- Call parent's constructor
	local me = self.parent:new(cons, container)
	setmetatable(me, self)
	self.__index = self
	
	me.windows = {}
	return me
end