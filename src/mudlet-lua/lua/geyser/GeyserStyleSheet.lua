--- Represents a stylesheet used for styling labels.
-- Based primarily off the work of Vadi on CSSMan <a href="https://forums.mudlet.org/viewtopic.php?f=6&t=3502">CSSMan</a>.
-- This version extends it with recursive inheritance of properties, storing the target of the stylesheet (QLabel, QPlainTextEdit, etc), and parsing string stylesheets for them.
-- <br/>See also: <a href="https://wiki.mudlet.org/w/Manual:Geyser#Geyser.StyleSheet">Mudlet Manual</a>
-- @author guy
-- @author Vadi
-- @module Geyser.StyleSheet

Geyser.StyleSheet = {}
Geyser.StyleSheet.__index = Geyser.StyleSheet

-- create locals for things we use in loops or recursively (speed optimization)
local trim, match, gsub, split, update, format = string.trim, string.match, string.gsub, string.split, table.update, string.format

--- Creates a new StyleSheet
-- @param stylesheet the stylesheet to start off with
-- @param parent if provided, this stylesheet will inherit values for keys it does not set from its parent
-- @param target if provided, will enclose the stylesheet in "category { stylesheet }". Will also extract this information from a stylesheet formatted in this way.
function Geyser.StyleSheet:new(stylesheet, parent, target)
  local obj = {}
  setmetatable(obj, Geyser.StyleSheet)
  local styleType = type(stylesheet)
  local styleTable
  if styleType == "string" then
    local tgt
    styleTable, tgt = self:parseCSS(stylesheet)
    if not target then target = tgt end
  elseif styleType == "table" then
    styleTable = table.deepcopy(stylesheet)
  else
    printError("Geyser.StyleSheet:new: stylesheet as string or table expected, got " .. styleType, true, true)
  end
  obj:setStyleTable(styleTable or {})
  obj:setParent(parent)
  obj:setTarget(target)
  return obj
end

--- Parses a text stylesheet into a property:value table
-- @param stylesheet the stylesheet(as a string) to parse
-- @return table of stylesheet properties with their values
function Geyser.StyleSheet:parseCSS(stylesheet)
  if stylesheet == "" then
    return {}, nil
  end
  local stylesheetType = type(stylesheet)
  if type(stylesheet) ~= "string" then
    printError("parseCSS: stylesheet as string expected, got " .. stylesheetType, true, true)
  end
  local styleTable = {}
  if not stylesheet:find(";") then
    return nil, "no valid lines found in stylesheet. Lines must be terminated by a ;"
  end
  local target, styles = match(stylesheet,"(%w+)%s*{(.*)}")
  if styles then stylesheet = styles end
  stylesheet = gsub(stylesheet, "[\r\n]", "")
  for _,element in ipairs(split(stylesheet, ";")) do
    element = trim(element)
    local property, value = match(element, "^(.-):(.+)$")
    if property and value then
      styleTable[trim(property)] = trim(value)
    end
  end
  return styleTable, target
end

--- Sets the value of a stylesheet property
-- @param property the property to set the value for, as a string
-- @param value the value to set for the property, as a string
function Geyser.StyleSheet:set(property, value)
  self.styleTable[trim(property)] = trim(value)
end

--- Returns the value of a stylesheet property
-- @param property the property to return the value of
function Geyser.StyleSheet:get(property)
  return self.styleTable[property]
end

--- Returns the stylesheet as a string for use in set*StyleSheet functions.
-- @param inherit Defaults to true. If true, returns table with properties inherited from its parent (if any). If false will only return the properties set in this StyleSheet
-- @return stylesheet as a string, suitable for passing in to set*StyleSheet() functions
function Geyser.StyleSheet:getCSS(inherit)
  inherit = type(inherit) == "nil" and true or inherit
  local styleTable = self:getStyleTable(inherit)
  local styleString = ""
  for prop, val in spairs(styleTable) do
    styleString = format("%s%s: %s;\n", styleString, prop, val)
  end
  if self.target then
    styleString = format("%s {\n%s}", self.target, styleString)
  end
  return styleString
end

--- Set the style for this StyleSheet using the text stylesheet.
-- @param css the stylesheet to parse. If not provided will clear the style
function Geyser.StyleSheet:setCSS(css)
  if css == nil then
    self.styleTable = {}
    return
  end
  local cssType = type(css)
  if cssType ~= "string" then
    printError(f"Geyser.StyleSheet:setCSS: bad argument #1 type (css as string expected, got {cssType})", true, true)
  end
  local styleTable, target = self:parseCSS(css)
  if not styleTable then
    return nil, f"error parsing css: {target}"
  end
  if target then
    self:setTarget(target)
  end
  self:setStyleTable(styleTable)
end

--- Returns the stylesheet as a table with the properties as keys which hold the value they've had set. Inherits property:values for unset properties from its parent stylesheet, if set.
-- @param inherit Defaults to true. If true, returns table with properties inherited from its parent (if any). If false will only return the properties set in this StyleSheet
-- @return StyleSheet as a property:value table
function Geyser.StyleSheet:getStyleTable(inherit)
  inherit = type(inherit) == "nil" and true or inherit
  local styleTable = {}
  if self.parent and inherit then
    styleTable = self.parent:getStyleTable()
  end
  styleTable = update(styleTable, self.styleTable)
  return styleTable
end

--- Allows you to set the styleTable directly with the properties as keys which hold the value to set for the property.
-- @param styleTable table with properties for keys and their values as the value. If not provided will clear the style.
function Geyser.StyleSheet:setStyleTable(styleTable)
  styleTable = styleTable or {}
  local styleTableType = type(styleTable)
  if styleTableType ~= "table" then
    printError(f"Geyser.StyleSheet:setStyleTable: bad argument #1 type (styleTable as table expected, got {styleTableType}!)", true, true)
  end
  self.styleTable = styleTable
end

--- Allows you to set the parent for this stylesheet. Any properties set in the parent will be used by this stylesheet unless it has a value set for that property itself.
-- @param parent a Geyser.StyleSheet to inherit from. If not provided, clears the parent.
function Geyser.StyleSheet:setParent(parent)
  if not parent then
    self.parent = nil
    return
  end
  if getmetatable(parent) == Geyser.StyleSheet then
    self.parent = parent
    return
  end
  printError(f"Geyser.StyleSheet:setParent: bad argument #1 type (Geyser.StyleSheet expected, got {type(parent)})", true, true)
end

--- Allows you to set a target for this stylesheet to effect, such as "QPlainTextEdit" etc.
-- @param target the target to apply this stylesheet to. if not provided will clear the target
function Geyser.StyleSheet:setTarget(target)
  if target == nil then
    self.target = nil
    return
  end
  local targetType = type(target)
  if targetType ~= "string" then
    printError(f"Geyser.StyleSheet:setTarget: bad argument #1 type (optional target as string expected, got {targetType})", true, true)
  end
  self.target = target
end
