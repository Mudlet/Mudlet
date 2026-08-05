--- Represents a gauge that can be either vertical or horizontal.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Geyser.Gauge">Mudlet Manual</a>
-- @author guy
-- @module Geyser.Gauge

--- Represents a gauge that can be either vertical or horizontal.
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

-- Reads one CSS token as a length in pixels. Qt reads a bare number as pixels
-- and a negative length is meaningful, so both are taken; a unit that has no
-- pixel value without knowing the font or the parent - em, %, pt - has none to
-- give here, and neither has a keyword such as "solid".
-- @param token The token to read
-- @return the length in pixels, or nil if the token is not a pixel length
local function pixelLength(token)
  local number, unit = token:match("^([+-]?%d*%.?%d+)(.*)$")
  if not number then
    return nil
  end
  unit = unit:lower()
  if unit ~= "" and unit ~= "px" then
    return nil
  end
  return tonumber(number)
end

-- Finds the value of a CSS declaration. The property has to start a word of
-- its own, or "qproperty-margin" would be read as a margin.
-- @param css The CSS string to search
-- @param property The property name, as a Lua pattern
-- @return the value with its trailing whitespace removed, or nil
local function cssValue(css, property)
  local value = css:match("%f[%w%-]" .. property .. "%s*:%s*([^;]+)")
  if not value then
    return nil
  end
  return value:match("^(.-)%s*$")
end

-- Reads a one to four value CSS box shorthand into its four sides.
-- @param value The declaration value, or nil
-- @return top, right, bottom, left, or nil if any part of the value is not a
--         pixel length: half a shorthand is worse than none, because the
--         lengths that are left would be read in the wrong positions
local function boxShorthand(value)
  if not value then
    return nil
  end
  local lengths = {}
  for token in value:gmatch("%S+") do
    local length = pixelLength(token)
    if not length then
      return nil
    end
    lengths[#lengths + 1] = length
  end
  if #lengths == 0 or #lengths > 4 then
    return nil
  end
  local top, right = lengths[1], lengths[2] or lengths[1]
  return top, right, lengths[3] or top, lengths[4] or right
end

-- Reads the width out of a CSS border shorthand such as "2px solid red", where
-- only the first length is a width and the rest describes the line.
-- @param value The declaration value, or nil
-- @return the width in pixels, or nil
local function borderWidth(value)
  if not value then
    return nil
  end
  for token in value:gmatch("%S+") do
    local length = pixelLength(token)
    if length then
      return length
    end
  end
  return nil
end

--- Helper function to extract spacing values (margin/border/padding) from CSS
-- Shorthands are read first and longhands over the top of them, so
-- "margin: 5px; margin-left: 20px" gives 20 on the left and 5 elsewhere.
-- @param css The CSS string to parse
-- @param property The property name to extract ("margin", "border" or "padding")
-- @return left, right, top, bottom spacing values in pixels, 0 for each side
--         the stylesheet says nothing measurable about
local function extractCSSSpacing(css, property)
  if not css then return 0, 0, 0, 0 end
  local spacing = {top = 0, right = 0, bottom = 0, left = 0}
  local border = property == "border"

  if border then
    local width = borderWidth(cssValue(css, "border"))
    if width then
      spacing.top, spacing.right, spacing.bottom, spacing.left = width, width, width, width
    end
  end

  local top, right, bottom, left = boxShorthand(cssValue(css, border and "border%-width" or property))
  if top then
    spacing.top, spacing.right, spacing.bottom, spacing.left = top, right, bottom, left
  end

  for _, side in ipairs({"top", "right", "bottom", "left"}) do
    local length
    if border then
      length = borderWidth(cssValue(css, "border%-" .. side)) or pixelLength(cssValue(css, "border%-" .. side .. "%-width") or "")
    else
      length = pixelLength(cssValue(css, property .. "%-" .. side) or "")
    end
    if length then
      spacing[side] = length
    end
  end

  return spacing.left, spacing.right, spacing.top, spacing.bottom
end

--- Sets the gauge amount.
-- @param currentValue Current numeric value, or if maxValue is omitted, then
--        it is assumed that currentValue is a value between 0 and 100 and is
--        used to set the gauge.
-- @param maxValue Maximum numeric value.  Optionally nil, see above.
-- @param text The text to display on the gauge, it is optional.
-- @return true, or nil and a message if maxValue has no reading to give
function Geyser.Gauge:setValue (currentValue, maxValue, text)
  assert(type(currentValue) == "number", string.format("bad argument #1 type (currentValue as number expected, got %s!)", type(currentValue)))
  assert(maxValue == nil or type(maxValue) == "number", string.format("bad argument #2 type (optional maxValue as number expected, got %s!)", type(maxValue)))
  -- A zero, negative or NaN maximum has no sensible reading: dividing by it leaves
  -- the gauge on an infinite or negative value that sticks until the next good
  -- call. Games do report these while a stat is still unknown, so refuse the
  -- reading and leave the gauge as it was rather than aborting the caller.
  if maxValue ~= nil and not (maxValue > 0) then
    local message = string.format("Geyser.Gauge:setValue: bad argument #2 value (maxValue must be a positive number, got %s!) - gauge '%s' was left as it was", tostring(maxValue), self.name)
    -- latched, because a game that reports a bad maximum reports it every prompt
    if not self.warnedBadMaxValue then
      self.warnedBadMaxValue = true
      debugc(message)
    end
    return nil, message
  end
  self.warnedBadMaxValue = nil
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
  
  -- Calculate spacing from the back label's CSS (margins, borders, padding)
  -- This fixes issue #5344: gauges with margins were misaligned
  local leftOffset, rightOffset, topOffset, bottomOffset = 0, 0, 0, 0
  
  if self.backCSS then
    local ml, mr, mt, mb = extractCSSSpacing(self.backCSS, "margin")
    local bl, br, bt, bb = extractCSSSpacing(self.backCSS, "border")
    local pl, pr, pt, pb = extractCSSSpacing(self.backCSS, "padding")
    
    leftOffset = ml + bl + pl
    rightOffset = mr + br + pr
    topOffset = mt + bt + pt
    bottomOffset = mb + bb + pb
  end
  
  -- Update gauge in the requested orientation
  -- Note: We use function-based constraints for dynamic sizing that accounts for margins
  -- The front label can have its own borders and padding (margins are stripped in setStyleSheet)
  -- Qt applies border/padding outside the widget's content area, so we don't need to compensate for them
  -- The offsets are given as functions rather than as "<n>px": a negative pixel
  -- constraint is measured from the opposite edge, which is not what a negative
  -- margin asks for

  if self.orientation == "horizontal" then
    -- Position the front label inside the back's content area
    self.front:move(function() return leftOffset end, function() return topOffset end)
    -- For width: we want value% of the CONTENT width (back label's content area)
    -- Content width = back_label_width - leftOffset - rightOffset
    local totalBackOffset = leftOffset + rightOffset
    local gaugeValue = self.value
    self.front:resize(
      function() return math.floor((self.back.get_width() - totalBackOffset) * (gaugeValue / 100) + 0.5) end,
      function() return math.floor(self.back.get_height() - topOffset - bottomOffset + 0.5) end
    )
  elseif self.orientation == "vertical" then
    -- For vertical (bottom-to-top), position needs to be calculated based on remaining space
    -- At 100%: y = topOffset (fills from top to bottom of content area)
    -- At 0%: y = topOffset + contentHeight (zero height at bottom)
    local totalBackOffset = topOffset + bottomOffset
    local gaugeValue = self.value
    self.front:move(
      function() return leftOffset end,
      function() return topOffset + math.floor((self.back.get_height() - totalBackOffset) * (1 - gaugeValue / 100) + 0.5) end
    )
    self.front:resize(
      function() return math.floor(self.back.get_width() - leftOffset - rightOffset + 0.5) end,
      function() return math.floor((self.back.get_height() - totalBackOffset) * (gaugeValue / 100) + 0.5) end
    )
  elseif self.orientation == "goofy" then
    -- For goofy (right-to-left), position needs to be calculated based on remaining space
    -- At 100%: x = leftOffset (fills from left to right edge)
    -- At 0%: x = leftOffset + contentWidth (zero width at right edge)
    local totalBackOffset = leftOffset + rightOffset
    local gaugeValue = self.value
    self.front:move(
      function() return leftOffset + math.floor((self.back.get_width() - totalBackOffset) * (1 - gaugeValue / 100) + 0.5) end,
      topOffset .. "px"
    )
    self.front:resize(
      function() return math.floor((self.back.get_width() - totalBackOffset) * (gaugeValue / 100) + 0.5) end,
      function() return math.floor(self.back.get_height() - topOffset - bottomOffset + 0.5) end
    )
  else -- batty (top to bottom)
    self.front:move(function() return leftOffset end, function() return topOffset end)
    local totalBackOffset = topOffset + bottomOffset
    local gaugeValue = self.value
    self.front:resize(
      function() return math.floor(self.back.get_width() - leftOffset - rightOffset + 0.5) end,
      function() return math.floor((self.back.get_height() - totalBackOffset) * (gaugeValue / 100) + 0.5) end
    )
  end

  if text then
    self.text:echo(text)
  end
  return true
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
-- @param color the color you want the text to be. Can use color names such as "red", decho codes such as "<255,0,0>" and hex codes such as "#ff0000"
function Geyser.Gauge:setFgColor(color)
  self.text:setFgColor(color)
end

--- Sets the text on the gauge, overwrites inherited echo function.
-- @param message the text to set
-- @param color the color of the text
-- @param format the text format
function Geyser.Gauge:echo(message, color, format)
  self.text:echo(message, color, format)
  self.format = self.text.format
  self.formatTable = self.text.formatTable
end

--- Sets the style sheet for the gauge
-- @param css Style sheet for the front label
-- @param cssback Style sheet for the back label
-- @param cssText Style sheet for the text label
function Geyser.Gauge:setStyleSheet(css, cssback, cssText)
  -- Store the original stylesheets
  self.frontCSS = css
  self.backCSS = cssback or css
  self.textCSS = cssText
  
  -- Apply back stylesheet normally (this has margins/borders/padding)
  self.back:setStyleSheet(self.backCSS)
  
  -- For the front label, strip ONLY margins (borders and padding are safe and allow styling)
  -- Margins on the front label cause positioning issues, but borders/padding are fine
  -- the trailing semicolon is optional: the last declaration in a stylesheet
  -- usually carries none, and a margin left on the front label is applied on
  -- top of the offset already worked out from the back label, doubling it.
  -- The frontier keeps qproperty-margin, which is not a margin, out of it.
  local frontCSSStripped = css
  if frontCSSStripped then
    frontCSSStripped = frontCSSStripped:gsub("%s*%f[%w%-]margin[^;]*;?", "")
  end
  self.front:setStyleSheet(frontCSSStripped)
  
  -- Apply text stylesheet if provided
  if cssText ~= nil then
    self.text:setStyleSheet(cssText)
  end
  
  -- Recalculate gauge positioning with the new stylesheet
  if self.value then
    self:setValue(self.value)
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

--- Deletes the gauge
-- Note: The child labels (back, front, text) are already in windowList
-- and will be deleted by the parent Container:delete() method, so we
-- don't need to explicitly delete them here to avoid double-deletion.
function Geyser.Gauge:type_delete()
  -- Children are automatically deleted by Container:delete()
  -- No additional cleanup needed
end

-- Overridden constructor to use add2
function Geyser.Gauge:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
  return me
end
