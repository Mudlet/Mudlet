--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a gauge that can be either vertical or horizontal.
-- @class table
-- @name Geyser.Gauge
-- @field value Percentage value of how "full" the gauge is.
-- @field strict If true, will cap the value of the gauge at 100, preventing
--               it from overflowing the edge. Defaults to false to maintain
--               old behaviours from before this was added.
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
  strict = false,
  orientation = "horizontal" })

--- Sets the gauge amount.
-- @param currentValue Current numeric value, or if maxValue is ommitted, then
--        it is assumed that currentValue is a value between 0 and 100 and is
--        used to set the gauge.
-- @param maxValue Maximum numeric value.  Optionally nil, see above.
-- @param text The text to display on the gauge, it is optional.
function Geyser.Gauge:setValue (currentValue, maxValue, text)
  assert(type(currentValue) == "number", string.format("bad argument #1 type (currentValue as number expected, got %s!)", type(currentValue)))
  assert(maxValue == nil or type(maxValue) == "number", string.format("bad argument #2 type (optional maxValue as number expected, got %s!)", type(maxValue)))
  -- Use sensible defaults for missing parameters.
  if currentValue < 0 then
    currentValue = 0
  end
  if maxValue then
    self.value = currentValue / maxValue * 100
  else
    self.value = currentValue
  end
-- prevent the gauge from overflowing its borders if currentValue > maxValue if gauge is set to be strict
  if self.strict and self.value > 100 then self.value = 100 end
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
    self.text:echo(text)
  end
end

--- Sets the gauge color.
-- @param r The red component, or a named color like "green".
-- @param g the green component, or nil if using a named color.
-- @param b the blue component, or nil if using a named color.
-- @param text The text to display on the gauge, it is optional.
function Geyser.Gauge:setColor (r, g, b, text)
  r, g, b = Geyser.Color.parse(r, g, b)
  self.front:setColor(r, g, b)
  self.back:setColor(r, g, b, 100)
  if text then
    self.text:echo(text)
  end
end

--- Sets the text on the gauge.
-- @param text The text to set.
function Geyser.Gauge:setText (text)
  if text then
    self.text:echo(text)
  end
end

--- Set the format for text on the gauge
-- @param format the format to set. Same as Geyser.Label:setFormat
function Geyser.Gauge:setFormat(format)
  self.text:setFormat(format)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Set whether or not the text in the gauge should be bold
-- @param bool True for bold
function Geyser.Gauge:setBold(bool)
  self.text:setBold(bool)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Set whether or not the text in the gauge should be italic
-- @param bool True for bold
function Geyser.Gauge:setItalics(bool)
  self.text:setItalics(bool)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Set whether or not the text in the gauge should be underlined
-- @param bool True for underlined
function Geyser.Gauge:setUnderline(bool)
  self.text:setUnderline(bool)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Set whether or not the text in the gauge should be strikethrough
-- @param bool True for strikethrough
function Geyser.Gauge:setStrikethrough(bool)
  self.text:setStrikethrough(bool)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Set the font size for the gauge to use
-- @param fontSize the font size to use for the gauge. Should be a number
function Geyser.Gauge:setFontSize(fontSize)
  self.text:setFontSize(fontSize)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Set the alignment of the text on the gauge
-- @param alignment Valid alignments are 'c', 'center', 'l', 'left', 'r', 'right', or '' to not include the alignment as part of the echo
function Geyser.Gauge:setAlignment(alignment)
  self.text:setAlignment(alignment)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Sets the color of the text on the gauge
-- @param color the color you want the text to be
function Geyser.Gauge:setFgColor(color)
  self.text:setFgColor(color)
end

--- Sets the text on the gauge, overwrites inherited echo function.
-- @param text The text to set.
function Geyser.Gauge:echo(message, color, format)
  self.text:echo(message, color, format)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

-- Sets the style sheet for the gauge
-- @param css Style sheet for the front label
-- @param cssback Style sheet for the back label
-- @param cssText Style sheet for the text label
function Geyser.Gauge:setStyleSheet(css, cssback, cssText)
  self.front:setStyleSheet(css)
  self.back:setStyleSheet(cssback or css)
  if cssText ~= nil then
    self.text:setStyleSheet(cssText)
  end
end

--- Sets the gauge to no longer intercept mouse events
function Geyser.Gauge:enableClickthrough()
    self.front:enableClickthrough()
    self.back:enableClickthrough()
    self.text:enableClickthrough()
end

--- Sets the gauge to once again intercept mouse events
function Geyser.Gauge:disableClickthrough()
    self.front:disableClickthrough()
    self.back:disableClickthrough()
    self.text:disableClickthrough()
end

--- Sets the tooltip of the gauge
-- @param txt the tooltip txt
-- @param duration the duration of the tooltip
function Geyser.Gauge:setToolTip(txt, duration)
  duration = duration or 0
  self.text:setToolTip(txt, duration)
end

--- Resets the tooltip of the gauge
function Geyser.Gauge:resetToolTip()
  self.text:resetToolTip()
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
  me.windowname = me.windowname or me.container.windowname or "main"
  -----------------------------------------------------------
  -- Now create the Gauge using primitives and tastey classes

  -- Set up the constraints for the front label, the label that changes size to
  -- indicated levels in the gauges. Message set to nil to avoid unwanted text
  local front = Geyser.copyTable(cons)
  front.name = me.name .. "_front"
  front.color = me.color
  front.message = nil
  front.x, front.y, front.width, front.height = 0, 0, "100%", "100%"

  -- Set up the constraints for the back label, which is always the size of the gauge.
  -- Message set to nil to avoid unwanted text
  local back = Geyser.copyTable(front)
  back.name = me.name .. "_back"
  local br, bg, bb = Geyser.Color.parse(me.color)
  back.color = Geyser.Color.hexa(br, bg, bb, 100)
  back.message = nil

  -- Set up the constraints for the text label, which is also always the size of the gauge.
  -- We also set this label's color to 0,0,0,0 so it's black and full transparent.
  local text = Geyser.copyTable(front)
  text.name = me.name .. "_text"
  text.fillBg = 0
  text.color = Geyser.Color.hexa(0, 0, 0, 0)



  -- Create back first so that the labels are stacked correctly.
  me.back = Geyser.Label:new(back, me)
  me.front = Geyser.Label:new(front, me)
  me.text = Geyser.Label:new(text, me)
  me.format = me.text.format
  me.formatTable = me.text.formatTable

  -- Set whether this gauge is strict about its max value being 100 or not
  if cons.strict then me.strict = true else me.strict = false end

  -- Set clickthrough if included in constructor
  if cons.clickthrough then me:enableClickthrough() end

  -- Echo text to the text label if 'message' constraint is set
  if cons.message then me:echo(me.message) end
  
  --print("  New in " .. self.name .. " : " .. me.name)
  return me
end
