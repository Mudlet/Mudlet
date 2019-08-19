--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Generate a window name unique to this session.
function Geyser.nameGen (type)
  local index = Geyser.i
  local t = type or "window"
  Geyser.i = Geyser.i + 1
  return "anon_" .. t .. "_" .. index
end

--- Hide all managed windows.
function Geyser.hideAll(type)
  for _, v in pairs(Geyser.windowList) do
    if v.type == type or not type then
      v:hide()
    end
  end
end

--- Show all managed windows.
function Geyser.showAll(type)
  for _, v in pairs(Geyser.windowList) do
    if v.type == type or not type then
      v:show()
    end
  end
end

--- Non-recursive display of an item, because the normal 'display' was
-- causing Mudlet to hang. Not sure why.
function Geyser.display (table)
  echo("------ " .. type(table) .. " ------\n")
  if type(table) == "table" then
    for k, v in pairs(table) do
      echo("'" .. tostring(k) .. "' - " .. tostring(v) .. "\n")
    end
  else
    echo(tostring(table) .. "\n")
  end
end

--- Clone a table, for good fun and profit.
function Geyser.copyTable (table)
  local copy = {}
  if table then
    for k, v in pairs(table) do
      -- do deep copy on a table if it requests one by having the __clone function defined.
      if type(v) == "table" and v.__clone then
        copy[k] = v.__clone()
      else
        copy[k] = v
      end
    end
  end
  return copy
end

createMiniConsole("__Geyser__Fontconsole",0,0,0,0)
hideWindow("__Geyser__Fontconsole")

function Geyser.calcSizeForFont(fontName, fontSize)
  local fcn = "__Geyser__Fontconsole"
  local fontNameType = type(fontName)
	local fontSizeType = type(fontSize)
	local af = getAvailableFonts()
  if fontNameType ~= "string" then
	  error("Geyser.calcSizeForFont(fontName, fontSize): Argument Error: fontName as string expected, got " .. fontNameType)
	elseif fontSizeType ~= "number" then
	  error("Geyser.calcSizeForFont(fontName, fontSize): Argument Error: fontSize as number expect, got " .. fontSizeType)
	elseif not table.contains(af, fontName) then
	  error("Geyser.calcSizeForFont(fontNAme, fontSize): " .. fontName .. " is not available on this machine. Please check getAvailableFonts() for options.")
  end
  setFont(fcn, fontName)
  setMiniConsoleFontSize(fcn, fontSize)
	return calcFontSize(fcn)
end