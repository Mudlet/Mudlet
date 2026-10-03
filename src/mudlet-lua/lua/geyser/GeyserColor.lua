--- Geyser color functions.
-- @author guy
-- @module Geyser.Color

Geyser.Color = {}

--- Converts color to 3 hex values as a string, no alpha, css style
-- @return The color formatted as a hex string, as accepted by html/css
function Geyser.Color.hex (r, g, b)
  return string.format("#%02x%02x%02x", Geyser.Color.parse(r, g, b))
end

--- Converts color to 4 hex values as a string, with alpha, css style
-- @return The color formatted as a hex string, as accepted by html/css
function Geyser.Color.hexa (r, g, b, a)
  return string.format("#%02x%02x%02x%02x", Geyser.Color.parse(r, g, b, a))
end

--- Converts color to 3 hex values as a string, no alpha, hecho style
-- @return The color formatted as a hex string, as accepted by hecho
function Geyser.Color.hhex (r, g, b)
  return string.format("|c%02x%02x%02x", Geyser.Color.parse(r, g, b))
end

--- Converts color to 4 hex values as a string, with alpha, hecho style
-- @return The color formatted as a hex string, as accepted by hecho
function Geyser.Color.hhexa (r, g, b, a)
  return string.format("|c%02x%02x%02x%02x", Geyser.Color.parse(r, g, b, a))
end

--- Converts color to 3 decimal values as a string, no alpha, decho style
-- @return The color formatted as a decho() style string
function Geyser.Color.hdec (r, g, b)
  return string.format("<%d,%d,%d>", Geyser.Color.parse(r, g, b))
end

--- Converts color to 4 decimal values as a string, with alpha, decho style
-- @return The color formatted as a decho() style string
function Geyser.Color.hdeca (r, g, b, a)
  return string.format("<%d,%d,%d,%d>", Geyser.Color.parse(r, g, b, a))
end

-- Geyser.Color.parse() as it always was, for the inputs it has no shortcut for
local function parseAnyColor(red, green, blue, alpha)
  local r, g, b, a = 0, 0, 0, 255

  -- have to have something to set, else can't do anything!
  if not red then
    return nil, "No color supplied"
  end

  -- function to return next number
  local next_num = nil
  local base = 10
  -- assigns all the colors, used after we figure out how the color is
  -- represented as a string
  local assign_colors = function()
    r = tonumber(next_num(), base)
    g = tonumber(next_num(), base)
    b = tonumber(next_num(), base)
    local has_a = next_num()
    if has_a then
      a = tonumber(has_a, base)
    end
  end

  -- Check if we were passed a string or table that needs to be parsed, i.e.,
  -- there is only a valid red value, and other params are nil.
  if not green or not blue then
    if type(red) == "table" then
      -- Here just copy over the appropriate values with sensible defaults
      r = red.r or 127
      g = red.g or 127
      b = red.b or 127
      a = red.a or 255
      return r, g, b, a
    elseif type(red) == "string" then
      -- first case is a hex string, where first char is '#'
      if string.find(red, "^#") then
        local pure_hex = string.sub(red, 2) -- strip format char
        next_num = string.gmatch(pure_hex, "%w%w")
        base = 16

        -- second case is a hex string, where first chars are '|c' or '0x'
      elseif string.find(red, "^[|0][cx]") then
        local pure_hex = string.sub(red, 3) -- strip format chars
        next_num = string.gmatch(pure_hex, "%w%w")
        base = 16

        -- third case is a decimal string, of the format "<dd,dd,dd>"
      elseif string.find(red, "^<") then
        next_num = string.gmatch(red, "%d+")

      else
        -- fourth case is a named string
        local col = Geyser.Color.find_color_name(red)
        if not col then
          -- finally, no matches, do nothing
          return
        end
        local i = 0
        local n = #color_table[col]
        next_num = function()
          -- create a simple iterator
          i = i + 1
          if i <= n then
            return color_table[col][i]
          else return nil
          end
        end
      end
    end

  else
    -- Otherwise we weren't passed a complete string, but instead discrete
    -- components as either decimal or hex
    -- Yes, this is a little silly to do this way, but it fits with the
    -- rest of the parsing going on...
    local i = 0
    next_num = function()
      i = i + 1
      if i == 1 then
        return red
      elseif i == 2 then
        return green
      elseif i == 3 then
        return blue
      elseif i == 4 then
        return alpha
      else return nil
      end
    end
  end
  assign_colors()
  return r, g, b, a
end

-- A hex or decimal colour string always means the same colour, so what it parses
-- to is kept - every label echo parses its colour again. Bounded, as a script
-- can make up any number of them.
local parsedColorStrings = {}
local parsedColorStringCount = 0

--- Returns 4 color components from (nearly any) acceptable format.  Colors can be
-- specified in two ways.  First: as a single word in english ("purple") or
-- hex ("#AA00FF", "|cAA00FF", or "0xAA00FF") or decimal ("<190,0,255>"). If
-- the hex or decimal representations contain a fourth element then alpha is
-- set too - otherwise alpha can't be set this way.  Second: by passing in
-- distinct components as unsigned integers (e.g. 23 or 0xA7).  When using the
-- second way, at least three values must be passed.  If only three are
-- passed, then alpha is 255.  Third: by passing in a table that has explicit
-- values for some, all or none of the keys r,g,b, and a.
-- @param red Either a valid string representation or the red component.
-- @param green The green component.
-- @param blue The blue component.
-- @param alpha The alpha component.
function Geyser.Color.parse(red, green, blue, alpha)
  -- the common shapes take a direct route that builds no closures, giving the
  -- same answers as parseAnyColor() would
  if green and blue then
    if not red then
      return nil, "No color supplied"
    end
    local a = 255
    if alpha then
      a = tonumber(alpha, 10)
    end
    return tonumber(red, 10), tonumber(green, 10), tonumber(blue, 10), a
  end
  if type(red) ~= "string" then
    return parseAnyColor(red, green, blue, alpha)
  end
  local parsed = parsedColorStrings[red]
  if parsed then
    return parsed[1], parsed[2], parsed[3], parsed[4]
  end
  if string.find(red, "^#") or string.find(red, "^[|0][cx]") or string.find(red, "^<") then
    local r, g, b, a = parseAnyColor(red, green, blue, alpha)
    if parsedColorStringCount >= 1000 then
      parsedColorStrings = {}
      parsedColorStringCount = 0
    end
    parsedColorStrings[red] = { r, g, b, a }
    parsedColorStringCount = parsedColorStringCount + 1
    return r, g, b, a
  end
  -- a name is looked up every time, as scripts can change color_table
  local col = Geyser.Color.find_color_name(red)
  if not col then
    return
  end
  local components = color_table[col]
  local n = #components
  local a = 255
  if n >= 4 and components[4] then
    a = tonumber(components[4], 10)
  end
  return tonumber(n >= 1 and components[1] or nil, 10), tonumber(n >= 2 and components[2] or nil, 10), tonumber(n >= 3 and components[3] or nil, 10), a
end

-- Lower cased name -> the name as color_table spells it. color_table holds
-- around 500 entries and a lookup runs on every echo that carries a named
-- colour, so scanning it is the single most expensive thing a Geyser label does.
local colorNamesByLowerCase

local function indexColorNames()
  local index = {}
  for name in pairs(color_table) do
    index[name:lower()] = name
  end
  colorNamesByLowerCase = index
  return index
end

--- Searches for a close match to 'color' in the color_table
--  Returns the color found, or false if it can't find one
-- @param color the color name you're trying to find
function Geyser.Color.find_color_name(color)
  if type(color) ~= "string" then
    return false
  end
  -- underscores are stripped from what is asked for but not from what
  -- color_table holds, so "light_blue" finds "lightblue" and not "light_blue"
  local wanted = color:lower():gsub("_", "")
  local index = colorNamesByLowerCase or indexColorNames()
  local found = index[wanted]
  if found and color_table[found] then
    return found
  end
  -- either the index has never seen this name or color_table no longer holds
  -- what it points at, so the index predates a change to the table. Scan for the
  -- answer, which costs what the whole lookup used to, and rebuild the index
  -- only when there was one to find - a name that is not a colour at all is the
  -- answer a script asking "is this a colour?" wants, not a reason to reindex.
  for name in pairs(color_table) do
    if name:lower() == wanted then
      indexColorNames()
      return name
    end
  end
  return false
end

--- Applies colors to a window drawing from defaults and overridden values.
-- @param cons The window to apply colors to
function Geyser.Color.applyColors(cons)
  cons:setFgColor(cons.fgColor)
  cons:setBgColor(cons.bgColor)
  cons:setColor(cons.color)
end
