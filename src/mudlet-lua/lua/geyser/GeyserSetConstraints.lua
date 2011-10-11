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
   -- GENERATE CONSTRAINT AWARE POSITIONING FUNCTIONS
   -- Parse the position constraints to generate functions that will get
   -- window dimensions according to those constraints. Also, update position
   -- information.  The order that the dimensions are specified in the for
   -- loop is important so that get_x() and get_y() are defined before they
   -- needed by width and height.
   for _,v in ipairs{"x", "y", "width", "height"} do
      local getter = "get_" .. v -- name of the function to calculate the

      -- if passed a number assume pixels are meant
      if type(cons[v]) == "number" then
         cons[v] = string.format("%dpx", cons[v])
      end
      
      -- Parse the constraint
      if string.find(cons[v], "%%") then
         -- This handles dimensions as a percentage of the container.
         -- Negative percentages are converted to the equivalent positive.
         --------------------------------------------------------------   

         -- scale is a value between 0 and 1   
         local scale = tonumber((string.gsub(cons[v], "%%", ""))) / 100.0
         if scale < 0 then
            scale = 1 + scale
         end
         local dim = ""
         local min, max = 0,0

         if v == "x" then
            min = "container.get_x()"
            max = "container.get_width()"
         elseif v == "width" then
            max = "container.get_width()"
         elseif v == "y" then
            min = "container.get_y()"
            max = "container.get_height()"
         else
            max = "container.get_height()"
         end
         
         -- Define the getter function
         -- Heiko: on European locales this leads to compile errors because
		 --        scale will be "0,5" instead of "0.5"-> syntax error
		 --        Need to find out if there's more such cases in Geyse
		 --        Need to find out if there's more such cases in Geyser
		 h_scale = tostring(scale)
		 local src = "local self,container = ...  return function () return " .. min .. " + (" .. h_scale .. " * " .. max .. ") end"
         
         -- compile the getter
         window[getter] = assert(loadstring(src, "getter for " .. v))(window,container)
         
      else
         -- This handles absolute positioning and character positioning
         -- Negative values indicated positioning from the anti-origin.
         -- Pre: cons[v] must contain "px" or "c"
         --------------------------------------------------------------
         
         -- Create default values
         local x_mult, y_mult = 1,1 -- by default assume not "c"
         local negative = string.find(cons[v], "-") or false -- detect "negative" 0
         
         -- As is, font size is considered a constraint
         if string.find(cons[v], "c") then
            x_mult, y_mult = calcFontSize(window.fontSize)
         end
         
         local pos = tonumber((string.gsub(cons[v], "%a", "")))
         local max = 0
         local min = 0

         if v == "x" then
            min = "container.get_x()"
            pos = x_mult * pos
         elseif v == "width" then
            pos = x_mult * pos
         elseif v == "y" then
            min = "container.get_y()"
            pos = y_mult * pos
         else
            pos = y_mult * pos
         end

         -- Treat negative values differently
         if negative then
            if v == "x" then
               min = "container.get_x()"
               max = "container.get_width()"
            elseif v == "width" then
               min = "container.get_x()"
               max = "container.get_width()"
               pos = string.format("(%d - self:get_x())", pos)
            elseif v == "y" then 
               min = "container.get_y()"
               max = "container.get_height()"
            else -- v == "height"
               min = "container.get_y()"
               max = "container.get_height()"
               pos = string.format("(%d - self:get_y())", pos)
            end
         end

         -- Define the getter function
         local src = "local self, container = ... return function () return " .. max .. " + " .. min .. " + " .. pos .. " end"

         -- compile the getter
         window[getter] = assert(loadstring(src, "getter for " .. v))(window, container)
      end
      
      -- Here the actual value of the dimension is set according to the
      -- constraints requested.
      --window[v] = window[getter]()
   end -- END for that generates POSITION FUNCTIONS
   os.setlocale(oldlocale, "numeric")
   window:reposition()
end
