--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Represents a label like we all know and love.
-- @class table
-- @name Geyser.Label
-- @field fillBg 1 if the background is to be filled, 0 for no background.
Geyser.Label = Geyser.Window:new({
      name = "LabelClass",
      format = "",
      args = "",
      fillBg = 1, })

--- Prints a message to the window.  All parameters are optional and if not
-- specified will use the last set value.
-- @param message The message to print. Can contain html formatting.
-- @param color The color to use.
-- @param format A format list to use. 'c' - center, 'b' - bold, 'i' - italics,
--               'u' - underline, '##' - font size.  For example, "cb18" 
--               specifies center bold 18pt font be used.  Order doesn't matter.
function Geyser.Label:echo(message, color, format)
   message = message or self.message
   self.message = self.message
   format = format or self.format
   self.format = format
   color = color or self.fgColor
   self.fgColor = color
   
   local fs = ""
   -- check for formatting commands
   if format then
      if string.find(format, "b") then message = "<b>" .. message .. "</b>" end
      if string.find(format, "i") then message = "<i>" .. message .. "</i>" end
      if string.find(format, "c") then message = "<center>" .. message .. "</i>" end
      if string.find(format, "u") then message = "<u>" .. message .. "</u>" end
      fs = string.gmatch(format, "%d+")()
      if not fs then fs = tostring(self.fontSize) end
      fs = "font-size: " .. fs .. "pt; "
   end
   message = [[<div style="color: ]] .. Geyser.Color.hex(self.fgColor) .. "; " .. fs ..
             [[">]] .. message .. [[</div>]]
   echo(self.name, message)
end

function Geyser.Label:setFgColor(color)
   self:echo(nil,color,nil)
end

function Geyser.Label:setFormat(format)
   self:echo(nil,nil,format)
end

function Geyser.Label:clear()
   echo(self.name, "")
end

--- Sets a background image for this label.
-- @param imageFileName The image to use for a background image.
function Geyser.Label:setBackgroundImage (imageFileName)
   setBackgroundImage(self.name, imageFileName)
end

--- Sets a tiled background image for this label.
-- @param imageFileName The image to use for a background image.
function Geyser.Label:setTiledBackgroundImage (imageFileName)
   self:setStyleSheet("background-image: url(" .. imageFileName .. ");")
end

--- Sets a callback to be used when this label is clicked.
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setClickCallback (func, ...)
   setLabelClickCallback(self.name, func, ...)
   self.callback = func
   self.args = {...}
end

--- Sets the style sheet of the label
-- @param css The style sheet string
function Geyser.Label:setStyleSheet(css)
        setLabelStyleSheet(self.name, css)
end

-- Save a reference to our parent constructor
Geyser.Label.parent = Geyser.Window

-- Overridden constructor
function Geyser.Label:new (cons, container)
   -- Initiate and set label specific things
   cons = cons or {}
   cons.type = cons.type or "label"

   -- Call parent's constructor
   local me = self.parent:new(cons, container)

   -- Set the metatable.
   setmetatable(me, self)
   self.__index = self

   -- Create the label using primitives
   createLabel(me.name, me:get_x(), me:get_y(),
               me:get_width(), me:get_height(), me.fillBg)

   -- Set any defined colors
   Geyser.Color.applyColors(me)
   me:echo()

   -- Set up the callback if we have one
   if me.callback then
      me:setClickCallback(me.callback, unpack(me.args))
   end

   --print("  New in " .. self.name .. " : " .. me.name)
   return me
end

