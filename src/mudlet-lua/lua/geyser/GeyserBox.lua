Geyser.Box = Geyser.Container:new({
		name = "BoxClass"
	})

Geyser.Box.Expand, Geyser.Box.Fixed = 0, 1

function Geyser.Box:resize_windows() end

function Geyser.Box:add(window, cons, policy)
	Geyser.add(self, window, cons)
	policy = policy or self.Expand
	window:hide()
	table.insert(self.windows, {window=window, policy=policy})
	self:resize_windows()
	window:show()
end

function Geyser.Box:clear()
	self.windows = {}
end

--- Responsible for placing/moving/resizing this window to the correct place/size.
-- Called on window resize events.
function Geyser.Box:reposition ()
	self:resize_windows()
	self.parent:reposition()
end

Geyser.Box.parent = Geyser.Container

function Geyser.Box:new(cons, container)
	-- Initiate and set Window specific things
	cons = cons or {}
	cons.type = cons.type or "box"
	
-- Call parent's constructor
	local me = self.parent:new(cons, container)
	setmetatable(me, self)
	self.__index = self
	
	me.windows = {}
	return me
end