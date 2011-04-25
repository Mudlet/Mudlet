--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a gauge that can be either vertical or horizontal.
-- @class table
-- @name Geyser.Gauge
-- @field value Percentage value of how "full" the gauge is.
-- @field orientation "horizontal" is the default and creates a horizontal
--                    gauge that fills from left to right. "vertical" creates
--                    a gauge that fills from bottom to top. "goofy" is
--                    horizontal but fills right to left. "batty" is
--                    vertical but fills from top to bottom.
-- @field color Color base for this gauge.  Default is #808080
Geyser.Gauge = Geyser.Container:new({
      name = "GaugeClass",
      value = 100, -- ranges from 0 to 100
      color = "#808080",
      orientation = "horizontal"})

--- Sets the gauge amount.
-- @param currentValue Current numeric value, or if maxValue is ommitted, then
--        it is assumed that currentValue is a value between 0 and 100 and is
--        used to set the gauge.
-- @param maxValue Maximum numeric value.  Optionally nil, see above.
-- @param text The text to display on the gauge, it is optional.
function Geyser.Gauge:setValue (currentValue, maxValue, text)
   -- Use sensible defaults for missing parameters.
   if currentValue < 0 then
      currentValue = 0
   end
   if maxValue then
      self.value = currentValue/maxValue * 100
   else
      self.value = currentValue
   end

   -- Update gauge in the requested orientation
   local shift = tostring(self.value) .. "%"
   if self.orientation == "horizontal" then
      self.front:resize(shift, "100%")
   elseif self.orientation == "vertical" then
      self.front:move("0px", "-" .. shift)
      self.front:resize("100%", "-0px") -- bind to bottom container border
   elseif self.orientation == "goofy" then
      self.front:move("-" .. shift, "0px")
      self.front:resize("-0px", "100%") -- bind to right container border
   else -- batty
      self.front:resize("100%", shift)
   end
   
   if text then
      self.front:echo(text)
      self.back:echo(text)
   end
end

--- Sets the gauge color.
-- @param r The red component, or a named color like "green".
-- @param g the green component, or nil if using a named color.
-- @param b the blue component, or nil if using a named color.
-- @param text The text to display on the gauge, it is optional.
function Geyser.Gauge:setColor (r, g, b, text)
   r,g,b = Geyser.Color.parse(r,g,b)
   self.front:setColor(r,g,b)
   self.back:setColor(r,g,b,100)
   if text then
      self.front:echo(text)
      self.back:echo(text)
   end
end

--- Sets the text on the gauge.
-- @param text The text to set.
function Geyser.Gauge:setText (text)
   if text then
      self.front:echo(text)
      self.back:echo(text)
   end
end

-- Sets the style sheet for the gauge
-- @param css Style sheet for the front label
-- @param cssback Style sheet for the back label
function Geyser.Gauge:setStyleSheet(css, cssback)
        self.front:setStyleSheet(css)
        self.back:setStyleSheet(cssback or css)
end

-- Save a reference to our parent constructor
Geyser.Gauge.parent = Geyser.Container

-- Overridden constructor
function Geyser.Gauge:new (cons, container)
   -- Initiate and set gauge specific things
   cons = cons or {}
   cons.type = cons.type or "gauge"

   -- Call parent's constructor
   local me = self.parent:new(cons, container)

   -- Set the metatable.
   setmetatable(me, self)
   self.__index = self

   -----------------------------------------------------------
   -- Now create the Gauge using primitives and tastey classes

   -- Set up the constraints for the front label, the label that changes size to
   -- indicated levels in the gauges.
   local front = Geyser.copyTable(cons)
   front.name = me.name .. "_front"
   front.color = me.color
   front.x, front.y, front.width, front.height = 0,0,"100%","100%"

   -- Set up the constraints for the back label, which is always the size of the gauge.
   local back = Geyser.copyTable(front)
   back.name = me.name .. "_back"
   local br, bg, bb = Geyser.Color.parse(me.color)
   back.color = Geyser.Color.hexa(br,bg,bb,100)

   -- Create back first so that the labels are stacked correctly.
   me.back = Geyser.Label:new(back, me)
   me.front = Geyser.Label:new(front, me)
   
   --print("  New in " .. self.name .. " : " .. me.name)
   return me
end

