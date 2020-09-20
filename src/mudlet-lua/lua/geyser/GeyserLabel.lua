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
  font = "",
  args = "",
  fillBg = 1, })
Geyser.Label.scrollV = {}
Geyser.Label.scrollH = {}
--- Prints a message to the window.  All parameters are optional and if not
-- specified will use the last set value.
-- @param message The message to print. Can contain html formatting.
-- @param color The color to use. If no color formatting is needed it is possible to use 'nocolor' which allows color formatting by using :setStyleSheet
-- @param format A format list to use. 'c' - center, 'l' - left, 'r' - right,  'b' - bold, 'i' - italics, 'u' - underline, 's' - strikethrough,  '##' - font size.  For example, "cb18" specifies center bold 18pt font be used.  Order doesn't matter.
function Geyser.Label:echo(message, color, format)
  message = message or self.message
  self.message = message
  color = color or self.fgColor
  self.fgColor = color
  if format then self:processFormatString(format) end

  local ft = self.formatTable
  local fs = ft.fontSize
  local alignment = ft.alignment
  if alignment ~= "" then
    alignment = string.format([[align="%s" ]], alignment)
  end
  if ft.bold then
    message = "<b>" .. message .. "</b>"
  end
  if ft.italics then
    message = "<i>" .. message .. "</i>"
  end
  if ft.underline then
    message = "<u>" .. message .. "</u>"
  end
  if ft.strikethrough then
    message = "<s>" .. message .. "</s>"
  end
  if self.font and self.font ~= "" then
    message = string.format('<font face ="%s">%s</font>', self.font, message)
  end
  if not fs then
    fs = tostring(self.fontSize)
  end
  if color == "nocolor" then
    color = [[ style=" ]]
  else
    color = [[ style="color: ]] .. Geyser.Color.hex(self.fgColor) .. [[; ]]
  end
  fs = "font-size: " .. fs .. "pt; "
  message = [[<div ]] .. alignment .. color .. fs ..
  [[">]] .. message .. [[</div>]]
  echo(self.name, message)
end

--- raw Echo without formatting/handholding stuff that Geyser.Label:echo() does
-- @param message The message to print. Can contain html formatting.
function Geyser.Label:rawEcho(message)
  echo(self.name, message)
end

function Geyser.Label:setFgColor(color)
  self:echo(nil, color, nil)
end

function Geyser.Label:setFormat(format)
  self:echo(nil, nil, format)
end


-- Internal function used for processing format strings.
function Geyser.Label:processFormatString(format)
  local formatType = type(format)
  assert(formatType == "string", "format as string expected, got " .. formatType)
  self.format = format
  self.formatTable = {}
  self.formatTable.bold = format:find("b") and true or false
  self.formatTable.italics = format:find("i") and true or false
  self.formatTable.underline = format:find("u") and true or false
  self.formatTable.strikethrough = format:find("s") and true or false
  local fs = format:gmatch("%d+")()
  if not fs then
    fs = self.fontSize
    self.format = self.format .. self.fontSize
  end
  self.formatTable.fontSize = fs
  self.fontSize = fs
  if format:find("c") then
    self.formatTable.alignment = "center"
  elseif format:find("l") then
    self.formatTable.alignment = "left"
  elseif format:find("r") then
    self.formatTable.alignment = "right"
  else
    self.formatTable.alignment = ""
  end
end

--- Sets the font face for the label, use empty string to clear the font and use css/default. Returns true if the font changed, nil+error if not.
-- @param font font face to use
function Geyser.Label:setFont(font)
  local af = getAvailableFonts()
  if not (af[font] or font == "") then
    local err = "Geyser.Label:setFont(): attempt to call setFont with font '" .. font .. "' which is not available, see getAvailableFonts() for valid options\n"
    err = err .. "In the meantime, we will use a similar font which isn't the one you asked for but we hope is close enough"
    debugc(err)
  end
  self.font = font
  self:echo()
end

--- Set whether or not the text in the label should be bold
-- @param bool True for bold
function Geyser.Label:setBold(bool)
  if bool then
    self.formatTable.bold = true
    if not self.format:find("b") then self.format = self.format .. "b" end
  else
    self.formatTable.bold = false
    if self.format:find("b") then self.format = self.format:gsub("b", "") end
  end
  self:echo()
end

--- Set whether or not the text in the label should be underline
-- @param bool True for underline
function Geyser.Label:setUnderline(bool)
  if bool then
    self.formatTable.underline = true
    if not self.format:find("u") then self.format = self.format .. "u" end
  else
    self.formatTable.underline = false
    if self.format:find("u") then self.format = self.format:gsub("u", "") end
  end
  self:echo()
end

--- Set whether or not the text in the label should be italics
-- @param bool True for italics
function Geyser.Label:setItalics(bool)
  if bool then
    self.formatTable.italics = true
    if not self.format:find("i") then self.format = self.format .. "i" end
  else
    self.formatTable.italics = false
    if self.format:find("i") then self.format = self.format:gsub("i", "") end
  end
  self:echo()
end

--- Set whether or not the text in the label should be strikethrough
-- @param bool True for strikethrough
function Geyser.Label:setStrikethrough(bool)
  if bool then
    self.formatTable.strikethrough = true
    if not self.format:find("s") then self.format = self.format .. "s" end
  else
    self.formatTable.strikethrough = false
    if self.format:find("s") then self.format = self.format:gsub("s", "") end
  end
  self:echo()
end

--- Set the font size for the label to use
-- @param fontSize the font size to use for the label. Should be a number
function Geyser.Label:setFontSize(fontSize)
  local fontSizeType = type(fontSize)
  fontSize = tonumber(fontSize)
  assert(fontSize, "fontSize as number expected, got " .. fontSizeType)
  self.fontSize = fontSize
  self.formatTable.fontSize = fontSize
  self.format = self.format:gsub("%d", "")
  self.format = self.format .. fontSize
  self:echo()
end

--- Sets the alignment for the label
-- @param alignment Valid alignments are 'c', 'center', 'l', 'left', 'r', 'right', or '' to not include the alignment as part of the echo
function Geyser.Label:setAlignment(alignment)
  local alignmentType = type(alignment)
  assert(alignmentType == "string", "alignment as string expected, got " .. alignmentType)
  local acceptedAlignments = {"c", "center", "l", "left", "r", "right", ""}
  assert(table.contains(acceptedAlignments, alignment), "invalid alignment sent. Valid alignments are 'c', 'center', 'l', 'left', 'r', 'right', or ''")
  if alignment:find('c') then
    self.formatTable.alignment = 'center'
    self.format = self.format .. "c"
    self.format = self.format:gsub("l", "")
    self.format = self.format:gsub("r", "")
  elseif alignment:find('l') then
    self.formatTable.alignment = 'left'
    self.format = self.format:gsub("c", "")
    self.format = self.format .. "l"
    self.format = self.format:gsub("r", "")
  elseif alignment:find('r') then
    self.formatTable.alignment = 'right'
    self.format = self.format:gsub("c", "")
    self.format = self.format:gsub("l", "")
    self.format = self.format .. "r"
  else
    self.formatTable.alignment = ""
    self.format = self.format:gsub("c", "")
    self.format = self.format:gsub("l", "")
    self.format = self.format:gsub("r", "")
  end
  self:echo()
end

function Geyser.Label:clear()
  self.message = ""
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
  self.onEnter = func
  self.onEnterArgs = { ... }
end

--- Sets a callback to be used when the mouse leaves this label.
-- @param func The function to use.
-- @param ... Parameters to pass to the function. Must be strings or numbers.
function Geyser.Label:setOnLeave (func, ...)
  setLabelOnLeave(self.name, func, ...)
  self.onLeave = func
  self.onLeaveArgs = { ... }
end


--- Sets the style sheet of the label
-- @param css The style sheet string
function Geyser.Label:setStyleSheet(css)
  css = css or self.stylesheet
  setLabelStyleSheet(self.name, css)
  self.stylesheet = css
end
--- Sets the tooltip of the label
-- @param txt the tooltip txt
-- @param duration the duration of the tooltip
function Geyser.Label:setToolTip(txt, duration)
  duration = duration or 10
  setLabelToolTip(self.name, txt, duration)
  self.toolTip = txt
  self.toolTipDuration = duration
end

--- Resets the tooltip of the label
function Geyser.Label:resetToolTip()
  resetLabelToolTip(self.name)
  self.toolTip = nil
  self.toolTipDuration = nil
end

--- Set a predefined mouse cursor shape for this label
-- @param cursorShape the predefined cursorshape as a string
-- see: https://wiki.mudlet.org/w/CursorShapes
function Geyser.Label:setCursor(cursorShape)
  setLabelCursor(self.name, cursorShape)
  -- Get cursorShape as string
  for k,v in pairs(mudlet.cursor) do
    if cursorShape == v then
      cursorShape = k
    end
  end

  self.cursorShape = cursorShape
end

--- Set a custom mouse cursor shape for this label
-- @param customCursor location of your custom cursor. It's suggested to use a png with size of 32x32 which is supported on all platforms
-- see https://doc.qt.io/qt-5/qcursor.html#shape
function Geyser.Label:setCustomCursor(customCursor, hotX, hotY)
  hotX = hotX or -1
  hotY = hotY or -1
  setLabelCustomCursor(self.name, customCursor, hotX, hotY)
  self.customCursor = customCursor
end

--- Resets the to the default Mouse Cursor Shape for this label
function Geyser.Label:resetCursor()
  resetLabelCursor(self.name)
  self.cursorShape = 0
  self.customCursor = ""
end

--- closes all nested labels
function closeAllLevels(label)
  if label.nestedLabels and label.nestedLabels[1] then
    label = label.nestedLabels[1]
  end
  for i, v in pairs(label.container.Label.scrollV) do
    v[1]:hide()
    v[2]:hide()
  end
  for i, v in pairs(label.container.Label.scrollH) do
    v[1]:hide()
    v[2]:hide()
  end
  for i, v in pairs(label.container.windowList) do
    if v.nestParent then
      v:hide()
    end
  end
end

--- Closes all nested labels under the given label, including any
--- nested children those children might possess
-- @param label The name of the label to use
function closeNestChildren(label)
  local nLabels = label.nestedLabels
  if nLabels then
    for i, v in pairs(nLabels) do
      v:hide()
      if v.nestedLabels then
        closeNestChildren(v)
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

--- Internal function.  This is a callback from a nested
--- labels scrollbar.
-- @param label The name of the scrollbar
function doNestScroll(label)
  local scrollDir = 0
  if string.find(label.name, "forScroll") then
    scrollDir = 1
  else
    scrollDir = -1
  end
  local bothScrolls
  if (string.sub(label.name, -1, -1) == "V") then
    bothScrolls = Geyser.Label.scrollV[label.nestParent]
  else
    bothScrolls = Geyser.Label.scrollH[label.nestParent]
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
  bscroll.nestParent:displayNest()
end

--- Displays the nested elements within label, and orients them
--- appropiately
-- @param label The name of the label to use
function Geyser.Label:displayNest()
  local maxDim = {}
  local flyMap = { R = { 1, 0 }, L = { -1, 0 }, T = { 0, -1 }, B = { 0, 1 } }
  if self.windowname ~= "main" then
    maxDim["H"], maxDim["V"] = getUserWindowSize(self.windowname)
  else
    maxDim["H"], maxDim["V"] = getMainWindowSize()
  end
  local parent = self
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
  local yOffsetT = 0
  for i, v in pairs(nestedLabels["V"]) do
    local width = v.get_width()
    local height = v.get_height()
    local number = #nestedLabels["V"]
    
    if v.flyDir == "L" then
      v.x = parX + flyMap[v.flyDir][1] * width
    else
      v.x = parX + flyMap[v.flyDir][1] * parW
    end
    
    -- T and B use their own offset values
    if v.flyDir == "T" then
      v.y = parY + flyMap[v.flyDir][2] * height * (number - flyIndex[v.flyDir]) + yOffsetT
      if v.y < 0 then
        yOffsetT = v.y * -1
        v.y = 0
      end
    else
      
      local edge = parY + parH + (number * height)
      if edge > maxDim["V"]  and v.flyDir == "B" then
        yOffsetT = edge - maxDim["V"]
      else
        yOffsetT = yOffset
      end
      
      v.y = parY + flyMap[v.flyDir][2] * parH - yOffsetT + height * flyIndex[v.flyDir]
    end
    
    v:show()
    v:raise()
    moveWindow(v.name, v.x, v.y)
    v:set_constraints()
    flyIndex[v.flyDir] = flyIndex[v.flyDir] + 1
  end
  local flyIndex = { R = 0, L = 0, T = 0, B = 0 }
  local xOffsetL = 0
  for i, v in pairs(nestedLabels["H"]) do
    local width = v.get_width()
    local height = v.get_height()
    local number = #nestedLabels["H"]
    -- L and H use their own offset values
    if v.flyDir == "L" then
      v.x = parX + flyMap[v.flyDir][1] * width * (number - flyIndex[v.flyDir]) + xOffsetL
      if v.x < 0 then
        xOffsetL = v.x * -1
        v.x = 0
      end
    else
      local edge = parX + parW + (number * width)
      if edge > maxDim["H"]  and v.flyDir == "R" then
        xOffsetL = edge - maxDim["H"]
      else
        xOffsetL = xOffset
      end
      v.x = parX + flyMap[v.flyDir][1] * parW - xOffsetL + width * flyIndex[v.flyDir]
    end
    
    if v.flyDir == "T" then
      v.y = parY + flyMap[v.flyDir][2] * height
    else
      v.y = parY + flyMap[v.flyDir][2] * parH
    end
    
    v:show()
    v:raise()
    moveWindow(v.name, v.x, v.y)
    v:set_constraints()
    flyIndex[v.flyDir] = flyIndex[v.flyDir] + 1
  end
end

--- Internal function when a parent nest element is clicked
--- to lay out the nested elements within
-- @param label The name of the label to use
function doNestShow(label)
  --Check if children are visible
  local lhidden = true
  if Geyser.Label.closeAllTimer then
    killTimer(Geyser.Label.closeAllTimer)
  end

  Geyser.Label.closeAllTimer = tempTimer(5, function() closeAllLevels(label) end)

  if label.nestedLabels and #label.nestedLabels > 0 then
    lhidden = label.nestedLabels[1].hidden
  end
  if not label.nestParent then
    closeAllLevels(label)
  else
    closeNeighbourChildren(label)
  end
  -- if Children are visible hide them
  if lhidden then
    label:displayNest()
  end
end

function closeNeighbourChildren(label)
 for i,v in ipairs(label.nestParent.nestedLabels) do
  closeNestChildren(v)
 end
end

--- Internal function when a nested element is moused over
--- to lay out the nested elements within that nested element
-- @param label The name of the label to use
function doNestEnter(label)
  local window = label
  if Geyser.Label.closeAllTimer then
    killTimer(Geyser.Label.closeAllTimer)
  end

  if window.flyOut and window and window.nestedLabels then
    if not label.nestParent then
      closeAllLevels(label)
    else
      closeNeighbourChildren(label)
    end
    --echo("entering window"..window.name.."\n")
    --Geyser.display(window)

      label:displayNest()
    end
end

--- Internal function when a nested element is left
--- to renest elements and restore order
-- @param label The name of the label to use
function doNestLeave(label)
  if Geyser.Label.closeAllTimer then
    killTimer(Geyser.Label.closeAllTimer)
  end
  Geyser.Label.closeAllTimer = tempTimer(2, function() closeAllLevels(label) end)
end

-- Save a reference to our parent constructor
Geyser.Label.parent = Geyser.Window

-- Overridden constructor
function Geyser.Label:new (cons, container)
  -- Initiate and set label specific things
  cons = cons or {}
  cons.type = cons.type or "label"
  cons.nestParent = cons.nestParent or nil
  cons.format = cons.format or ""

  -- Call parent's constructor
  local me = self.parent:new(cons, container)

  -- Set the metatable.
  setmetatable(me, self)
  self.__index = self
  me.windowname = me.windowname or me.container.windowname or "main"

  -- workaround for createLabel possibly being overwritten and not understanding the new parent argument
  -- see https://github.com/Mudlet/Mudlet/issues/3393
  if me.windowname == "main" then
    createLabel(me.name, me:get_x(), me:get_y(),
      me:get_width(), me:get_height(), me.fillBg)
  else
    createLabel(me.windowname, me.name, me:get_x(), me:get_y(),
      me:get_width(), me:get_height(), me.fillBg)
  end
-- This only has an effect if add2 is being used as for the standard add method me.hidden and me.auto_hidden is always false at creation/initialisation
  if me.hidden or me.auto_hidden then
    hideWindow(me.name)
  end
  -- parse any given format string and set sensible defaults
  me:processFormatString(cons.format)

  -- Set any defined colors
  Geyser.Color.applyColors(me)
  me:echo()

  -- Set up mouse hover as the callback if we have one
  if cons.nestflyout then
    me:setOnEnter("doNestShow", me)
  end
  -- Set up the callback if we have one
  if cons.nestable then
    me:setClickCallback("doNestShow", me)
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
    if type(me.onEnterArgs) == "string" or type(me.onEnterArgs) == "number" then
      me:setOnEnter(me.onEnter, me.onEnterArgs)
    elseif type(me.onEnterArgs) == "table" then
      me:setOnEnter(me.onEnter, unpack(me.onEnterArgs))
    else
      me:setOnEnter(me.onEnter)
    end
  end

  if me.onLeave then
    if type(me.onLeaveArgs) == "string" or type(me.onLeaveArgs) == "number" then
      me:setOnLeave(me.onLeave, me.onLeaveArgs)
    elseif type(me.onLeaveArgs) == "table" then
      me:setOnLeave(me.onLeave, unpack(me.onLeaveArgs))
    else
      me:setOnLeave(me.onLeave)
    end
  end

  if me.toolTip then
    me.toolTipDuration = me.toolTipDuration or 10
    me:setToolTip(me.toolTip, me.toolTipDuration)
  end

  -- Set clickthrough if included in constructor
  if cons.clickthrough then me:enableClickthrough() end

  if me.stylesheet then me:setStyleSheet() end

  --print("  New in " .. self.name .. " : " .. me.name)
  return me
end

--- Overridden constructor to use add2
function Geyser.Label:new2 (cons, container)
  cons = cons or {}
  cons.useAdd2 = true
  local me = self:new(cons, container)
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

  local cons = { name = "forScroll" .. label.name .. layout, x = label:get_x(), y = label:get_y(),
    width = label:get_width(), layoutDir = layoutDir, flyDir = flyDir, height = label:get_height(), message = "More..." }

  local forward = Geyser.Label:new(cons, label.container)

  forward.nestParent = parent
  forward.maxScroll = #parent.nestedLabels + 1
  forward:setOnEnter("doNestEnter", forward)
  forward:setOnLeave("doNestLeave", forward)
  forward:setClickCallback("doNestScroll", forward)
  cons.name = "backScroll" .. label.name .. layout

  local backward = Geyser.Label:new(cons, label.container)
  backward.nestParent = parent
  backward:setOnEnter("doNestEnter", backward)
  backward:setOnLeave("doNestLeave", backward)
  backward:setClickCallback("doNestScroll", backward)

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
  if self.windowname ~= "main" and not container then
    container = Geyser.windowList[self.windowname.."Container"].windowList[self.windowname]
  end
  local flyOut = false
  local flyDir, layoutDir
  if cons.layoutDir then
    flyDir = string.sub(cons.layoutDir, 1, 1)
    layoutDir = string.sub(cons.layoutDir, 2, 2)
  else
    flyDir = "L"
    layoutDir = "V"
  end
  local me = Geyser.Label:new(cons, container)
  --this is our parent
  me.nestParent = self
  me:setOnEnter("doNestEnter", me)
  me:setOnLeave("doNestLeave", me)

  if not me.clickCallback then
    --used in instances where an element only meant to serve as
    --a nest container is clicked on.  Without this, we get
    --seg faults
    me:setClickCallback("fakeFunction")
  end
  if not me.releaseCallback then
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
  if me.index then
    table.insert(self.nestedLabels, me.index, me)
  else
    self.nestedLabels[#self.nestedLabels + 1] = me
  end
  me:hide()
  return me
end

--- Sets label to no longer intercept mouse events
function Geyser.Label:enableClickthrough()
  enableClickthrough(self.name)
  self.clickthrough = true
end

--- Sets label to once again intercept mouse events
function Geyser.Label:disableClickthrough()
  disableClickthrough(self.name)
  self.clickthrough = false
end

-- internal function to change the layout of the rightClick menu if we are at the right edge
-- @param labelNest Nested Labels
-- @param fdir flying direction of the label
local function changeMenuLayout(labelNest, fdir)
  if not labelNest then
    return
  end
  for k, v in pairs(labelNest) do
    v.flyDir = fdir
    changeMenuLayout(v.nestedLabels, fdir)
  end
end

-- internal function to add or restyle an element
local function addElement(self, name, configLabel, myMenu, depth, index, restyle)
  myMenu.MenuLabels[name] = myMenu.MenuLabels[name] or
  myMenu:addChild(
  {
    index = index,
    depth = depth,
    menuName = name,
    width = configLabel["MenuWidth"..depth] or configLabel["MenuWidth"],
    height = configLabel["MenuHeight"..depth] or configLabel["MenuHeight"],
    name = myMenu.name..name,
    format = configLabel["MenuFormat"..depth] or configLabel["MenuFormat"],
    font = configLabel["MenuFont"..depth] or configLabel["MenuFont"],
    fgColor = "nocolor",
    message = name,
    color = "red",
    layoutDir = "RV",
    flyOut = true,
    nestable = true,
  }
)
-- ensure Labels are on the same parent window
if self.windowname ~= myMenu.MenuLabels[name].windowname then
  if self.windowname == "main" then
    myMenu.MenuLabels[name]:changeContainer(Geyser)
  else
    myMenu.MenuLabels[name]:changeContainer(Geyser.windowList[self.windowname.."Container"].windowList[self.windowname])
  end
end

if restyle then
  myMenu.MenuLabels[name].stylesheet = nil
end

local Style = configLabel["Style"..depth] or configLabel["Style"]
local MenuStyle = myMenu.MenuLabels[name].stylesheet or configLabel["MenuStyle"..depth] or configLabel["MenuStyle"]
MenuStyle = MenuStyle or configLabel.MenuStyleMode[string.lower(Style)]
myMenu.MenuLabels[name]:setStyleSheet(MenuStyle)
end

-- internal function to create the right click Menu Labels
function Geyser.Label:createMenuItems(restyle, MenuItems, configLabel, myMenu, depth)
  
  depth = depth or 1
  MenuItems = MenuItems or self.MenuItems
  self.MenuItems = MenuItems
  myMenu = myMenu or self.rightClickMenu
  configLabel = configLabel or myMenu
  myMenu.MenuLabels = myMenu.MenuLabels or {}
  local index = 1
  
  for i = 1, #MenuItems do
    if type(MenuItems[i]) == "string" and not MenuItems[i].ignore then
      addElement(self, MenuItems[i], configLabel, myMenu, depth, index, restyle)
      myMenu.MenuLabels[MenuItems[i]].index = index
      myMenu.MenuLabels[MenuItems[i]].tblIndex = i
      index = index + 1
    end
    --Ignore all children if parent is hidden
    if type(MenuItems[i]) == "table" and not MenuItems[i - 1].ignore then
      myMenu.MenuLabels[MenuItems[i - 1]]:createMenuItems(restyle, MenuItems[i], configLabel, myMenu.MenuLabels[MenuItems[i - 1]], depth + 1)
      myMenu.MenuLabels[MenuItems[i - 1]].isParent = true
    end
  end
end

function Geyser.Label:styleMenuItems(mode, css)
  local menu = self.rightClickMenu
  mode = mode or menu.Style
  menu.Style = mode
  css = css or menu.MenuStyle
  menu.MenuStyle = css
  self:createMenuItems(true)
end

-- internal function to handle the onRightClick event
-- @param oldClickCallBack previous clickcallback function
-- @param oldClickArgs previous clickcallback args
-- @param event the onClick event
function Geyser.Label:onRightClick(event)
  closeAllLevels(self.rightClickMenu)
  if event.button == "RightButton" then
    local winw = getUserWindowSize(self.windowname)
    local mousepos = self:get_x() + event.x
    local maxdiff = tonumber(self.rightClickMenu.MenuWidth)
    local diff = winw - mousepos
    local flyDir = self.rightClickMenu.nestedLabels[1].flyDir
    if diff <= maxdiff and flyDir == "R" then
      changeMenuLayout(self.rightClickMenu.nestedLabels, "L")
    elseif diff > maxdiff and flyDir == "L" then
      changeMenuLayout(self.rightClickMenu.nestedLabels, "R")
    end
    self.rightClickMenu:move(event.x, event.y)
    doNestShow(self.rightClickMenu)
  end
end

--- finds and returns a right click menu item
-- @param name Name of the menu item. If the menu item has a parent name needs to be given as "Parent.MenuItemName"
-- @param only used internally the right click menu [optional]
-- @param findParent only used internally to return a Parent [optional]
function Geyser.Label:findMenuElement(name, parent, findParent)
  local parent = parent or self.rightClickMenu
  local menu = parent.MenuItems
  local parentName = parent.menuName
  for i = 1, #menu do
    local item = menu[i]
    if type(item) == "string" then
      item = parentName and parentName.."."..item or item
      if item == name and not findParent then
        return parent.MenuLabels[menu[i]], menu
      end
      if findParent and item == name and type(menu[i + 1]) == "table" then
        return parent.MenuLabels[menu[i]], menu[i + 1]
      end
    end
    if type(item) == "table" then
      local itemParent = menu[i-1]
      local element, menuTable = self:findMenuElement(name, parent.MenuLabels[itemParent])
      if element then
        return element, menuTable
      end
    end
  end
  return nil, "findMenuElement: Couldn't find menu element "..name
end

--- Sets a action to be used when this label from the right click menu is clicked
-- @param name Name of the menu item. If the menu item has a parent name needs to be given as "Parent.MenuItemName"
-- @param ... Parameters to pass to the function. Will be passed directly to the setClickCallback function.
-- @see Geyser.Label:setClickCallback
function Geyser.Label:setMenuAction(name, ...)
  local menuElement = self:findMenuElement(name)
  if not menuElement then
    error ("setMenuAction: Couldn't find menu element "..name)
  end
  menuElement:setClickCallback(...)
end

--- hides a right click menu item
-- @param name Name of the menu item. If the menu item has a parent name needs to be given as "Parent.MenuItemName"
function Geyser.Label:hideMenuLabel(name)
  local menuElement = self:findMenuElement(name)
  if not menuElement then
    error ("hideMenuLabel: Couldn't find menu element "..name)
  end
  local nestTable = menuElement.nestParent.nestedLabels
  local index = table.index_of(menuElement.nestParent.nestedLabels, menuElement)
  -- If it's already hidden do nothing
  if menuElement.ignore then 
    return 
  end
  menuElement.MenuIndex = index
  menuElement.MenuNestTable = nestTable
  menuElement.ignore = true
  menuElement:hide()
  table.remove(nestTable, index)
  self:createMenuItems()
end

--- shows a previously hidden right click menu item
-- @param name Name of the menu item. If the menu item has a parent name needs to be given as "Parent.MenuItemName"
function Geyser.Label:showMenuLabel(name)
  local menuElement = self:findMenuElement(name)
  if not menuElement then
    error ("showMenuLabel: Couldn't find menu element "..name)
  end
  -- If it's already shown do nothing
  if not menuElement.ignore then 
    return 
  end
  menuElement.ignore = false
  table.insert(menuElement.MenuNestTable, menuElement.MenuIndex, menuElement)
  self:createMenuItems()
end

--- adds a new item to the right click menu
-- @param name Name of the new menu item.
-- @param parent name of the parent where the new item will be created in (optional)
-- @param index of the new menu item (optional)
function Geyser.Label:addMenuLabel(name, parent, index)
  local menuElement, menuParent = self:findMenuElement(parent, self.rightClickMenu, true)
  
  if parent and not menuParent then
    error ("showMenuLabel: Couldn't find menu parent "..parent)
  end
  
  menuElement = menuElement or self.rightClickMenu
  menuParent = menuParent or self.rightClickMenu.MenuItems
  
  if parent then
    parent = parent.."."
  else
    parent = ""
  end
  
  if not menuElement.MenuLabels[name] then
    menuParent[#menuParent + 1] = name
  elseif menuElement.MenuLabels[name].ignore then
    self:showMenuLabel(parent..name)
  end
  
  self:createMenuItems()
  
  if index then
    self:changeMenuIndex(parent..name, index)
  end
end

--- changes a right click menu items index
-- @param name Name of the menu item. If the menu item has a parent name needs to be given as "Parent.MenuItemName"
-- @param index the new index
function Geyser.Label:changeMenuIndex(name, index)
  local menuElement, menuTable = self:findMenuElement(name, self.rightClickMenu)
  
  if not menuElement then
    error ("changeMenuIndex: Couldn't find menu element "..name)
  end
  
  local nestTable = menuElement.nestParent.nestedLabels
  local newindex = nestTable[index].tblIndex
  
  -- table index is not the same as index
  -- if element is parent behave differently
  if nestTable[index].isParent then
    if newindex > menuElement.tblIndex then
      newindex = newindex + 1
    end
  end
  --nestTable
  table.remove(nestTable, menuElement.index)
  table.insert(nestTable, index, menuElement)
  
  menuElement.index = index
  
  -- MenuItems Table
  -- parents need to bring also their children to their index
  if menuElement.isParent then
    local child = menuTable[menuElement.tblIndex + 1]
    if newindex > menuElement.tblIndex then
      newindex = newindex - 1
    end
    table.remove(menuTable, menuElement.tblIndex + 1)
    table.remove(menuTable, menuElement.tblIndex)
    table.insert(menuTable, newindex, child)
    table.insert(menuTable, newindex, menuElement.menuName)
  else
    table.remove(menuTable, menuElement.tblIndex)
    table.insert(menuTable, newindex, menuElement.menuName)
  end
  self:createMenuItems()
end

--- creates a right click menu for your Label
---@param cons different parameters controlling the size and style of the right click menu elements
--@param[opt="140" ] cons.MenuWidth  default menu width of your right click menu. to give levels different width add a number at the end per level usage MenuWidth1
--@param[opt="25" ] cons.MenuWidth default menu height of your right click menu. to give levels different height add a number at the end per level usage MenuHeight1
--@param[opt="c10"] cons.MenuFormat default font/echo format of your right click menu. different levels can use different formating. usage MenuFormat1 MenuFormat2
--@param[opt="light"] cons.Style default styling mode of your right click menu. 2 possible modes "light" and "dark". different levels can also have different styling modes
--@param cons.MenuStyle default style of your menu. if this is given cons.Style will be ignored. different levels can also have different MenuStyles
--@param cons.MenuItems list of right click menu items/elements. usage example: MenuItems = {"First", "Second", {"First"},"Third"} 
function Geyser.Label:createRightClickMenu(cons)
  cons.width = "0"
  cons.height = "0"
  cons.nestable = true
  cons.name = self.name .. "rightClickMenu"
  cons.MenuWidth = cons.MenuWidth or 140
  cons.MenuHeight = cons.MenuHeight or 25
  cons.MenuFormat = cons.MenuFormat or "c10"
  cons.MenuStyleMode = {}
  cons.MenuStyleMode["light"] = [[QLabel::hover{ background-color: rgba(0,150,255,100%); color: white;} QLabel::!hover{color: black; background-color: rgba(240,240,240,100%);} ]]
  cons.MenuStyleMode["dark"] = [[QLabel::hover{ background-color: #282828;  color: #808080;} QLabel::!hover{color: #707070; background-color:#181818;}]]
  
  cons.Style = cons.Style or "light"
  
  if not(self.rightClickMenu) then
    self:setClickCallback(self.onRightClick, self)
  end
  
  -- create a label with a nestable=true property as base menu
  self.rightClickMenu = Geyser.Label:new(cons, self)
  self:createMenuItems(nil, cons.MenuItems)
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
