--- Setting window contraints.
-- @author guy
-- @module Geyser.SetConstraints

local function return_zero()
  return 0
end

-- What each window's getters were last compiled from, so that laying the same
-- constraints out again - which a resize does for every window in a box that
-- holds a fixed size child, several times over - can keep the getters it has
-- instead of parsing the constraints again. Weak keys, so a window that is
-- gone takes its entry with it; and weak values inside each entry, because the
-- getters there refer to the window, and Lua 5.1 would otherwise keep the
-- window alive through its own entry. Anything collected from an entry only
-- makes it stop matching.
local compiled = setmetatable({}, {__mode = "k"})
local weakValues = {__mode = "v"}

-- Internal function: whether the getters on a window are still the ones that
-- were compiled from these very constraints, against this very container
local function unchanged(window, cons, container)
  local last = compiled[window]
  -- a getter collected from the entry reads as nil, which must not pass for a
  -- window whose own getter has been taken away
  return last ~= nil and last.container == container
    and last.x == cons.x and last.y == cons.y and last.width == cons.width and last.height == cons.height
    and last.get_x ~= nil and last.get_y ~= nil and last.get_width ~= nil and last.get_height ~= nil
    and window.get_x == last.get_x and window.get_y == last.get_y
    and window.get_width == last.get_width and window.get_height == last.get_height
end

function Geyser.calc_constraints (window, cons, container)
  -- If container is nil then by default it is the dimensions of the main window
  container = container or Geyser
  container["return_zero"] = return_zero
  window["return_zero"] = return_zero
  if unchanged(window, cons, container) then
    return
  end
  local oldlocale = os.setlocale(nil, "numeric")
  os.setlocale("C", "numeric")
  -- a character constraint reads the font size as it is parsed, so its getters
  -- are not kept for next time
  local readsFontSize = false
  
  -- GENERATE CONSTRAINT AWARE POSITIONING FUNCTIONS
  -- Parse the position constraints to generate functions that will get
  -- window dimensions according to those constraints. Also, update position
  -- information.  The order that the dimensions are specified in the for
  -- loop is important so that get_x() and get_y() are defined before they
  -- needed by width and height.
  for _, v in ipairs { "x", "y", "width", "height" } do
    local getter = "get_" .. v -- name of the function to calculate the
    local num
    -- if passed a number assume pixels are meant
    if type(cons[v]) == "number" then
      cons[v] = string.format("%dpx", cons[v])
    end
    
    -- if passed as function which returns numbers assume pixels are meant
    -- give num the value 0 and let the function define the value
    if type(cons[v]) == "function" then
      num = 0
    end    
    num = num or cons[v]
    
    -- Parse the constraint
    if string.find(num, "%%") then
      -- This handles dimensions as a percentage of the container.
      -- Negative percentages are converted to the equivalent positive.
      --------------------------------------------------------------
      
      -- scale is a value between 0 and 1
      -- offset is always in pixel
      local scale, offset = string.match(num,"([%+%-%d%p]+)%%%s*([%+%-%d%p]*)")
      local negative = string.find(scale, "-") or false -- detect "negative" 0
      scale = tonumber(scale) / 100.0
      offset = tonumber(offset) or 0
      if negative then
        scale = 1 + scale
      end
      
      local min, max = "return_zero", "return_zero"
      
      if v == "x" then
        min = "get_x"
        max = "get_width"
      elseif v == "width" then
        max = "get_width"
      elseif v == "y" then
        min = "get_y"
        max = "get_height"
      else
        max = "get_height"
      end
      
      -- Define the getter function
      -- Heiko: on European locales this leads to compile errors because
      --        scale will be "0,5" instead of "0.5"-> syntax error
      --        Need to find out if there's more such cases in Geyser
      
      -- compile the getter, leaving out the terms that are always zero: a getter
      -- calls its container's, all the way up, on every resize and gauge update
      -- an offset of -0 ("0%-0") keeps the full sum, whose zero terms give 0, not -0
      local scaled = scale ~= 0
      if 1 / offset == -math.huge then
        window[getter] = function()
          return container[min]() + scale * container[max]() + offset
        end
      elseif min == "return_zero" and not scaled then
        window[getter] = function()
          return offset
        end
      elseif min == "return_zero" then
        window[getter] = function()
          return scale * container[max]() + offset
        end
      elseif not scaled then
        window[getter] = function()
          return container[min]() + offset
        end
      else
        window[getter] = function()
          return container[min]() + scale * container[max]() + offset
        end
      end
      
      
    else
      -- This handles absolute positioning and character positioning
      -- Negative values indicated positioning from the anti-origin.
      -- Pre: num must contain "px" or "c"
      --------------------------------------------------------------
      
      -- Create default values
      local x_mult, y_mult = 1, 1 -- by default assume not "c"
      local negative = string.find(num, "-") or false -- detect "negative" 0
      
      -- As is, font size is considered a constraint
      if string.find(num, "c") then
        x_mult, y_mult = calcFontSize(window.fontSize)
        readsFontSize = true
      end
      
      local pos_func = "return_zero"
      local max = "return_zero"
      local min = "return_zero"
      local func = return_zero
      local pos = tonumber((string.gsub(num, "%a", "")))
      
      -- give func the function value if a function is given
      if type(cons[v]) == "function" then
        func = cons[v]
      end
      
      if v == "x" then
        min = "get_x"
        pos = x_mult * pos
      elseif v == "width" then
        pos = x_mult * pos
      elseif v == "y" then
        min = "get_y"
        pos = y_mult * pos
      else
        pos = y_mult * pos
      end
      
      -- Treat negative values differently
      if negative then
        if v == "x" then
          min = "get_x"
          max = "get_width"
        elseif v == "width" then
          min = "get_x"
          max = "get_width"
          pos_func = "get_x"
        elseif v == "y" then
          min = "get_y"
          max = "get_height"
        else -- v == "height"
          min = "get_y"
          max = "get_height"
          pos_func = "get_y"
        end
      end
      
      -- compile the getter, leaving out the terms that are always zero
      -- a function constraint is parsed as "0", so it is never negative; the
      -- 0 + keeps the arithmetic that coerces what the function returns
      if func ~= return_zero and min == "return_zero" then
        window[getter] = function()
          return 0 + func()
        end
      elseif func ~= return_zero then
        window[getter] = function()
          return container[min]() + func()
        end
      elseif not negative and min == "return_zero" then
        window[getter] = function()
          return pos
        end
      elseif not negative then
        window[getter] = function()
          return container[min]() + pos
        end
      elseif pos_func == "return_zero" then
        window[getter] = function()
          return container[max]() + container[min]() + pos
        end
      else
        window[getter] = function()
          return container[max]() + container[min]() + pos - window[pos_func]()
        end
      end
    end
    
    -- Here the actual value of the dimension is set according to the
    -- constraints requested.
    --window[v] = window[getter]()
  end -- END for that generates POSITION FUNCTIONS
  os.setlocale(oldlocale, "numeric")
  if not readsFontSize then
    compiled[window] = setmetatable({
      container = container,
      x = cons.x, y = cons.y, width = cons.width, height = cons.height,
      get_x = window.get_x, get_y = window.get_y, get_width = window.get_width, get_height = window.get_height,
    }, weakValues)
  else
    compiled[window] = nil
  end
end

--- This function sets the constraints of a window.
-- It doesn't mess with anything other than positioning data.  It
-- creates get_x(), get_y(), get_width(), and get_height() functions
-- for 'window'
-- @param window The window to create the constraints for.
-- @param cons A table that holds the constraints.
-- @param container A table that holds maximum position values and
-- represents the dimensions of the "window" that holds whatever
-- widget is being created.
function Geyser.set_constraints (window, cons, container)
  Geyser.calc_constraints(window, cons, container)
  window:reposition()
end
