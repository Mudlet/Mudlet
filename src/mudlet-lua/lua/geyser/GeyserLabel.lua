-- Label class to use CSS and images

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
Geyser.Label.currentLabel = nil
Geyser.Label.scrollV = {}
Geyser.Label.scrollH = {}
Geyser.Label.numChildren = 0
--- Prints a message to the window.  All parameters are optional and if not
-- specified will use the last set value.
-- @param message The message to print. Can contain html formatting.
-- @param color The color to use.
-- @param format A format list to use. 'c' - center, 'l' - left, 'r' - right,  'b' - bold, 'i' - italics, 'u' - underline, 's' - strikethrough,  '##' - font size.  For example, "cb18" specifies center bold 18pt font be used.  Order doesn't matter.
function Geyser.Label:echo(message, color, format)
  message = message or self.message
  self.message = message
  format = format or self.format
  self.format = format
  color = color or self.fgColor
  self.fgColor = color

  local fs = ""
  local alignment = ""
  -- check for formatting commands
  if format then
    if string.find(format, "b") then
      message = "<b>" .. message .. "</b>"
    end
    if string.find(format, "i") then
      message = "<i>" .. message .. "</i>"
    end
    if string.find(format, "c") then
      alignment = "center"
    elseif string.find(format, "l") then
      alignment = "left"
    elseif string.find(format, "r") then
      alignment = "right"
    end
    if alignment ~= "" then
      alignment = string.format([[align="%s" ]], alignment)
    end
    if string.find(format, "u") then
      message = "<u>" .. message .. "</u>"
    end
    if string.find(format, 's') then
      message = "<s>" .. message .. "</s>"
    end
    fs = string.gmatch(format, "%d+")()
    if not fs then
      fs = tostring(self.fontSize)
    end
    fs = "font-size: " .. fs .. "pt; "
  end
  message = [[<div ]] .. alignment .. [[ style="color: ]] .. Geyser.Color.hex(self.fgColor) .. "; " .. fs ..
  [[">]] .. message .. [[</div>]]
  echo(self.name, message)
end

function Geyser.Label:setFgColor(color)
  self:echo(nil, color, nil)
end

function Geyser.Label:setFormat(format)
  self:echo(nil, nil, format)
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

--- Sets a callback to be used when this label is clicked. When this
-- function is called by the event system, details of the event will be
-- appended as the final argument (see @{mouseClickEvent})
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setClickCallback (func, ...)
  setLabelClickCallback(self.name, func, ...)
  self.clickCallback = func
  self.clickArgs = { ... }
end

--- Sets a callback to be used when this label is double clicked. When this
-- function is called by the event system, details of the event will be
-- appended as the final argument (see @{mouseClickEvent})
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setDoubleClickCallback (func, ...)
  setLabelDoubleClickCallback(self.name, func, ...)
  self.doubleclickCallback = func
  self.doubleclickArgs = { ... }
end

--- Sets a callback to be used when a mouse click is released over this label. When this
-- function is called by the event system, details of the event will be
-- appended as the final argument (see @{mouseClickEvent})
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setReleaseCallback (func, ...)
  setLabelReleaseCallback(self.name, func, ...)
  self.releaseCallback = func
  self.releaseArgs = { ... }
end

--- Sets a callback to be used when the mouse cursor is moved over this label. When this
-- function is called by the event system, details of the event will be
-- appended as the final argument (see @{mouseClickEvent})
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setMoveCallback (func, ...)
  setLabelMoveCallback(self.name, func, ...)
  self.moveCallback = func
  self.moveArgs = { ... }
end

--- Sets a callback to be used when the user scrolls over this label. When this
-- function is called by the event system, details of the event will be
-- appended as the final argument (see @{mouseWheelEvent})
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setWheelCallback (func, ...)
  setLabelWheelCallback(self.name, func, ...)
  self.wheelCallback = func
  self.wheelArgs = { ... }
end

--- Sets a callback to be used when the mouse passes over this label.
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setOnEnter (func, ...)
  setLabelOnEnter(self.name, func, ...)
end

--- Sets a callback to be used when the mouse leaves this label.
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setOnLeave (func, ...)
  setLabelOnLeave(self.name, func, ...)
end


--- Sets the style sheet of the label
-- @param css The style sheet string
function Geyser.Label:setStyleSheet(css)
  setLabelStyleSheet(self.name, css)
end

--- Returns the Geyser object associated with the label name
-- @param label The name of the label to use
function Geyser.Label:getWindow(label)
  for i, v in pairs(Geyser.windowList) do
    if v.name == label then
      return v
    end

    -- search down one level to enable nesting in a container
    for key, val in pairs(v.windowList) do
      if val.name == label then
        return val
      end
    end
  end
end

--- closes all nested labels
function closeAllLevels()
  for i, v in pairs(Geyser.Label.scrollV) do
    v[1]:hide()
    v[2]:hide()
  end
  for i, v in pairs(Geyser.Label.scrollH) do
    v[1]:hide()
    v[2]:hide()
  end
  for i, v in pairs(Geyser.windowList) do
    if v.nestParent then
      v:hide()
    end
  end
end

--- Closes all nested labels under the given label, including any
--- nested children those children might possess
-- @param label The name of the label to use
function closeNestChildren(label)
  local nLabels = Geyser.Label:getWindow(label).nestedLabels
  if nLabels then
    for i, v in pairs(nLabels) do
      v:hide()
      if v.nestedLabels then
        closeNestChildren(v.name)
      end
      if Geyser.Label.scrollV[v.nestParent] then
        Geyser.Label.scrollV[v.nestParent][1]:hide()
        Geyser.Label.scrollV[v.nestParent][2]:hide()
      end
      if Geyser.Label.scrollH[v.nestParent] then
        Geyser.Label.scrollH[v.nestParent][1]:hide()
        Geyser.Label.scrollH[v.nestParent][2]:hide()
      end
    end
  end
end

--- Internal function.  This is a timer callback from a nested
--- labels OnLeave function which takes care of renesting
--- labels
-- @param label The name of the label to use
function closeNest(label)
  --if we aren't in any label, close em all
  if not Geyser.Label.currentLabel then
    closeAllLevels()
    return
  end
  --is the current label on the same level of the prior label?
  local lParent = Geyser.Label:getWindow(label).nestParent
  local cLabel = Geyser.Label:getWindow(Geyser.Label.currentLabel)
  if not cLabel then
    return
  end
  local cParent = cLabel.nestParent
  if lParent and cParent then
    if lParent == cParent then
      --if so, don't do anything, but close any fly outs of the label
      --echo("on same level\n")
      closeNestChildren(label)
      return
    end
  end
  --is the current label a nested element of the prior table?
  local lNestLabels = Geyser.Label:getWindow(label).nestedLabels
  if lNestLabels then
    for i, v in pairs(lNestLabels) do
      if v.name == Geyser.Label.currentLabel then
        --  echo("new element is nested of prior table\n")
        --if so, don't do anything
        return
      end
    end
  end
  --is the current label the parent of the prior label?
  if (lParent.name ~= Geyser.Label.currentLabel) then
    -- echo("new element isn't parent of prior element\n")
    closeNestChildren(lParent.name)
  end
end

--- Internal function.  This is a callback from a nested
--- labels scrollbar.
-- @param label The name of the scrollbar
function doNestScroll(label)
  local scrollDir = 0
  if string.find(label, "forScroll") then
    scrollDir = 1
  else
    scrollDir = -1
  end
  local bothScrolls
  if (string.sub(label, -1, -1) == "V") then
    bothScrolls = Geyser.Label.scrollV[Geyser.Label:getWindow(label).nestParent]
  else
    bothScrolls = Geyser.Label.scrollH[Geyser.Label:getWindow(label).nestParent]
  end
  local bscroll = bothScrolls[1]
  local fscroll = bothScrolls[2]
  local scrollDiff = fscroll.scroll - bscroll.scroll
  fscroll.scroll = fscroll.scroll + scrollDir
  bscroll.scroll = bscroll.scroll + scrollDir
  if bscroll.scroll < 0 then
    bscroll.scroll = 0
    fscroll.scroll = scrollDiff
  end
  if fscroll.scroll >= fscroll.maxScroll then
    fscroll.scroll = fscroll.maxScroll
    bscroll.scroll = fscroll.scroll - scrollDiff
  end
  Geyser.Label:displayNest(bscroll.nestParent.name)
end

--- Displays the nested elements within label, and orients them
--- appropiately
-- @param label The name of the label to use
function Geyser.Label:displayNest(label)
  local maxDim = {}
  local flyMap = { R = { 1, 0 }, L = { -1, 0 }, T = { 0, -1 }, B = { 0, 1 } }
  maxDim["H"], maxDim["V"] = getMainWindowSize()
  local parent = Geyser.Label:getWindow(label)
  --build a list of the labels we can use until we hit the max
  local nestedLabels = {}
  nestedLabels["V"] = {}
  nestedLabels["H"] = {}
  local layout = {}
  layout["V"] = 0
  layout["H"] = 0
  local scrollStartV, scrollEndV, scrollStartH, scrollEndH = nil, nil, nil, nil
  local scrollV, scrollH = nil, nil
  if Geyser.Label.scrollV[parent] then
    scrollStartV = Geyser.Label.scrollV[parent][1].scroll
    scrollEndV = Geyser.Label.scrollV[parent][2].scroll
  end
  if Geyser.Label.scrollH[parent] then
    scrollStartH = Geyser.Label.scrollH[parent][1].scroll
    scrollEndH = Geyser.Label.scrollH[parent][2].scroll
  end
  local entryCount = { V = 0, H = 0 }
  parent.nestedLabels = parent.nestedLabels or {}
  for i, v in pairs(parent.nestedLabels) do
    entryCount[v.layoutDir] = entryCount[v.layoutDir] + 1
    if v.layoutDir == "V" and not scrollV then
      if layout[v.layoutDir] + v.get_height() <= maxDim[v.layoutDir] then
        if not scrollStartV or (entryCount[v.layoutDir] > scrollStartV and entryCount[v.layoutDir] < scrollEndV) then
          table.insert(nestedLabels[v.layoutDir], v)
          layout[v.layoutDir] = layout[v.layoutDir] + v.get_height()
        end
      else
        scrollV = true
        table.remove(nestedLabels[v.layoutDir])
        table.remove(nestedLabels[v.layoutDir])
        v:hide()
      end
    elseif v.layoutDir == "H" and not scrollH then
      if layout[v.layoutDir] + v.get_height() <= maxDim[v.layoutDir] then
        if not scrollStartH or (entryCount[v.layoutDir] > scrollStartH and entryCount[v.layoutDir] < scrollEndH) then
          table.insert(nestedLabels[v.layoutDir], v)
          layout[v.layoutDir] = layout[v.layoutDir] + v.get_width()
        end
      else
        scrollH = true
        table.remove(nestedLabels[v.layoutDir])
        table.remove(nestedLabels[v.layoutDir])
        v:hide()
      end
    end
  end
  --see how far we need to offset to the top or to the left to fit what we're displaying
  local parX = parent.get_x()
  local parY = parent.get_y()
  local parH = parent.get_height()
  local parW = parent.get_width()
  local xOffset, yOffset = 0, 0
  if scrollV or (Geyser.Label.scrollV and Geyser.Label.scrollV[parent]) then
    if not Geyser.Label.scrollV[parent] then
      Geyser.Label.scrollV[parent] = Geyser.Label:addScrollbars(nestedLabels["V"][1].nestParent, nestedLabels["V"][1].flyDir .. nestedLabels["V"][1].layoutDir)
      local numLabels = #nestedLabels["V"]
      Geyser.Label.scrollV[parent][1].scroll = 0
      Geyser.Label.scrollV[parent][2].scroll = numLabels + 1
    end
    table.insert(nestedLabels["V"], 1, Geyser.Label.scrollV[parent][1])
    table.insert(nestedLabels["V"], Geyser.Label.scrollV[parent][2])
    layout["V"] = layout["V"] + Geyser.Label.scrollV[parent][1].get_height() + Geyser.Label.scrollV[parent][2].get_height()
  end
  if scrollH or (Geyser.Label.scrollH and Geyser.Label.scrollH[parent]) then
    if not Geyser.Label.scrollH[parent] then
      Geyser.Label.scrollH[parent] = Geyser.Label:addScrollbars(nestedLabels["H"][1].nestParent, nestedLabels["H"][1].flyDir .. nestedLabels["H"][1].layoutDir)
      local numLabels = #nestedLabels["H"]
      Geyser.Label.scrollH[parent][1].scroll = 0
      Geyser.Label.scrollH[parent][2].scroll = numLabels + 1
    end
    table.insert(nestedLabels["H"], 1, Geyser.Label.scrollH[parent][1])
    table.insert(nestedLabels["H"], Geyser.Label.scrollH[parent][2])
    layout["H"] = layout["H"] + Geyser.Label.scrollH[parent][1].get_width() + Geyser.Label.scrollH[parent][2].get_width()
  end
  if layout["H"] > maxDim["H"] then
    xOffset = parX
  elseif layout["H"] > (maxDim["H"] - parX) then
    xOffset = parX - (maxDim["H"] - layout["H"])
  end
  if layout["V"] > maxDim["V"] then
    yOffset = parY
  elseif layout["V"] > (maxDim["V"] - parY) then
    yOffset = parY - (maxDim["V"] - layout["V"])
  end
  local flyIndex = { R = 0, L = 0, T = 0, B = 0 }
  for i, v in pairs(nestedLabels["V"]) do
    local width = v.get_width()
    local height = v.get_height()
    v.x = parX + flyMap[v.flyDir][1] * parW
    v.y = parY + flyMap[v.flyDir][2] * parH - yOffset + height * flyIndex[v.flyDir]
    v:show()
    moveWindow(v.name, v.x, v.y)
    v:set_constraints()
    flyIndex[v.flyDir] = flyIndex[v.flyDir] + 1
  end
  local flyIndex = { R = 0, L = 0, T = 0, B = 0 }
  for i, v in pairs(nestedLabels["H"]) do
    v.x = parX + flyMap[v.flyDir][1] * parW - xOffset + v.get_width() * flyIndex[v.flyDir]
    v.y = parY + flyMap[v.flyDir][2] * parH
    v:show()
    moveWindow(v.name, v.x, v.y)
    v:set_constraints()
    flyIndex[v.flyDir] = flyIndex[v.flyDir] + 1
  end
end

--- Internal function when a parent nest element is clicked
--- to lay out the nested elements within
-- @param label The name of the label to use
function doNestShow(label)
  Geyser.Label:displayNest(label)
end

--- Internal function when a nested element is moused over
--- to lay out the nested elements within that nested element
--- only active if flyOut is true
-- @param label The name of the label to use
function doNestEnter(label)
  local window = Geyser.Label:getWindow(label)
  --echo("entering window"..window.name.."\n")
  --Geyser.display(window)
  Geyser.Label.currentLabel = label
  if window and window.nestedLabels then
    Geyser.Label:displayNest(label)
  end
end

--- Internal function when a nested element is left
--- to renest elements and restore order
-- @param label The name of the label to use
function doNestLeave(label)
  if Geyser.Label.currentLabel == label then
    Geyser.Label.currentLabel = nil
  end
  tempTimer(0.1, "closeNest(\"" .. label .. "\")")
end

-- Save a reference to our parent constructor
Geyser.Label.parent = Geyser.Window

-- Overridden constructor
function Geyser.Label:new (cons, container)
  -- Initiate and set label specific things
  cons = cons or {}
  cons.type = cons.type or "label"
  cons.nestParent = cons.nestParent or nil

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

  -- Set up mouse hover as the callback if we have one
  if cons.nestflyout then
    setLabelOnEnter(me.name, "doNestShow", me.name)
  end
  -- Set up the callback if we have one
  if cons.nestable then
    setLabelClickCallback(me.name, "doNestShow", me.name)
  end
  if me.clickCallback then
    if type(me.clickArgs) == "string" or type(me.clickArgs) == "number" then
      me:setClickCallback(me.clickCallback, me.clickArgs)
    elseif type(me.clickArgs) == "table" then
      me:setClickCallback(me.clickCallback, unpack(me.clickArgs))
    else
      me:setClickCallback(me.clickCallback)
    end
  end

  if me.doubleClickCallback then
    if type(me.doubleClickArgs) == "string" or type(me.doubleClickArgs) == "number" then
      me:setDoubleClickCallback(me.doubleClickCallback, me.doubleClickArgs)
    elseif type(me.doubleClickArgs) == "table" then
      me:setDoubleClickCallback(me.doubleClickCallback, unpack(me.doubleClickArgs))
    else
      me:setDoubleClickCallback(me.doubleClickCallback)
    end
  end

  if me.releaseCallback then
    if type(me.releaseArgs) == "string" or type(me.releaseArgs) == "number" then
      me:setReleaseCallback(me.releaseCallback, me.releaseArgs)
    elseif type(me.releaseArgs) == "table" then
      me:setReleaseCallback(me.releaseCallback, unpack(me.releaseArgs))
    else
      me:setReleaseCallback(me.releaseCallback)
    end
  end

  if me.moveCallback then
    if type(me.moveArgs) == "string" or type(me.moveArgs) == "number" then
      me:setMoveCallback(me.moveCallback, me.moveArgs)
    elseif type(me.moveArgs) == "table" then
      me:setMoveCallback(me.moveCallback, unpack(me.moveArgs))
    else
      me:setMoveCallback(me.moveCallback)
    end
  end

  if me.wheelCallback then
    if type(me.wheelArgs) == "string" or type(me.wheelArgs) == "number" then
      me:setWheelCallback(me.wheelCallback, me.wheelArgs)
    elseif type(me.wheelArgs) == "table" then
      me:setWheelCallback(me.wheelCallback, unpack(me.wheelArgs))
    else
      me:setWheelCallback(me.wheelCallback)
    end
  end


  if me.onEnter then
    me:setOnEnter(me.onEnter, me.args)
  end

  if me.onLeave then
    me:setOnLeave(me.onLeave, me.args)
  end
  
  -- Set clickthrough if included in constructor
  if cons.clickthrough then me:enableClickthrough() end
  
  --print("  New in " .. self.name .. " : " .. me.name)
  return me
end

function fakeFunction()
end

--- internal function that adds the "More..." scrollbars
function Geyser.Label:addScrollbars(parent, layout)
  local label = parent.nestedLabels[1]
  local flyDir, layoutDir
  flyDir = string.sub(layout, 1, 1)
  layoutDir = string.sub(layout, 2, 2)
  cons = { name = "forScroll" .. label.name .. layout, x = label:get_x(), y = label:get_y(),
    width = label:get_width(), layoutDir = layoutDir, flyDir = flyDir, height = label:get_height(), message = "More..." }
  local forward = Geyser.Label:new(cons, parent.container)
  forward.nestParent = parent
  forward.maxScroll = #parent.nestedLabels + 1
  setLabelOnEnter(forward.name, "doNestEnter", forward.name)
  setLabelOnLeave(forward.name, "doNestLeave", forward.name)
  forward:setClickCallback("doNestScroll", forward.name)
  cons.name = "backScroll" .. label.name .. layout
  local backward = Geyser.Label:new(cons, label.container)
  backward.nestParent = parent
  setLabelOnEnter(backward.name, "doNestEnter", backward.name)
  setLabelOnLeave(backward.name, "doNestLeave", backward.name)
  backward:setClickCallback("doNestScroll", backward.name)
  return { backward, forward }
end

---@param cons table of Geyser window options such as name, width, and height
-- @param cons.name a unique name for the label
-- @param cons.height height of the label - specify it as the defaults are huge
-- @param cons.width width of the label - specify it as the defaults are huge
-- @param[opt='LV'] cons.layoutDir specifies in which direction and axis should the labels align, where 2 letters combine into the option: first letter R for right, L for left, T for top, B for bottom, followed by the orientation: V for vertical or H for horizontal. So options are: layoutDir="RV", layoutDir="RH", layoutDir="LV", layoutDir="LH", and so on
-- @param[opt=false] cons.flyOut allows labels to show up when mouse is hovered over
-- @param[opt=''] cons.message initial message to show on the label
-- @param[opt='white'] cons.fgColor optional foreground colour - colour to use for text on the label
-- @param[opt='black'] cons.bgColor optional background colour - colour of the whole label
-- @param[opt=1] cons.fillBg 1 if the background is to be filled, 0 for no background
function Geyser.Label:addChild(cons, container)
  cons = cons or {}
  cons.type = cons.type or "nestedLabel"
  local flyOut = false
  local flyDir, layoutDir
  if cons.layoutDir then
    flyDir = string.sub(cons.layoutDir, 1, 1)
    layoutDir = string.sub(cons.layoutDir, 2, 2)
  else
    flyDir = "L"
    layoutDir = "V"
  end
  Geyser.Label.numChildren = Geyser.Label.numChildren + 1
  if not cons.name then
    cons.name = Geyser.Label.numChildren
  end
  local me = Geyser.Label:new(cons, container)
  --this is our parent
  me.nestParent = self
  if cons.flyOut == true then
    setLabelOnEnter(me.name, "doNestEnter", me.name)
    setLabelOnLeave(me.name, "doNestLeave", me.name)
  end
  if me.clickCallback then
    me:setClickCallback(me.clickCallback, me.clickArgs)
  else
    --used in instances where an element only meant to serve as
    --a nest container is clicked on.  Without this, we get
    --seg faults
    me:setClickCallback("fakeFunction")
  end
  if me.releaseCallback then
    me:setReleaseCallback(me.releaseCallback, me.releaseArgs)
  else
    --used in instances where an element only meant to serve as
    --a nest container is released over.  Without this, we get
    --seg faults
    me:setReleaseCallback("fakeFunction")
  end
  me.flyDir = flyDir
  me.layoutDir = layoutDir
  self.nestedLabels = self.nestedLabels or {}
  for i, v in pairs(self.nestedLabels) do
    if v.name == me.name then
      self.nestedLabels[i] = nil
      break
    end
  end
  table.insert(self.nestedLabels, me)
  me:hide()
  return me
end

--- Sets label to no longer intercept mouse events
function Geyser.Label:enableClickthrough()
  enableClickthrough(self.name)
end

--- Sets label to once again intercept mouse events
function Geyser.Label:disableClickthrough()
  disableClickthrough(self.name)
end

---
-- The table returned by @{setClickCallback}
-- @field x The x coordinate of the click local to the label
-- @field y The y coordinate of the click local to the label
-- @field globalX The global x coordinate of the click
-- @field globalY The global y coordinate of the click
-- @field button A string corresponding to the button clicked
-- @field buttons A table of strings correspinding to additional buttons held down during the click event
-- @table mouseClickEvent

---
-- The table returned by @{setWheelCallback}
-- @field x The x coordinate of the click local to the label
-- @field y The y coordinate of the click local to the label
-- @field globalX The global x coordinate of the click
-- @field globalY The global y coordinate of the click
-- @field buttons A table of strings correspinding to additional buttons held down during the click event
-- @field angleDeltaX A number corresponding with the vertical wheel motion. For most devices, this number is in increments of 120
-- @field angleDeltaY A number corresponding with the horizontal wheel motion. For most devices, this number is in increments of 120
-- @table mouseWheelEvent
