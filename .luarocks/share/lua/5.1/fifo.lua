local select , setmetatable = select , setmetatable

local function is_integer(x)
	return x % 1 == 0
end

local fifo = {}
local fifo_mt = {
	__index = fifo ;
	__newindex = function ( f , k , v )
		error("Tried to set table field in fifo")
	end ;
}

local empty_default = function ( _ ) error ( "Fifo empty" ) end

function fifo.new ( ... )
	return setmetatable({
		empty = empty_default;
		head = 1;
		tail = select("#",...);
		data = {...};
	}, fifo_mt)
end

function fifo:length ( )
	return self.tail - self.head + 1
end
fifo_mt.__len = fifo.length

-- Peek at the nth item
function fifo:peek ( n )
	n = n or 1
	assert(is_integer(n), "bad index to :peek()")

	local index = self.head - 1 + n
	if index > self.tail then
		return nil, false
	else
		return self.data[index], true
	end
end

function fifo:push ( v )
	self.tail = self.tail + 1
	self.data[self.tail] = v
end

function fifo:pop ( )
	local head , tail = self.head , self.tail
	if head > tail then return self:empty() end

	local v = self.data[head]
	self.data[head] = nil
	self.head = head + 1
	return v
end

function fifo:insert ( n , v )
	local head , tail = self.head , self.tail

	if n <= 0 or head + n > tail + 2 or not is_integer(n) then
		error("bad index to :insert()")
	end

	local p = head + n - 1
	if p <= (head + tail)/2 then
		for i = head , p do
			self.data[i- 1] = self.data[i]
		end
		self.data[p- 1] = v
		self.head = head - 1
	else
		for i = tail , p , -1 do
			self.data[i+ 1] = self.data[i]
		end
		self.data[p] = v
		self.tail = tail + 1
	end
end

function fifo:remove ( n )
	local head , tail = self.head , self.tail

	if n <= 0 or not is_integer(n) then
		error("bad index to :remove()")
	end

	if head + n - 1 > tail then return self:empty() end

	local p = head + n - 1
	local v = self.data[p]

	if p <= (head + tail)/2 then
		for i = p , head , -1 do
			self.data[i] = self.data[i-1]
		end
		self.head = head + 1
	else
		for i = p , tail do
			self.data[i] = self.data[i+1]
		end
		self.tail = tail - 1
	end

	return v
end

function fifo:setempty ( func )
	self.empty = func
	return self
end

return fifo.new
