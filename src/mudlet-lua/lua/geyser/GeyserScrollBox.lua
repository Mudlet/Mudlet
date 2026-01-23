--- Represents a ScrollBox primitive.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Add_a_scrollable_box">Mudlet Manual</a>
-- @author guy
-- @author Edru
-- @module Geyser.ScrollBox

--- Represents a ScrollBox primitive
Geyser.ScrollBox = Geyser.Window:new({
    name = "ScrollBoxClass"
})

-- Overridden reposition for special coordination handling
function Geyser.ScrollBox:reposition()
    Geyser.calc_constraints(self, self, self.container)
    moveWindow(self.name, self:get_x(), self:get_y())
    resizeWindow(self.name, self:get_width(), self:get_height())
    self.get_x = function() return 0 end
    self.get_y = function() return 0 end
      -- deal with all children of this container
    for k, v in pairs(self.windowList) do
        if k ~= self and not v.nestLabels then
        v:reposition()
        end
    end
end

-- Save a reference to our parent constructor
Geyser.ScrollBox.parent = Geyser.Window

-- Sets the style sheet of the scrollbox.
-- @todo Not currently implemented.
-- @param css The style sheet string
-- function Geyser.ScrollBox:setStyleSheet(css)
--     css = css or self.stylesheet
--     setScrollBoxStyleSheet(self.name, css)
--     self.stylesheet = css
-- end

-- Overridden constructor
function Geyser.ScrollBox:new (cons, container)
    cons = cons or {}
    cons.type = cons.type or "scrollBox"
    
    -- Call parent's constructor
    local me = self.parent:new(cons, container)
    me.windowname = me.windowname or me.container.windowname or "main"
    
    -- Set the metatable.
    setmetatable(me, self)
    self.__index = self
    
    createScrollBox(me.windowname, me.name, me:get_x(), me:get_y(), me:get_width(), me:get_height())

    --ScrollBox needs a special windowname handling as it by itself is a "window"
    --the given windowname will be saved to the parentWindowName variable
    me.parentWindowName = me.windowname
    me.windowname = me.name
    Geyser.parentWindows = Geyser.parentWindows or {}
    Geyser.parentWindows[me.name] = me

    --ScrollBox needs special coordinate handling for the children in it
    me.get_x = function() return 0 end
    me.get_y = function() return 0 end
    
    -- if me.stylesheet then 
    --     me:setStyleSheet()
    -- end
    
    return me
end

--- Deletes the scrollbox using the C++ deleteScrollBox function
function Geyser.ScrollBox:type_delete()
    deleteScrollBox(self.name)
end

--- Overridden constructor to use add2
function Geyser.ScrollBox:new2 (cons, container)
    cons = cons or {}
    cons.useAdd2 = true
    local me = self:new(cons, container)
    return me
end
