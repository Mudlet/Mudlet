--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

local cparse = Geyser.Color.parse

--- Represents an abstract window class designed to be subclassed for windows
-- that are built on Mudlet primitives, like labels.
-- @class table
-- @name Geyser.Window
-- @field message The message last *echo()'d to this window. Default is “”.
-- @field bgColor Text background color, default "white"
-- @field fgColor Text foreground color, default "black"
-- @field color Window background color, default "#202020"
Geyser.Window = Geyser.Container:new({
      name = "WindowClass",
      message = "",

      -- default colors.  The are boring
      fgColor = "white",
      bgColor = "black",
      color = "#202020",
   })

-- Add regular window methods, these are mostly just OO-ized wrappers

--- Prints a message to the window
-- @param message The message to print. Can contain html formatting.
function Geyser.Window:echo(message)
   self.message = message
   echo(self.name, self.message)
end

--- Prints a message to the window.
-- @param message The message to print. Uses color formatting information -
-- a message of "<red>Hi" would make 'Hi' red.
function Geyser.Window:cecho(message) self.message = message or self.message cecho(self.name, self.message) end

--- Prints a message to the window.
-- @param message The message to print. Uses color formatting information -
-- a message of "<255,0,0>Hi" would make 'Hi' red.
function Geyser.Window:decho(message) self.message = message or self.message decho(self.name, self.message) end

--- Prints a message to the window.
-- @param message The message to print. Uses color formatting information -
-- a message of "|cff0000Hi" would make 'Hi' red.
function Geyser.Window:hecho(message) self.message = message or self.message hecho(self.name, self.message) end

--- Get the window's foreground color.
-- @return The foreground color of this window primitive as a Geyser.Color object.
function Geyser.Window:getFgColor ()
   return getFgColor(self.name)
end

---  Get the window's background color
-- @return The background color of this window primitive as a Geyser.Color object.
function Geyser.Window:getBgColor ()
   return getBgColor(self.name)
end

--- Sets the background color of this window primitive.  If this primitive
-- was using a shared color, then it creates a new, personal color.
-- @param r The red value, or a quoted color name, like "green".
-- @param g The green value, or nil if using a name.
-- @param b The blue value, or nil if using a name.
function Geyser.Window:setBgColor (r,g,b)
   setBgColor(self.name, cparse(r,g,b))
end

--- Sets the foreground color of this window primitive.  If this primitive
-- was using a shared color, then it creates a new, personal color.
-- @param r The red value, or a quoted color name, like "green".
-- @param g The green value, or nil if using a name.
-- @param b The blue value, or nil if using a name.
function Geyser.Window:setFgColor (r,g,b)
   setFgColor(self.name, cparse(r,g,b))
end

--- Sets the background color and alpha.  If this primitive
-- was using a shared color, then it creates a new, personal color.
-- @param r The red component of the color, or a named color like "green".
-- @param g The green component, or nil if using named colors.
-- @param b The blue component, or nil if using named colors.
-- @param a The alpha component. If nil, uses current alpha value.
function Geyser.Window:setColor (r,g,b,a)
   setBackgroundColor(self.name, cparse(r,g,b,a))
end

--- Pastes text from the clipboard into this window primitive.
function Geyser.Window:paste ()
   paste(self.name)
end

--- Sets the text format for this window. Note that the *echo()
-- functions will override these settings.
-- @param r1 The red foreground component.
-- @param g1 The green foreground component.
-- @param b1 The blue foreground component.
-- @param r2 The red background component.
-- @param g2 The green background component.
-- @param b2 The blue background component.
-- @param bold The bolded status. 1 is bold, 0 is normal.
-- @param underline The underlined status. 1 is underlined, 0 is normal.
-- @param italics The italicized status. 1 is italicized, 0 is normal.
function Geyser.Window:setTextFormat(r1, g1, b1, r1, g2, b2, bold, underline, italics)
   setTextFormat(self.name, r1, g1, b1, r1, g2, b2, bold, underline, italics)
end

--- Sets bolded text.
-- @param bool True for bold.
function Geyser.Window:setBold(bool)
   setBold(self.name, bool)
end

--- Sets underlined text.
-- @param bool True for underlined.
function Geyser.Window:setUnderline(bool)
   setUnderline(self.name, bool)
end

--- Sets italicized text.
-- @param bool True for italicized.
function Geyser.Window:setItalics(bool)
   setItalics(self.name, bool)
end

-- Save a reference to our parent's constructor
Geyser.Window.parent = Geyser.Container

function Geyser.Window:new (cons, container)
   -- Initiate and set Window specific things
   cons = cons or {}
   cons.type = cons.type or "window"

   -- Call parent's constructor
   local me = self.parent:new(cons, container)

   -- Set the metatable.
   setmetatable(me, self)
   self.__index = self

   --print(" New in " .. self.name .. " : " .. me.name)
   return me
end
