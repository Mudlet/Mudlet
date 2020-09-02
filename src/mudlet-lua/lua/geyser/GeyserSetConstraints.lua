--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

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
  oldlocale = os.setlocale(nil, "numeric")
  os.setlocale("C", "numeric")
  -- If container is nil then by default it is the dimensions of the main window
  container = container or Geyser
  local con_x = container.get_x()
  local con_y = container.get_y()
  local con_width = container.get_width()
  local con_height = container.get_height()

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
    if type(cons[v]) == "function" then
      num = string.format("%dpx", cons[v]())
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
      local dim = ""
      local min, max = 0, 0

      if v == "x" then
        min = con_x
        max = con_width
      elseif v == "width" then
        max = con_width
      elseif v == "y" then
        min = con_y
        max = con_height
      else
        max = con_height
      end

      -- Define the getter function
      -- Heiko: on European locales this leads to compile errors because
      --        scale will be "0,5" instead of "0.5"-> syntax error
      --        Need to find out if there's more such cases in Geyser

      -- compile the getter
      window[getter] = function() return min + (scale * max) + offset end

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
      end

      local pos = tonumber((string.gsub(num, "%a", "")))
      local max = 0
      local min = 0

      if v == "x" then
        min = con_x
        pos = x_mult * pos
      elseif v == "width" then
        pos = x_mult * pos
      elseif v == "y" then
        min = con_y
        pos = y_mult * pos
      else
        pos = y_mult * pos
      end

      -- Treat negative values differently
      if negative then
        if v == "x" then
          min = con_x
          max = con_width
        elseif v == "width" then
          min = con_x
          max = con_width
          pos = pos - window:get_x()
        elseif v == "y" then
          min = con_y
          max = con_height
        else -- v == "height"
          min = con_y
          max = con_height
          pos = pos - window:get_y()
        end
      end

      -- compile the getter
      window[getter] = function() return max + min + pos end
    end

    -- Here the actual value of the dimension is set according to the
    -- constraints requested.
    --window[v] = window[getter]()
  end -- END for that generates POSITION FUNCTIONS
  os.setlocale(oldlocale, "numeric")
  window:reposition()
end
