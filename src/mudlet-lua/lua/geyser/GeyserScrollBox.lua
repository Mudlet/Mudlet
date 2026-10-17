--- Represents a ScrollBox primitive.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Add_a_scrollable_box">Mudlet Manual</a>
-- @author guy
-- @author Edru
-- @module Geyser.ScrollBox

--- Represents a ScrollBox primitive
Geyser.ScrollBox = Geyser.Window:new({
    name = "ScrollBoxClass"
})

-- Internal function: children are laid out inside the box's own widget, so from
-- its origin and at the whole pixel size the widget gets, not its fractional one.
-- The size still follows the box's constraints, which a parent HBox or VBox reads
-- and rewrites before it repositions the box.
local function frameChildren(box)
    local own = {get_x = box.get_x, get_y = box.get_y, get_width = box.get_width, get_height = box.get_height}
    local zero = function() return 0 end
    local across = {get_x = own.get_x, get_y = zero, get_width = own.get_width, get_height = zero}
    local down = {get_x = zero, get_y = own.get_y, get_width = zero, get_height = own.get_height}
    -- a negative width or height is compiled as the far edge less the box's own
    -- get_x or get_y, so those must give the box's real origin while it measures itself
    local measuring = false
    box.get_x = function() return measuring and own.get_x() or 0 end
    box.get_y = function() return measuring and own.get_y() or 0 end
    box.get_width = function()
        measuring = true
        local _, _, width = Geyser.Container.pixel_geometry(across)
        measuring = false
        return width
    end
    box.get_height = function()
        measuring = true
        local _, _, _, height = Geyser.Container.pixel_geometry(down)
        measuring = false
        return height
    end
end

-- Overridden reposition for special coordination handling
function Geyser.ScrollBox:reposition(skipChildren)
    Geyser.calc_constraints(self, self, self.container)
    local x, y, width, height = self:pixel_geometry()
    moveWindow(self.name, x, y)
    resizeWindow(self.name, width, height)
    frameChildren(self)
      -- deal with all children of this container
    if not skipChildren then
        for k, v in pairs(self.windowList) do
            if k ~= self and not v.nestLabels then
            v:reposition()
            end
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
    local taken = mudlet.nameTakenByOtherKind(cons.name, "scrollbox", cons.windowname or (container or Geyser).windowname)
    if taken then
        printError(string.format("Geyser.ScrollBox '%s' was not created: %s", cons.name, taken), false, false)
        return nil, taken
    end
    
    -- Call parent's constructor
    local me = self.parent:new(cons, container)
    me.windowname = me.windowname or me.container.windowname or "main"
    
    -- Set the metatable.
    setmetatable(me, self)
    self.__index = self
    
    local x, y, width, height = me:pixel_geometry()
    local ok, err = createScrollBox(me.windowname, me.name, x, y, width, height)
    -- the object is returned and registered as a parent window either way, so report a failed creation
    if not mudlet.elementCreated(me.windowname, me.name, ok, err, "scrollbox") then
        printError(string.format("Geyser.ScrollBox '%s' was not created: %s", me.name, err or "unknown error"), false, false)
    end

    -- Geyser.Container:new() settles the hidden constraint before there is a widget to hide, so the hide is made good here
    if me.hidden or me.auto_hidden then
        hideWindow(me.name)
    end

    --ScrollBox needs a special windowname handling as it by itself is a "window"
    --the given windowname will be saved to the parentWindowName variable
    me.parentWindowName = me.windowname
    me.windowname = me.name
    Geyser.parentWindows = Geyser.parentWindows or {}
    Geyser.parentWindows[me.name] = me

    --ScrollBox needs special coordinate handling for the children in it
    frameChildren(me)
    
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
    return self:new(cons, container)
end
