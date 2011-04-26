Geyser.HBox = Geyser.Box:new({
		name = "HBoxClass"
	})

function Geyser.HBox:calculate_dynamic_window_width()
	local total_count = #self.windows
	local fixed_count = 0
	local fixed_width_sum  = 0
	if total_count <= 1 then
		--If there is only one window it can have all the width, if there are none then it doesn't matter
		return self:get_width()
	end
	for _, layout in ipairs(self.windows) do
		if layout.policy == self.Fixed then
			fixed_count = fixed_count + 1
			fixed_width_sum = fixed_width_sum + layout.window:get_width()
		end
	end
	return (self:get_width() - fixed_width_sum) / (total_count - fixed_count)
end
	
function Geyser.HBox:resize_windows()
	local windows = self.windows
	if not windows then return end
	local window_width = self:calculate_dynamic_window_width()
	start_x = 0
	for _, layout in ipairs(windows) do
		layout.window:move(start_x, 0)
		if layout.policy == self.Expand then
			layout.window:resize(window_width, nil)
		end
		start_x = start_x + layout.window:get_width()
	end
end

Geyser.HBox.parent = Geyser.Box

function Geyser.HBox:new(cons, container)
	-- Initiate and set Window specific things
	cons = cons or {}
	cons.type = cons.type or "hbox"

-- Call parent's constructor
	local me = self.parent:new(cons, container)
	setmetatable(me, self)
	self.__index = self
	
	me.windows = {}
	return me
end