--- Just like a normal container, only adjustable.
-- Just use it like a normal Geyser Container with some extras like:
-- moveable, adjustable size, attach to borders, minimizeable, save/load.
-- Right click on top border for menu.<br/>
-- Inspired heavily by Adjustable Label (by Jor'Mox) and EMCO (by demonnic)
-- <br/>See: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Adjustable.Container">Mudlet Manual</a>
-- @author guy
-- @author Edru
-- @module Adjustable.Container

Adjustable = Adjustable or {}

Adjustable.Container = Adjustable.Container or Geyser.Container:new({
    name = "AdjustableContainerClass",
    padding = 10,
    buttonsize = 20,
    adjLabelstyle = "background-color:rgba(0,0,0,100); border: 1px solid grey;",
    titleTxtColor = "green",
    titleFormat = "l",
    attachedMargin = 0,
})

-- Static/Default Locale Table
Adjustable.Container.Locale = Adjustable.Container.Locale or {
    connectTo = { message = "Connect To: " },
    disconnect = { message = "Disconnect " },
    top = { message = "Top" },
    bottom = { message = "Bottom" },
    left = { message = "Left" },
    right = { message = "Right" }
}

-- Registry for attached containers
Adjustable.Container.Attached = Adjustable.Container.Attached or {
    top = {}, bottom = {}, left = {}, right = {}
}

-- Internal function to add "%" to a value and round it
local function make_percent(num)
    return string.format("%.5f%%", (num * 100))
end

-- Internal function: checks mouse position and sets state
local function adjust_Info(self, label, event)
    local x, y = getMousePosition()
    local w, h = self.adjLabel:get_width(), self.adjLabel:get_height()
    local x1, y1 = x - event.x, y - event.y
    local x2, y2 = x1 + w, y1 + h
    local left, right, top, bottom = event.x <= 10, x >= x2 - 10, event.y <= 3, y >= y2 - 10
    
    if right and left then left = false end
    if top and bottom then top = false end

    if event.button ~= "LeftButton" and not self.minimized then
        if (top or bottom) and not (left or right) then
            label:setCursor("ResizeVertical")
        elseif (left or right) and not (top or bottom) then
            label:setCursor("ResizeHorizontal")
        elseif (top and left) or (bottom and right) then
            label:setCursor("ResizeTopLeft")
        elseif (top and right) or (bottom and left) then
            label:setCursor("ResizeTopRight")
        else
            label:setCursor("OpenHand")
        end
    end

    self.adjustInfo = {
        name = self.name, 
        top = top, 
        bottom = bottom, 
        left = left, 
        right = right, 
        x = x, 
        y = y, 
        move = (self.adjustInfo and self.adjustInfo.move)
    }
end

function Adjustable.Container:setTitle(text, color, format)
    self.titleFormat = format or self.titleFormat or "l"
    self.titleText = text or self.titleText or string.format("%s - Adjustable Container", self.name)
    self.titleTxtColor = color or self.titleTxtColor or "green"
    
    if self.locked and (self.connectedContainers or self.lockStyle == "standard" or self.lockStyle == "border" or self.lockStyle == "full") then
        return
    end
    self.adjLabel:echo(string.format("&nbsp;&nbsp;%s", self.titleText), self.titleTxtColor, self.titleFormat)
end

function Adjustable.Container:resetTitle()
    self.titleText = nil
    self.titleTxtColor = nil
    self.titleFormat = nil
    self:setTitle()
end

function Adjustable.Container:onClick(label, event)
    self.adjustInfo = self.adjustInfo or {}
    if label.cursorShape == "OpenHand" then
        label:setCursor("ClosedHand")
    end
    
    if event.button == "LeftButton" and not (self.locked and not self.connectedContainers) then
        if self.raiseOnClick then
            self:raiseAll()
        end
        self.adjustInfo.name = label.name
        self.adjustInfo.move = not (self.adjustInfo.right or self.adjustInfo.left or self.adjustInfo.top or self.adjustInfo.bottom)
        if self.minimized then self.adjustInfo.move = true end
        adjust_Info(self, label, event)
    end
    
    if event.button == "RightButton" then
        if self.container ~= Geyser and self.rCLabel and table.index_of(self.rCLabel.nestedLabels or {}, self.attLabel) then
            label:hideMenuLabel("attLabel")
        elseif self.container == Geyser and self.rCLabel and not table.index_of(self.rCLabel.nestedLabels or {}, self.attLabel) then
            label:showMenuLabel("attLabel") 
        end

        if self.customItemsLabel and (not self.customItemsLabel.nestedLabels or #self.customItemsLabel.nestedLabels == 0) then
            label:hideMenuLabel("customItemsLabel")
        elseif self.customItemsLabel then
            label:showMenuLabel("customItemsLabel")
        end
    end
    
    if label.onRightClick then label:onRightClick(event) end
end

function Adjustable.Container:onRelease(label, event)
    if event.button == "LeftButton" and self.adjustInfo and self.adjustInfo.name == label.name then
        if label.cursorShape == "ClosedHand" then
            label:setCursor("OpenHand")
        end
        raiseEvent(
          "AdjustableContainerRepositionFinish",
          self.name,
          self:get_width(),
          self:get_height(),
          self:get_x(),
          self:get_y()
        )
        self.adjustInfo = {}
    end
end

function Adjustable.Container:onMove(label, event)
    self.adjustInfo = self.adjustInfo or {}
    if self.locked and not self.connectedContainers then
        if label.cursorShape ~= 0 then
            label:resetCursor()
        end
        return
    end
    
    if self.adjustInfo.move == nil then
        adjust_Info(self, label, event)
    end

    if self.connectedToBorder then
        for k in pairs(self.connectedToBorder) do
            if self.adjustInfo[k] then
                label:resetCursor()
                return
            end
        end
    end

    if self.adjustInfo.x and self.adjustInfo.name == label.name then
        self:adjustBorder()
        local x, y = getMousePosition()
        local winw, winh = getMainWindowSize()
        local x1, y1, w, h = self:get_x(), self:get_y(), self:get_width(), self:get_height()
        
        if (self.container) and (self.container ~= Geyser) then
            x1, y1 = x1 - self.container:get_x(), y1 - self.container:get_y()
            winw, winh = self.container:get_width(), self.container:get_height()
        end
        
        local dx, dy = self.adjustInfo.x - x, self.adjustInfo.y - y
        local max, min = math.max, math.min
        local hasScrollBox = self.windowname and Geyser.parentWindows and Geyser.parentWindows[self.windowname] and Geyser.parentWindows[self.windowname].type == "scrollBox"
        
        if self.adjustInfo.move and not self.connectedContainers then
            label:setCursor("ClosedHand")
            local tx, ty = max(0, x1 - dx), max(0, y1 - dy)
            if not hasScrollBox then
                tx, ty = min(tx, winw - w), min(ty, winh - h)
            end
            self:move(make_percent(tx / winw), make_percent(ty / winh))
        elseif self.adjustInfo.move == false then
            local w2, h2, x2, y2 = w - dx, h - dy, x1 - dx, y1 - dy
            local tx, ty, tw, th = x1, y1, w, h
            if self.adjustInfo.top then
                ty, th = y2, h + dy
            elseif self.adjustInfo.bottom then
                th = h2
            end
            if self.adjustInfo.left then
                tx, tw = x2, w + dx
            elseif self.adjustInfo.right then
                tw = w2
            end
            tx, ty, tw, th = max(0, tx), max(0, ty), max(10, tw), max(10, th)
            if not hasScrollBox then
                tw, th = min(tw, winw), min(th, winh)
                tx, ty = min(tx, winw - tw), min(ty, winh - th)
            end
            self:move(make_percent(tx / winw), make_percent(ty / winh))
            
            local minw, minh = 0, 0
            if self.container == Geyser and not self.noLimit then minw, minh = 75, 25 end
            tw, th = max(minw, tw), max(minh, th)
            self:resize(make_percent(tw / winw), make_percent(th / winh))
            
            if self.connectedContainers then
                self:adjustConnectedContainers()
            end
        end
        self.adjustInfo.x, self.adjustInfo.y = x, y
    end
end

function Adjustable.Container:validAttachPositions()
    local winw, winh = getMainWindowSize()
    local found_positions = {}
    if (winh * 0.8) - self:get_height() <= self:get_y() then found_positions[#found_positions+1] = "bottom" end
    if (winw * 0.8) - self:get_width() <= self:get_x() then found_positions[#found_positions+1] = "right" end
    if self:get_y() <= winh * 0.2 then found_positions[#found_positions+1] = "top" end
    if self:get_x() <= winw * 0.2 then found_positions[#found_positions+1] = "left" end
    return found_positions
end

function Adjustable.Container:adjustBorder()
    local winw, winh = getMainWindowSize()
    if type(self.attached) ~= "string" then return false end

    local where = self.attached:lower()
    local valid = self:validAttachPositions()
    if not table.contains(valid, where) or self.minimized or self.hidden then 
        self:detach()
        return
    end

    if where == "right" then 
        self.borderSize = winw + self.attachedMargin - self:get_x()
    elseif where == "left" then
        self.borderSize = self:get_width() + self:get_x() + self.attachedMargin
    elseif where == "bottom" then 
        self.borderSize = winh + self.attachedMargin - self:get_y()
    elseif where == "top" then 
        self.borderSize = self:get_height() + self:get_y() + self.attachedMargin
    else
        self.attached = false
        return
    end

    local borderSize = self.borderSize
    for k, v in pairs(Adjustable.Container.Attached[where] or {}) do
        if v.borderSize and v.borderSize > borderSize then
            borderSize = v.borderSize
        end
    end
    
    local funcname = string.format("setBorder%s", where:gsub("^%l", string.upper))
    if _G[funcname] then _G[funcname](borderSize) end
end

function Adjustable.Container:adjustConnectedContainers()
    local where = self.attached
    if not where or not self.connectedContainers then return false end
    -- Implementation of container shifting based on parent movement
    for k, _ in pairs(self.connectedContainers) do
        local container = Adjustable.Container.Attached[where][k]
        if container and container ~= self then
            container:adjustBorder()
        end
    end
end

function Adjustable.Container:connectToBorder(border)
    if not self.attached or not Adjustable.Container.Attached[border] then return end
    self.connectedToBorder = self.connectedToBorder or {}
    self.connectedToBorder[border] = true
    self.connectedContainers = self.connectedContainers or {}
    for k, v in pairs(Adjustable.Container.Attached[border]) do
        v.connectedContainers = v.connectedContainers or {}
        v.connectedContainers[self.name] = true
        if self.attached == border then
            v.connectedToBorder = v.connectedToBorder or {}
            v.connectedToBorder[border] = true
            self.connectedContainers[k] = true
        end
    end
end

function Adjustable.Container:addConnectMenu()
    local label = self.adjLabel
    if not label or label:findMenuElement("Connect To: ") then return end
    
    local menuTxt = self.Locale.connectTo.message
    label:addMenuLabel("Connect To: ")
    label:findMenuElement("Connect To: "):echo(menuTxt, "nocolor", "c")
    
    local menuParent = self.rCLabel.MenuItems
    menuParent[#menuParent + 1] = {"top", "bottom", "left", "right"}
    
    label:createMenuItems()
    for _, v in ipairs(menuParent[#menuParent]) do
        local subTxt = self.Locale[v] and self.Locale[v].message or v
        label:findMenuElement("Connect To: ."..v):echo(subTxt, "nocolor")
        label:setMenuAction("Connect To: ."..v, function() 
            if closeAllLevels then closeAllLevels(self.rCLabel) end
            self:connectToBorder(v) 
        end)
    end
    
    label:addMenuLabel("Disconnect ")
    label:setMenuAction("Disconnect ", function() 
        if closeAllLevels then closeAllLevels(self.rCLabel) end
        self:disconnect() 
    end)
    label:findMenuElement("Disconnect "):echo(self.Locale.disconnect.message, "nocolor", "c")
end

function Adjustable.Container:disconnect()
    if not self.connectedToBorder then return end
    for border, _ in pairs(self.connectedToBorder) do
        for _, v1 in pairs(Adjustable.Container.Attached[border] or {}) do
            if v1.connectedContainers then
                v1.connectedContainers[self.name] = nil
            end
        end
    end
    self.connectedToBorder = nil
    self.connectedContainers = nil
end

function Adjustable.Container:setBorderMargin(margin)
    self.attachedMargin = margin
    self:adjustBorder()
end

function Adjustable.Container:resizeBorder()
    local winw, winh = getMainWindowSize()
    if (winw ~= self.old_w_value or winh ~= self.old_h_value) then
        if not self.timer_active then
            self.timer_active = true
            tempTimer(0.2, function() 
                self:adjustBorder() 
                self:adjustConnectedContainers() 
                self.timer_active = false
            end)
        end
    end
    self.old_w_value, self.old_h_value = winw, winh
end

function Adjustable.Container:attachToBorder(border)
    if self.attached then self:detach() end
    Adjustable.Container.Attached[border] = Adjustable.Container.Attached[border] or {}
    Adjustable.Container.Attached[border][self.name] = self
    self.attached = border
    self:adjustBorder()
    self.resizeHandlerID = registerAnonymousEventHandler("sysWindowResizeEvent", function() self:resizeBorder() end)
end

function Adjustable.Container:detach()
    if self.attached and Adjustable.Container.Attached[self.attached] then
        Adjustable.Container.Attached[self.attached][self.name] = nil
        self:resetBorder(self.attached)
    end
    self.borderSize = nil
    self.attached = false
    if self.resizeHandlerID then killAnonymousEventHandler(self.resizeHandlerID) end
end

function Adjustable.Container:resetBorder(where)
    local resetTo = 0
    for _, v in pairs(Adjustable.Container.Attached[where] or {}) do
        if v.borderSize and v.borderSize > resetTo then
            resetTo = v.borderSize
        end
    end
    local func = _G["setBorder" .. where:gsub("^%l", string.upper)]
    if func then func(resetTo) end
end

function Adjustable.Container:createContainers()
    self.adjLabel = Geyser.Label:new({
        x = 0, y = 0, height = "100%", width = "100%",
        name = self.name.."adjLabel"
    }, self)
    self.adjLabel:setStyleSheet(self.adjLabelstyle)
    
    self.Inside = Geyser.Container:new({
        x = self.padding, y = self.padding * 2,
        height = "-" .. self.padding, width = "-" .. self.padding,
        name = self.name.."InsideContainer"
    }, self)
end

function Adjustable.Container:lockContainer(lockNr, lockStyle)
    if type(lockNr) == "string" then lockStyle = lockNr end
    self.lockStyle = lockStyle or "standard"
    self.locked = true
    self.exitLabel:hide()
    self.minimizeLabel:hide()
    self:adjustBorder()
end

function Adjustable.Container:unlockContainer()
    self.locked = false
    self.Inside:resize("-"..self.padding, "-"..self.padding)
    self.Inside:move(self.padding, self.padding*2)
    self.exitLabel:show()
    self.minimizeLabel:show()
    self:setTitle()
end

function Adjustable.Container:minimize()
    if self.minimized then return end
    self.origh = self.height
    self.Inside:hide()
    self:resize(nil, self.buttonsize + 10)
    self.minimized = true
    self:adjustBorder()
end

function Adjustable.Container:restore()
    if not self.minimized then return end
    self.Inside:show()
    self:resize(nil, self.origh or "25%")
    self.minimized = false
    self:adjustBorder()
end

function Adjustable.Container:new(cons, container)
    local me = Geyser.Container.new(self, cons, container)
    setmetatable(me, self)
    me:createContainers()
    me:createLabels()
    me:setTitle()
    return me
end

function Adjustable.Container:createLabels()
    self.exitLabel = Geyser.Label:new({
        x = -(self.buttonsize * 1.5), y = 2, 
        width = self.buttonsize, height = self.buttonsize,
        name = self.name.."ExitLabel"
    }, self.adjLabel)
    self.exitLabel:echo("<center>X")
    self.exitLabel:setClickCallback(function() self:hide() end)

    self.minimizeLabel = Geyser.Label:new({
        x = -(self.buttonsize * 2.8), y = 2, 
        width = self.buttonsize, height = self.buttonsize,
        name = self.name.."MinLabel"
    }, self.adjLabel)
    self.minimizeLabel:echo("<center>_")
    self.minimizeLabel:setClickCallback(function() 
        if self.minimized then self:restore() else self:minimize() end 
    end)
    
    -- Connect mouse events to the main label
    self.adjLabel:setClickCallback(function(l, e) self:onClick(self.adjLabel, e) end)
    self.adjLabel:setReleaseCallback(function(l, e) self:onRelease(self.adjLabel, e) end)
    self.adjLabel:setMoveCallback(function(l, e) self:onMove(self.adjLabel, e) end)
end
