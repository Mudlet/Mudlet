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

-- Takes CSS comments out, so a commented out declaration is not read as a live
-- one.
-- @param css The CSS string to clean
-- @return the string without its comments
local function withoutComments(css)
  return (css:gsub("/%*.-%*/", ""))
end

-- Finds the value of a CSS declaration. The property has to start a word of its
-- own, or "qproperty-margin" would be read as a margin, and the value ends at a
-- block brace as well as at a semicolon, so an unterminated
-- "QLabel { margin: 4px }" does not carry the brace into the value.
-- Only the first declaration of a property is read, so a stylesheet that sets
-- one twice, or sets one inside a state block such as ":hover", is read from
-- whichever comes first rather than by the CSS cascade.
-- @param css The CSS string to search, with its comments already taken out
-- @param property The property name
-- @return the value, lower cased and trimmed, or nil
local function cssValue(css, property)
  local value = css:match("%f[%w%-]" .. property:gsub("%-", "%%-") .. "%s*:%s*([^;{}]+)")
  if not value then
    return nil
  end
  -- "!important" marks the declaration's priority and is not part of its value.
  -- Lower casing lets an upper case unit be read and costs nothing else: every
  -- part of these values that is kept is a number or a unit.
  value = value:lower():gsub("!%s*important", "")
  return value:match("^%s*(.-)%s*$")
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
-- "margin: 5px; margin-left: 20px" gives 20 on the left and 5 elsewhere. This
-- is not the CSS cascade: a longhand wins over a shorthand whichever order they
-- are written in.
-- @param css The CSS string to parse
-- @param property The property name to extract ("margin", "border" or "padding")
-- @return left, right, top, bottom spacing values in pixels, 0 for each side
--         the stylesheet says nothing measurable about, and the text of the
--         first declaration that held a length with no pixel reading
local function extractCSSSpacing(css, property)
  if not css then return 0, 0, 0, 0 end
  css = withoutComments(css)
  local spacing = {top = 0, right = 0, bottom = 0, left = 0}
  local border = property == "border"
  local unreadable

  -- Reads one declaration. reportsUnreadable says whether a value the reader
  -- could make nothing of is worth telling the user about: it is for a length,
  -- and it is not for a border shorthand, where "border: none" legitimately
  -- names no width at all.
  local function read(declaration, reader, reportsUnreadable)
    local value = cssValue(css, declaration)
    if not value then
      return nil
    end
    local first, right, bottom, left = reader(value)
    if not first and reportsUnreadable then
      unreadable = unreadable or (declaration .. ": " .. value)
    end
    return first, right, bottom, left
  end

  if border then
    local width = read("border", borderWidth, false)
    if width then
      spacing.top, spacing.right, spacing.bottom, spacing.left = width, width, width, width
    end
  end

  local top, right, bottom, left = read(border and "border-width" or property, boxShorthand, true)
  if top then
    spacing.top, spacing.right, spacing.bottom, spacing.left = top, right, bottom, left
  end

  for _, side in ipairs({"top", "right", "bottom", "left"}) do
    local length
    if border then
      length = read("border-" .. side, borderWidth, false) or read("border-" .. side .. "-width", pixelLength, true)
    else
      length = read(property .. "-" .. side, pixelLength, true)
    end
    if length then
      spacing[side] = length
    end
  end

  return spacing.left, spacing.right, spacing.top, spacing.bottom, unreadable
end

-- Reads the back label's margin, border and padding as one set of offsets, kept
-- against the stylesheet they were read from: setValue runs on every prompt in
-- most UIs and these three readings cost more than everything else it does,
-- while the stylesheet behind them changes about once a session.
-- @return left, right, top, bottom, and the first declaration that held a
--         length with no pixel reading
local function backSpacing(gauge)
  local css = gauge.backCSS
  local cached = gauge.backSpacingCache
  -- read is its own flag because a gauge with no back stylesheet at all has a
  -- css of nil, which is a reading in its own right
  if cached.read and cached.css == css then
    return cached.left, cached.right, cached.top, cached.bottom, cached.unreadable
  end

  local left, right, top, bottom, unreadable = 0, 0, 0, 0, nil
  if css then
    local ml, mr, mt, mb, marginUnreadable = extractCSSSpacing(css, "margin")
    local bl, br, bt, bb, borderUnreadable = extractCSSSpacing(css, "border")
    local pl, pr, pt, pb, paddingUnreadable = extractCSSSpacing(css, "padding")
    left, right, top, bottom = ml + bl + pl, mr + br + pr, mt + bt + pt, mb + bb + pb
    unreadable = marginUnreadable or borderUnreadable or paddingUnreadable
  end

  cached.read, cached.css = true, css
  cached.left, cached.right, cached.top, cached.bottom, cached.unreadable = left, right, top, bottom, unreadable
  return left, right, top, bottom, unreadable
end

-- the stock reposition, as loaded, so that one a script wrapped later still runs
local containerReposition = Geyser.Container.reposition

-- A fill whose corner stays put only needs resizing for a new value - unless
-- something inside the front label, or a reposition of its own, has to follow.
local function resizesInPlace(fill, front)
  return fill.cornerFixed and front.reposition == containerReposition and front.redraw == nil
    and next(front.windowList) == nil
end

local stockPixelGeometry = Geyser.Container.pixel_geometry

-- must match roundEdge in GeyserContainer.lua bit for bit, or an in-place size drifts from pixel_geometry()
local function roundEdge(value)
  return math.floor(value + 0.5 + 1e-6)
end

local dimensions = {"x", "y", "width", "height"}
local rootGetX, rootGetY, rootGetWidth, rootGetHeight = Geyser.get_x, Geyser.get_y, Geyser.get_width, Geyser.get_height
-- far deeper than any real layout; a container loop would otherwise never end
local deepestChain = 100
local weakValues = {__mode = "v"}
local churnSkips, mostSkips = 32, 1024
-- recording costs a few pixel_geometry() calls, so a layout pays for itself only after this many reuses
local paysAfter = 3

-- where the fill starts, the back label's inner length along it and the front
-- label's size across it: all an in-place resize reads besides the value
local function measure(fill, front, back, layout)
  local x, y = front.get_x(), front.get_y()
  if layout.horizontal then
    layout.start, layout.inner = x, back.get_width() - fill.left - fill.right
    layout.across = roundEdge(y + front.get_height()) - roundEdge(y)
  else
    layout.start, layout.inner = y, back.get_height() - fill.top - fill.bottom
    layout.across = roundEdge(x + front.get_width()) - roundEdge(x)
  end
  layout.startEdge = roundEdge(layout.start)
end

local function askAgain(live, length, getter)
  live[length + 1], live[length + 2] = getter, getter()
  return length + 2
end

-- Records the getters measure() goes through, from the front label up. The front
-- label's own, and any compiled from a pixel or percentage constraint, only have
-- to stay in place; any other, and the main window's size behind the root, is
-- asked again.
-- @param old a layout of this gauge's that no longer holds, whose tables are reused
-- @return the layout, or nil when the resize has to fall back to pixel_geometry()
local function recordLayout(gauge, fill, front, old)
  local back = gauge.back
  if back.container ~= gauge or front.pixel_geometry ~= stockPixelGeometry then
    return nil
  end
  for _, dimension in ipairs(dimensions) do
    local constraint, container = Geyser.compiled_from(front, dimension)
    if constraint ~= fill[dimension] or container ~= gauge then
      return nil
    end
  end

  -- weak, so that a window the gauge has left is not kept alive by it
  local chain = old and old.chain or setmetatable({}, weakValues)
  local live = old and old.live or setmetatable({}, weakValues)
  chain[1], chain[2], chain[3], chain[4], chain[5] = front, front.get_x, front.get_y, front.get_width, front.get_height
  local length, liveLength, readsMainWindow = 5, 0, false
  -- the front label's getters read the back label's lengths, and the gauge's x and y above it
  local window, needX, needY, needWidth, needHeight = back, false, false, true, true
  local upX, upY, upWidth, upHeight = true, true, false, false
  while window do
    if length >= deepestChain * 5 then
      return nil
    end
    local getX, getY, getWidth, getHeight = window.get_x, window.get_y, window.get_width, window.get_height
    chain[length + 1], chain[length + 2], chain[length + 3], chain[length + 4], chain[length + 5] =
      window, getX, getY, getWidth, getHeight
    length = length + 5
    local container = window.container
    local against, readsX, readsY, readsWidth, readsHeight
    if window == Geyser then
      readsX = getX == rootGetX and 0 or nil
      readsY = getY == rootGetY and 0 or nil
      readsWidth = getWidth == rootGetWidth and 0 or nil
      readsHeight = getHeight == rootGetHeight and 0 or nil
      readsMainWindow = (needWidth and readsWidth ~= nil) or (needHeight and readsHeight ~= nil)
    else
      against, readsX, readsY, readsWidth, readsHeight = Geyser.compiled_reads(window)
      if against ~= container then
        readsX, readsY, readsWidth, readsHeight = nil, nil, nil, nil
      end
    end
    -- lengths first, as one measured from the far edge also reads the window's own near edge
    if needWidth then
      if readsWidth then
        upX, upWidth, needX = upX or readsWidth % 2 == 1, upWidth or readsWidth % 4 >= 2, needX or readsWidth >= 4
      else
        liveLength = askAgain(live, liveLength, getWidth)
      end
    end
    if needHeight then
      if readsHeight then
        upY, upHeight, needY = upY or readsHeight % 2 == 1, upHeight or readsHeight % 4 >= 2, needY or readsHeight >= 4
      else
        liveLength = askAgain(live, liveLength, getHeight)
      end
    end
    if needX then
      if readsX then
        upX, upWidth = upX or readsX % 2 == 1, upWidth or readsX % 4 >= 2
      else
        liveLength = askAgain(live, liveLength, getX)
      end
    end
    if needY then
      if readsY then
        upY, upHeight = upY or readsY % 2 == 1, upHeight or readsY % 4 >= 2
      else
        liveLength = askAgain(live, liveLength, getY)
      end
    end
    if not (upX or upY or upWidth or upHeight) then
      break
    end
    window, needX, needY, needWidth, needHeight = container, upX, upY, upWidth, upHeight
    upX, upY, upWidth, upHeight = false, false, false, false
  end

  local layout = {
    chain = chain, length = length, live = live, liveLength = liveLength,
    horizontal = fill.orientation == "horizontal",
    left = fill.left, right = fill.right, top = fill.top, bottom = fill.bottom,
    -- updates in a row it has served since it was last measured
    reused = 0,
  }
  if readsMainWindow then
    local width, height = getMainWindowSize()
    -- false, as a nil width would read as a layout the main window's size does not matter to
    layout.mainWidth, layout.mainHeight = width or false, height
  end
  measure(fill, front, back, layout)
  return layout
end

local function chainHolds(gauge, front, layout)
  local chain = layout.chain
  if chain[1] ~= front or chain[6] ~= gauge.back or front.pixel_geometry ~= stockPixelGeometry then
    return false
  end
  for i = 1, layout.length, 5 do
    local window = chain[i]
    if window == nil or window.get_x ~= chain[i + 1] or window.get_y ~= chain[i + 2]
       or window.get_width ~= chain[i + 3] or window.get_height ~= chain[i + 4] then
      return false
    end
  end
  return true
end

-- Whether what the layout's measure() read still reads the same, recording
-- what reads differently now
local function valuesHold(fill, layout)
  local holds = true
  if fill.left ~= layout.left or fill.right ~= layout.right or fill.top ~= layout.top or fill.bottom ~= layout.bottom then
    layout.left, layout.right, layout.top, layout.bottom = fill.left, fill.right, fill.top, fill.bottom
    holds = false
  end
  if layout.mainWidth ~= nil then
    local width, height = getMainWindowSize()
    width = width or false
    if width ~= layout.mainWidth or height ~= layout.mainHeight then
      layout.mainWidth, layout.mainHeight = width, height
      holds = false
    end
  end
  local live = layout.live
  for i = 1, layout.liveLength, 2 do
    local value = live[i]()
    if value ~= live[i + 1] then
      live[i + 1] = value
      holds = false
    end
  end
  return holds
end

-- each layout dropped before it paid for itself turns recording off for twice as long as the last
local function skipRecording(fill)
  local skips = fill.backoff or churnSkips
  fill.skips, fill.backoff = skips, math.min(skips * 2, mostSkips)
end

-- The front label's size for the fill's current value, worked out the way
-- pixel_geometry() would without calling every getter up to the root again.
local function inPlaceSize(gauge, fill, front)
  local layout = fill.layout
  -- off the fill until it is whole again, so a getter that raises cannot leave it half updated
  fill.layout = nil
  if layout and chainHolds(gauge, front, layout) then
    local reused = layout.reused
    if valuesHold(fill, layout) then
      layout.reused = reused + 1
      if reused + 1 == paysAfter then
        fill.backoff = nil
      end
    elseif reused == 0 then
      -- what it rests on answers differently every update, so asking it again costs more than pixel_geometry()
      skipRecording(fill)
      layout = nil
    else
      layout.reused = 0
      measure(fill, front, gauge.back, layout)
    end
  elseif layout and layout.reused < paysAfter then
    -- it broke before it paid for itself, so it is probably laid out again too often for recording to pay
    skipRecording(fill)
    layout = nil
  else
    layout = recordLayout(gauge, fill, front, layout)
  end
  if not layout then
    local _, _, width, height = front:pixel_geometry()
    return width, height
  end
  fill.layout = layout
  local along = roundEdge(layout.start + layout.inner * (fill.value / 100)) - layout.startEdge
  if layout.horizontal then
    return along, layout.across
  end
  return layout.across, along
end

-- Gives the front label the constraints for one orientation. They are functions
-- rather than "<n>px" because a negative pixel constraint is measured from the
-- opposite edge, which is not what a negative margin asks for, and they read the
-- gauge's fill table rather than closing over its numbers, so that a new value
-- costs a layout pass and not a fresh set of constraints. Only the back label's
-- spacing is compensated for: Qt lays the front label's own border and padding
-- outside its content area, and setStyleSheet strips its margins. The lengths
-- stay fractional: rounding them here as well as at the edges would leave a
-- full front a pixel off the back it covers.
local function setFrontConstraints(gauge)
  local fill = gauge.frontFill
  local front = gauge.front
  local orientation = gauge.orientation
  -- whether x and y below leave the value out
  fill.cornerFixed = orientation ~= "vertical" and orientation ~= "goofy"

  if orientation == "horizontal" then
    front.x = function() return fill.left end
    front.y = function() return fill.top end
    front.width = function()
      return (gauge.back.get_width() - fill.left - fill.right) * (fill.value / 100)
    end
    front.height = function() return gauge.back.get_height() - fill.top - fill.bottom end
  elseif orientation == "vertical" then
    -- bottom to top, so an emptier gauge starts further down
    front.x = function() return fill.left end
    front.y = function()
      return fill.top + (gauge.back.get_height() - fill.top - fill.bottom) * (1 - fill.value / 100)
    end
    front.width = function() return gauge.back.get_width() - fill.left - fill.right end
    front.height = function()
      return (gauge.back.get_height() - fill.top - fill.bottom) * (fill.value / 100)
    end
  elseif orientation == "goofy" then
    -- right to left, so an emptier gauge starts further right
    front.x = function()
      return fill.left + (gauge.back.get_width() - fill.left - fill.right) * (1 - fill.value / 100)
    end
    front.y = function() return fill.top end
    front.width = function()
      return (gauge.back.get_width() - fill.left - fill.right) * (fill.value / 100)
    end
    front.height = function() return gauge.back.get_height() - fill.top - fill.bottom end
  else -- batty (top to bottom), and anything the gauge cannot make sense of
    front.x = function() return fill.left end
    front.y = function() return fill.top end
    front.width = function() return gauge.back.get_width() - fill.left - fill.right end
    front.height = function()
      return (gauge.back.get_height() - fill.top - fill.bottom) * (fill.value / 100)
    end
  end

  -- the constraints as installed, so that anything that replaces one of them -
  -- an autoAdjustSize on the front label, a script moving it by hand - is
  -- noticed and undone on the next update rather than sticking for good
  fill.orientation = orientation
  fill.x, fill.y, fill.width, fill.height = front.x, front.y, front.width, front.height
  front:set_constraints(front)
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
  
  -- the constructor makes both of these; a gauge that got here without them is
  -- given them rather than made to error
  self.frontFill = self.frontFill or {}
  self.backSpacingCache = self.backSpacingCache or {}

  -- Spacing from the back label's CSS (margins, borders, padding), so that a
  -- gauge with margins is not misaligned (issue #5344)
  local leftOffset, rightOffset, topOffset, bottomOffset, unreadable = backSpacing(self)

  -- Qt still applies a spacing Geyser cannot measure, so the fill bar ends up
  -- laid out against the wrong box and spills past the gauge's frame. Say so
  -- rather than leave it looking like a Geyser bug - latched, because setValue
  -- runs on every prompt.
  if unreadable and not self.warnedUnreadableCSS then
    self.warnedUnreadableCSS = true
    debugc(string.format(
      "Geyser.Gauge: gauge '%s' has a stylesheet spacing Geyser cannot measure in pixels (%s), so it is left out of the fill bar's position - use px or unitless lengths",
      self.name, unreadable))
  end

  local fill = self.frontFill
  local filled = fill.value ~= self.value
  local spaced = fill.left ~= leftOffset or fill.right ~= rightOffset or fill.top ~= topOffset or fill.bottom ~= bottomOffset
  fill.value, fill.left, fill.right, fill.top, fill.bottom = self.value, leftOffset, rightOffset, topOffset, bottomOffset

  -- The constraints read the fill table, so a new value only needs the front
  -- label laid out again - or just resized, when its corner stays put - and a
  -- gauge sitting on the value it already has, which the gauges a UI repaints
  -- wholesale on every prompt usually are, does not even need that.
  local front = self.front
  if fill.orientation ~= self.orientation or front.x ~= fill.x or front.y ~= fill.y
     or front.width ~= fill.width or front.height ~= fill.height then
    setFrontConstraints(self)
  elseif spaced or (filled and not resizesInPlace(fill, front)) then
    front:reposition()
  elseif filled and fill.skips then
    local skips = fill.skips
    fill.skips = skips > 1 and skips - 1 or nil
    local _, _, width, height = front:pixel_geometry()
    resizeWindow(front.name, width, height)
  elseif filled then
    resizeWindow(front.name, inPlaceSize(self, fill, front))
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
  self.warnedUnreadableCSS = nil
  
  -- Apply back stylesheet normally (this has margins/borders/padding)
  self.back:setStyleSheet(self.backCSS)
  
  -- For the front label, strip ONLY margins (borders and padding are safe and allow styling)
  -- Margins on the front label cause positioning issues, but borders/padding are fine
  -- the trailing semicolon is optional: the last declaration in a stylesheet
  -- usually carries none, and a margin left on the front label is applied on
  -- top of the offset already worked out from the back label, doubling it.
  -- The frontier keeps qproperty-margin, which is not a margin, out of it, and
  -- the excluded braces and star stop a declaration that carries no semicolon
  -- from eating the end of its block or of a comment, which would leave Qt an
  -- unparseable sheet and the front label with no styling at all.
  local frontCSSStripped = css
  if frontCSSStripped then
    frontCSSStripped = frontCSSStripped:gsub("%s*%f[%w%-]margin[^;{}%*]*;?", "")
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

  -- the gauge's labels are made only once the gauge is in its container, too late to stop there
  if cons.name then
    for _, suffix in ipairs({"_back", "_front", "_text"}) do
      local taken = mudlet.nameTakenByOtherKind(cons.name .. suffix, "label", cons.windowname or (container or Geyser).windowname)
      if taken then
        printError(string.format("Geyser.Gauge '%s' was not created: %s", cons.name, taken), false, false)
        return nil, taken
      end
    end
  end

  -- Call parent's constructor
  local me = self.parent:new(cons, container)

  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self
  me.windowname = me.windowname or me.container.windowname or "main"

  -- what the front label was last laid out against, and the reading taken off
  -- the back label's stylesheet
  me.frontFill = {}
  me.backSpacingCache = {}

  -- Now create the Gauge using primitives and tastey classes

  -- Set up the constraints for the front label, the label that changes size to
  -- indicated levels in the gauges. Message set to nil to avoid unwanted text
  local front = Geyser.copyTable(cons)
  front.name = me.name .. "_front"
  front.color = me.color
  front.message = nil
  front.x, front.y, front.width, front.height = 0, 0, "100%", "100%"
  -- the gauge's own hidden constraint is not its labels': they follow the gauge
  -- as its children, and a label hidden in its own right would refuse to come
  -- back when the gauge is shown
  front.hidden, front.auto_hidden = nil, nil

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
  return self:new(cons, container)
end
