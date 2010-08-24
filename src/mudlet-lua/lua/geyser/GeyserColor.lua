--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

Geyser.Color = {}

--- Converts color to 3 hex values as a string, no alpha, css style
-- @return The color formatted as a hex string, as accepted by html/css
function Geyser.Color.hex (r,g,b)
   return string.format("#%02x%02x%02x", Geyser.Color.parse(r, g, b))
end

--- Converts color to 4 hex values as a string, with alpha, css style
-- @return The color formatted as a hex string, as accepted by html/css
function Geyser.Color.hexa (r,g,b,a)
   return string.format("#%02x%02x%02x%02x", Geyser.Color.parse(r, g, b, a))
end

--- Converts color to 3 hex values as a string, no alpha, hecho style
-- @return The color formatted as a hex string, as accepted by hecho
function Geyser.Color.hhex (r,g,b)
   return string.format("|c%02x%02x%02x", Geyser.Color.parse(r, g, b))
end

--- Converts color to 4 hex values as a string, with alpha, hecho style
-- @return The color formatted as a hex string, as accepted by hecho
function Geyser.Color.hhexa (r,g,b,a)
   return string.format("|c%02x%02x%02x%02x", Geyser.Color.parse(r, g, b, a))
end

--- Converts color to 3 decimal values as a string, no alpha, decho style
-- @return The color formatted as a decho() style string
function Geyser.Color.hdec (r,g,b)
   return string.format("<%d,%d,%d>", Geyser.Color.parse(r, g, b))
end

--- Converts color to 4 decimal values as a string, with alpha, decho style
-- @return The color formatted as a decho() style string
function Geyser.Color.hdeca (r,g,b,a)
   return string.format("<%d,%d,%d,%d>", Geyser.Color.parse(r, g, b, a))
end

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
   local r,g,b,a = 0,0,0,255

   -- have to have something to set, else can't do anything!
   if not red then
      print("No color supplied.\n")
      return
   end
   
   -- function to return next number
   local next_num = nil
   local base = 10
   -- assigns all the colors, used after we figure out how the color is
   -- represented as a string
   local assign_colors = function ()
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
         return r,g,b,a
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

            -- fourth case is a named string
         elseif color_table[red] then
            local i = 0
            local n = #color_table[red]
            next_num = function () -- create a simple iterator
                          i = i + 1
                          if i <= n then return color_table[red][i]
                          else return nil end
                       end
            
         else
            -- finally, no matches, do nothing
            return
         end
      end

   else
      -- Otherwise we weren't passed a complete string, but instead discrete
      -- components as either decimal or hex
      -- Yes, this is a little silly to do this way, but it fits with the
      -- rest of the parsing going on...
      local i = 0
      next_num = function ()
                    i = i + 1
                    if i == 1 then return red
                    elseif i == 2 then return green
                    elseif i == 3 then return blue
                    elseif i == 4 then return alpha
                    else return nil
                    end
                 end
   end
   assign_colors()
   return r,g,b,a
end

--- Applies colors to a window drawing from defaults and overridden values.
-- @param cons The window to apply colors to
function Geyser.Color.applyColors(cons)
   cons:setFgColor(cons.fgColor)
   cons:setBgColor(cons.bgColor)
   cons:setColor(cons.color)
end
