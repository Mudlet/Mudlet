----------------------------------------------------------------------------------
--- Mudlet GUI Utils
----------------------------------------------------------------------------------


--- The <i>gaugesTable table</i>. First we need to make this table which will be
--- used later to store important data in.
---
--- @class function
--- @name gaugesTable
gaugesTable = {}

--- The <i>color_table table</i> holds definition of color names. These are intended to be
-- used in conjunction with fg() and bg() colorizer functions.
-- Mudlet's original table - going back a few years - differs from the
-- "standard" which can be found at https://www.w3.org/TR/SVG11/types.html#ColorKeywords
-- Mudlet has additional colours not in the standard:
-- "light_goldenrod", "light_slate_blue", "navy_blue" and "violet_red"
-- Mudlet is missing some colours:
-- "aqua", "fuchsia", "dark_blue", "dark_cyan", "dark_gray"/"dark_grey",
-- "dark_magenta", "dark_red", "indigo", "light_green", "olive", "silver",
-- "teal", "violet_red"
-- Also Mudlet redefines:
-- "gray"/"grey", "green", "maroon" and "purple"
-- All of the above also implies the Camel Case equivalents; in summary:
--  Colour               Mudlet            Web Standard
--  aqua                                     0, 255, 255
--  fuchsia                                255,   0, 255
--  dark_blue                                0,   0, 139
--  dark_gray/dark_grey                    169, 169, 169
--  dark_magenta                           139,   0, 139
--  dark_red                               139,   0,   0
--  gray/grey            190, 190, 190     128, 128, 128
--  green                  0, 255,   0       0, 128,   0
--  indigo                                  75,   0, 130
--  light_goldrod        238, 221, 130
--  light_slate_blue     132, 112, 255
--  light_green                            144, 238, 144
--  maroon               176,  48,  96     128,   0,   0
--  navy_blue              0,   0, 128
--  olive                                  128, 128,   0
--  purple               160,  32, 240     128,   0, 128
--  silver                                 192, 192, 192
--  teal                                     0, 128, 128
--  violet_red                             208,  32, 240
-- @see showColors
-- @see bg
-- @see fg
-- @class function
-- @name color_table
color_table = color_table or {}
color_table["alice_blue"]             = { 240, 248, 255 }
color_table["AliceBlue"]              = { 240, 248, 255 }
color_table["antique_white"]          = { 250, 235, 215 }
color_table["AntiqueWhite"]           = { 250, 235, 215 }
color_table["aquamarine"]             = { 127, 255, 212 }
color_table["azure"]                  = { 240, 255, 255 }
color_table["beige"]                  = { 245, 245, 220 }
color_table["bisque"]                 = { 255, 228, 196 }
color_table["black"]                  = { 0, 0, 0 }
color_table["blanched_almond"]        = { 255, 235, 205 }
color_table["BlanchedAlmond"]         = { 255, 235, 205 }
color_table["blue"]                   = { 0, 0, 255 }
color_table["blue_violet"]            = { 138, 43, 226 }
color_table["BlueViolet"]             = { 138, 43, 226 }
color_table["brown"]                  = { 165, 42, 42 }
color_table["burlywood"]              = { 222, 184, 135 }
color_table["cadet_blue"]             = { 95, 158, 160 }
color_table["CadetBlue"]              = { 95, 158, 160 }
color_table["chartreuse"]             = { 127, 255, 0 }
color_table["chocolate"]              = { 210, 105, 30 }
color_table["coral"]                  = { 255, 127, 80 }
color_table["cornflower_blue"]        = { 100, 149, 237 }
color_table["CornflowerBlue"]         = { 100, 149, 237 }
color_table["cornsilk"]               = { 255, 248, 220 }
color_table["cyan"]                   = { 0, 255, 255 }
color_table["dark_goldenrod"]         = { 184, 134, 11 }
color_table["DarkGoldenrod"]          = { 184, 134, 11 }
color_table["dark_green"]             = { 0, 100, 0 }
color_table["DarkGreen"]              = { 0, 100, 0 }
color_table["dark_khaki"]             = { 189, 183, 107 }
color_table["DarkKhaki"]              = { 189, 183, 107 }
color_table["dark_olive_green"]       = { 85, 107, 47 }
color_table["DarkOliveGreen"]         = { 85, 107, 47 }
color_table["dark_orange"]            = { 255, 140, 0 }
color_table["DarkOrange"]             = { 255, 140, 0 }
color_table["dark_orchid"]            = { 153, 50, 204 }
color_table["DarkOrchid"]             = { 153, 50, 204 }
color_table["dark_salmon"]            = { 233, 150, 122 }
color_table["DarkSalmon"]             = { 233, 150, 122 }
color_table["dark_slate_blue"]        = { 72, 61, 139 }
color_table["dark_sea_green"]         = { 143, 188, 143 }
color_table["DarkSeaGreen"]           = { 143, 188, 143 }
color_table["DarkSlateBlue"]          = { 72, 61, 139 }
color_table["dark_slate_gray"]        = { 47, 79, 79 }
color_table["DarkSlateGray"]          = { 47, 79, 79 }
color_table["dark_slate_grey"]        = { 47, 79, 79 }
color_table["DarkSlateGrey"]          = { 47, 79, 79 }
color_table["dark_turquoise"]         = { 0, 206, 209 }
color_table["DarkTurquoise"]          = { 0, 206, 209 }
color_table["dark_violet"]            = { 148, 0, 211 }
color_table["DarkViolet"]             = { 148, 0, 211 }
color_table["deep_pink"]              = { 255, 20, 147 }
color_table["DeepPink"]               = { 255, 20, 147 }
color_table["deep_sky_blue"]          = { 0, 191, 255 }
color_table["DeepSkyBlue"]            = { 0, 191, 255 }
color_table["dodger_blue"]            = { 30, 144, 255 }
color_table["DodgerBlue"]             = { 30, 144, 255 }
color_table["dim_gray"]               = { 105, 105, 105 }
color_table["DimGray"]                = { 105, 105, 105 }
color_table["dim_grey"]               = { 105, 105, 105 }
color_table["DimGrey"]                = { 105, 105, 105 }
color_table["firebrick"]              = { 178, 34, 34 }
color_table["floral_white"]           = { 255, 250, 240 }
color_table["FloralWhite"]            = { 255, 250, 240 }
color_table["forest_green"]           = { 34, 139, 34 }
color_table["ForestGreen"]            = { 34, 139, 34 }
color_table["gainsboro"]              = { 220, 220, 220 }
color_table["ghost_white"]            = { 248, 248, 255 }
color_table["GhostWhite"]             = { 248, 248, 255 }
color_table["gold"]                   = { 255, 215, 0 }
color_table["goldenrod"]              = { 218, 165, 32 }
color_table["gray"]                   = { 190, 190, 190 }
color_table["grey"]                   = { 190, 190, 190 }
color_table["green"]                  = { 0, 255, 0 }
color_table["green_yellow"]           = { 173, 255, 47 }
color_table["GreenYellow"]            = { 173, 255, 47 }
color_table["honeydew"]               = { 240, 255, 240 }
color_table["hot_pink"]               = { 255, 105, 180 }
color_table["HotPink"]                = { 255, 105, 180 }
color_table["indian_red"]             = { 205, 92, 92 }
color_table["IndianRed"]              = { 205, 92, 92 }
color_table["khaki"]                  = { 240, 230, 140 }
color_table["ivory"]                  = { 255, 255, 240 }
color_table["lavender"]               = { 230, 230, 250 }
color_table["lavender_blush"]         = { 255, 240, 245 }
color_table["LavenderBlush"]          = { 255, 240, 245 }
color_table["lawn_green"]             = { 124, 252, 0 }
color_table["LawnGreen"]              = { 124, 252, 0 }
color_table["lemon_chiffon"]          = { 255, 250, 205 }
color_table["LemonChiffon"]           = { 255, 250, 205 }
color_table["light_blue"]             = { 173, 216, 230 }
color_table["LightBlue"]              = { 173, 216, 230 }
color_table["light_coral"]            = { 240, 128, 128 }
color_table["LightCoral"]             = { 240, 128, 128 }
color_table["light_cyan"]             = { 224, 255, 255 }
color_table["LightCyan"]              = { 224, 255, 255 }
color_table["light_goldenrod"]        = { 238, 221, 130 }
color_table["LightGoldenrod"]         = { 238, 221, 130 }
color_table["light_goldenrod_yellow"] = { 250, 250, 210 }
color_table["LightGoldenrodYellow"]   = { 250, 250, 210 }
color_table["light_gray"]             = { 211, 211, 211 }
color_table["LightGray"]              = { 211, 211, 211 }
color_table["light_grey"]             = { 211, 211, 211 }
color_table["LightGrey"]              = { 211, 211, 211 }
color_table["light_pink"]             = { 255, 182, 193 }
color_table["LightPink"]              = { 255, 182, 193 }
color_table["light_salmon"]           = { 255, 160, 122 }
color_table["LightSalmon"]            = { 255, 160, 122 }
color_table["light_sea_green"]        = { 32, 178, 170 }
color_table["LightSeaGreen"]          = { 32, 178, 170 }
color_table["light_sky_blue"]         = { 135, 206, 250 }
color_table["LightSkyBlue"]           = { 135, 206, 250 }
color_table["light_slate_blue"]       = { 132, 112, 255 }
color_table["LightSlateBlue"]         = { 132, 112, 255 }
color_table["light_slate_gray"]       = { 119, 136, 153 }
color_table["LightSlateGray"]         = { 119, 136, 153 }
color_table["light_slate_grey"]       = { 119, 136, 153 }
color_table["LightSlateGrey"]         = { 119, 136, 153 }
color_table["light_steel_blue"]       = { 176, 196, 222 }
color_table["LightSteelBlue"]         = { 176, 196, 222 }
color_table["light_yellow"]           = { 255, 255, 224 }
color_table["LightYellow"]            = { 255, 255, 224 }
color_table["lime_green"]             = { 50, 205, 50 }
color_table["LimeGreen"]              = { 50, 205, 50 }
color_table["linen"]                  = { 250, 240, 230 }
color_table["magenta"]                = { 255, 0, 255 }
color_table["maroon"]                 = { 176, 48, 96 }
color_table["medium_aquamarine"]      = { 102, 205, 170 }
color_table["MediumAquamarine"]       = { 102, 205, 170 }
color_table["medium_blue"]            = { 0, 0, 205 }
color_table["MediumBlue"]             = { 0, 0, 205 }
color_table["medium_orchid"]          = { 186, 85, 211 }
color_table["MediumOrchid"]           = { 186, 85, 211 }
color_table["medium_purple"]          = { 147, 112, 219 }
color_table["MediumPurple"]           = { 147, 112, 219 }
color_table["medium_sea_green"]       = { 60, 179, 113 }
color_table["MediumSeaGreen"]         = { 60, 179, 113 }
color_table["medium_slate_blue"]      = { 123, 104, 238 }
color_table["MediumSlateBlue"]        = { 123, 104, 238 }
color_table["medium_spring_green"]    = { 0, 250, 154 }
color_table["MediumSpringGreen"]      = { 0, 250, 154 }
color_table["medium_turquoise"]       = { 72, 209, 204 }
color_table["MediumTurquoise"]        = { 72, 209, 204 }
color_table["medium_violet_red"]      = { 199, 21, 133 }
color_table["MediumVioletRed"]        = { 199, 21, 133 }
color_table["midnight_blue"]          = { 25, 25, 112 }
color_table["MidnightBlue"]           = { 25, 25, 112 }
color_table["mint_cream"]             = { 245, 255, 250 }
color_table["MintCream"]              = { 245, 255, 250 }
color_table["misty_rose"]             = { 255, 228, 225 }
color_table["MistyRose"]              = { 255, 228, 225 }
color_table["moccasin"]               = { 255, 228, 181 }
color_table["navajo_white"]           = { 255, 222, 173 }
color_table["NavajoWhite"]            = { 255, 222, 173 }
color_table["navy"]                   = { 0, 0, 128 }
color_table["navy_blue"]              = { 0, 0, 128 }
color_table["NavyBlue"]               = { 0, 0, 128 }
color_table["old_lace"]               = { 253, 245, 230 }
color_table["OldLace"]                = { 253, 245, 230 }
color_table["olive_drab"]             = { 107, 142, 35 }
color_table["OliveDrab"]              = { 107, 142, 35 }
color_table["orange"]                 = { 255, 165, 0 }
color_table["orange_red"]             = { 255, 69, 0 }
color_table["OrangeRed"]              = { 255, 69, 0 }
color_table["orchid"]                 = { 218, 112, 214 }
color_table["pale_goldenrod"]         = { 238, 232, 170 }
color_table["PaleGoldenrod"]          = { 238, 232, 170 }
color_table["pale_green"]             = { 152, 251, 152 }
color_table["PaleGreen"]              = { 152, 251, 152 }
color_table["pale_turquoise"]         = { 175, 238, 238 }
color_table["PaleTurquoise"]          = { 175, 238, 238 }
color_table["pale_violet_red"]        = { 219, 112, 147 }
color_table["PaleVioletRed"]          = { 219, 112, 147 }
color_table["papaya_whip"]            = { 255, 239, 213 }
color_table["PapayaWhip"]             = { 255, 239, 213 }
color_table["peach_puff"]             = { 255, 218, 185 }
color_table["PeachPuff"]              = { 255, 218, 185 }
color_table["peru"]                   = { 205, 133, 63 }
color_table["pink"]                   = { 255, 192, 203 }
color_table["plum"]                   = { 221, 160, 221 }
color_table["powder_blue"]            = { 176, 224, 230 }
color_table["PowderBlue"]             = { 176, 224, 230 }
color_table["purple"]                 = { 160, 32, 240 }
color_table["royal_blue"]             = { 65, 105, 225 }
color_table["RoyalBlue"]              = { 65, 105, 225 }
color_table["red"]                    = { 255, 0, 0 }
color_table["rosy_brown"]             = { 188, 143, 143 }
color_table["RosyBrown"]              = { 188, 143, 143 }
color_table["saddle_brown"]           = { 139, 69, 19 }
color_table["SaddleBrown"]            = { 139, 69, 19 }
color_table["salmon"]                 = { 250, 128, 114 }
color_table["sandy_brown"]            = { 244, 164, 96 }
color_table["SandyBrown"]             = { 244, 164, 96 }
color_table["sea_green"]              = { 46, 139, 87 }
color_table["SeaGreen"]               = { 46, 139, 87 }
color_table["seashell"]               = { 255, 245, 238 }
color_table["sienna"]                 = { 160, 82, 45 }
color_table["sky_blue"]               = { 135, 206, 235 }
color_table["SkyBlue"]                = { 135, 206, 235 }
color_table["slate_blue"]             = { 106, 90, 205 }
color_table["SlateBlue"]              = { 106, 90, 205 }
color_table["slate_gray"]             = { 112, 128, 144 }
color_table["SlateGray"]              = { 112, 128, 144 }
color_table["slate_grey"]             = { 112, 128, 144 }
color_table["SlateGrey"]              = { 112, 128, 144 }
color_table["snow"]                   = { 255, 250, 250 }
color_table["steel_blue"]             = { 70, 130, 180 }
color_table["SteelBlue"]              = { 70, 130, 180 }
color_table["spring_green"]           = { 0, 255, 127 }
color_table["SpringGreen"]            = { 0, 255, 127 }
color_table["tan"]                    = { 210, 180, 140 }
color_table["thistle"]                = { 216, 191, 216 }
color_table["tomato"]                 = { 255, 99, 71 }
color_table["transparent"]            = { 255, 255, 255, 0}
color_table["turquoise"]              = { 64, 224, 208 }
color_table["violet_red"]             = { 208, 32, 144 }
color_table["VioletRed"]              = { 208, 32, 144 }
color_table["violet"]                 = { 238, 130, 238 }
color_table["wheat"]                  = { 245, 222, 179 }
color_table["white"]                  = { 255, 255, 255 }
color_table["white_smoke"]            = { 245, 245, 245 }
color_table["WhiteSmoke"]             = { 245, 245, 245 }
color_table["yellow"]                 = { 255, 255, 0 }
color_table["yellow_green"]           = { 154, 205, 50 }
color_table["YellowGreen"]            = { 154, 205, 50 }


--- Move a custom gauge.
---
--- @usage This would move the health bar gauge to the location 1200, 400.
---   <pre>
---   moveGauge("healthBar", 1200, 400)
---   </pre>
---
--- @see createGauge
function moveGauge(gaugeName, x, y)
  assert(gaugesTable[gaugeName], "moveGauge: no such gauge exists.")
  assert(type(x) == 'number', 'moveGauge: bad argument #2 type (expected number, got '..type(x)..'!)')
  assert(type(y) == 'number', 'moveGauge: bad argument #3 type (expected number, got '..type(y)..'!)')
  moveWindow(gaugeName .. "_back", x, y)
  moveWindow(gaugeName .. "_text", x, y)
  -- save new values in table
  gaugesTable[gaugeName].x, gaugesTable[gaugeName].y = x, y
  setGauge(gaugeName, gaugesTable[gaugeName].value, 1)
end


--- Hide a custom gauge.
---
--- @usage This should hide the given gauge.
---   <pre>
---   hideGauge("healthBar")
---   </pre>
---
--- @see createGauge, moveGauge, showGauge
function hideGauge(gaugeName)
  assert(gaugesTable[gaugeName], "hideGauge: no such gauge exists.")
  hideWindow(gaugeName .. "_back")
  hideWindow(gaugeName .. "_front")
  hideWindow(gaugeName .. "_text")
end


--- Show a custom gauge.
---
--- @usage This should show the given gauge.
---   <pre>
---   showGauge("healthBar")
---   </pre>
---
--- @see createGauge, moveGauge, hideGauge
function showGauge(gaugeName)
  assert(gaugesTable[gaugeName], "showGauge: no such gauge exists.")
  showWindow(gaugeName .. "_back")
  showWindow(gaugeName .. "_front")
  showWindow(gaugeName .. "_text")
end

--- @see createGauge
function setGaugeWindow(windowName, gaugeName, x, y, show)
  windowName = windowName or "main"
  x = x or 0
  y = y or 0
  show = show or true
  assert(gaugesTable[gaugeName], "setGaugeWindow: no such gauge exists.")
  setWindow(windowName, gaugeName .. "_back", x, y, show)
  setWindow(windowName, gaugeName .. "_front", x, y, show)
  setWindow(windowName, gaugeName .. "_text", x, y, show)
  -- save new values in table
  gaugesTable[gaugeName].x, gaugesTable[gaugeName].y = x, y
  setGauge(gaugeName, gaugesTable[gaugeName].value, 1)
end

--- Set the text on a custom gauge.
---
--- @usage
---   <pre>
---   setGaugeText("healthBar", "HP: 100%", 40, 40, 40)
---   </pre>
--- @usage
---   <pre>
---   setGaugeText("healthBar", "HP: 100%", "red")
---   </pre>
---
--- @param gaugeName
--- @param gaugeText An empty gaugeText will clear the text entirely.
--- @param color1 Colors are optional and will default to 0,0,0(black) if not passed as args.
--- @param color2
--- @param color3
---
--- @see createGauge
function setGaugeText(gaugeName, gaugeText, r, g, b)
  assert(gaugesTable[gaugeName], "setGaugeText: no such gauge exists.")
  if r ~= nil then
    if g == nil then
      r, g, b = getRGB(r)
    end
  else
    r, g, b = 0, 0, 0
  end
  gaugeText = gaugeText or ""
  local echoString = [[<font color="#]] .. RGB2Hex(r, g, b) .. [[">]] .. gaugeText .. [[</font>]]
  echo(gaugeName .. "_text", echoString)
  -- save new values in table
  gaugesTable[gaugeName].text = echoString
end

--- Set gauge to no longer intercept mouse events
--- @param gaugeName
function enableGaugeClickthrough(gaugeName)
  assert(gaugesTable[gaugeName], "enableGaugeClickthrough: no such gauge exists.")
  enableClickthrough(gaugeName .. "_back")
  enableClickthrough(gaugeName .. "_front")
  enableClickthrough(gaugeName .. "_text")
end

--- Set gauge to once again intercept mouse events
--- @param gaugeName
function disableGaugeClickthrough(gaugeName)
  assert(gaugesTable[gaugeName], "disableGaugeClickthrough: no such gauge exists.")
  disableClickthrough(gaugeName .. "_back")
  disableClickthrough(gaugeName .. "_front")
  disableClickthrough(gaugeName .. "_text")
end

--- Set gauge to have a tooltip
--- @param gaugeName
--- @param text the tooltip text
--- @param duration tooltip duration
function setGaugeToolTip(gaugeName, text, duration)
  duration = duration or 0
  assert(gaugesTable[gaugeName], "setGaugeToolTip: no such gauge exists.")
  setLabelToolTip(gaugeName .. "_text", text, duration)
end

--- Reset gauge tooltip
--- @param gaugeName
function resetGaugeToolTip(gaugeName)
  assert(gaugesTable[gaugeName], "resetGaugeToolTip: no such gauge exists.")
  resetLabelToolTip(gaugeName .. "_text")
end

--- Pads a hex number to ensure a minimum of 2 digits.
---
--- @usage Following command will returns "F0".
---   <pre>
---   PadHexNum("F")
---   </pre>
function PadHexNum(incString)
  assert(type(incString) == 'string', 'PadHexNum: bad argument #1 type (expected string, got '..type(incString)..'!)')
  local l_Return = incString
  if tonumber(incString, 16) < 16 then
    if tonumber(incString, 16) < 10 then
      l_Return = "0" .. l_Return
    elseif tonumber(incString, 16) > 10 then
      l_Return = l_Return .. "0"
    end
  end

  return l_Return
end



--- Converts an RGB value into an HTML compliant(label usable) HEX number.
--- This function is colorNames aware and can take any defined global color as its first argument.
---
--- @usage Both following commands will returns "FFFFFF".
---   <pre>
---   RGB2Hex(255,255,255)
---   RGB2Hex("white")
---   </pre>
---
--- @see showColor
function RGB2Hex(red, green, blue)
  assert(red, "RGB2Hex: require at least one argument (color_name or r,g,b)!")
  local l_Red, l_Green, l_Blue = 0, 0, 0
  if green == nil then
    -- Not an RGB but a "color" instead!
    l_Red, l_Green, l_Blue = getRGB(red)
  else -- Nope, true color here
    l_Red, l_Green, l_Blue = red, green, blue
  end

  return PadHexNum(string.format("%X", l_Red)) ..
  PadHexNum(string.format("%X", l_Green)) ..
  PadHexNum(string.format("%X", l_Blue))
end



--- Get RGB component from color name.
---
--- @usage Following will display "0.255.0" on your screen.
---   <pre>
---   local red, green, blue = getRGB("green")
---   echo(red .. "." .. green .. "." .. blue )
---   </pre>
function getRGB(colorName)
  assert(type(colorName) == 'string', 'getRGB: bad argument #1 type (expected string, got '..type(colorName)..'!)')
  local red = color_table[colorName][1]
  local green = color_table[colorName][2]
  local blue = color_table[colorName][3]
  return red, green, blue
end



--- Make your very own customized gauge with this function.
---
--- @usage This would make a gauge at that's 300px width, 20px in height, located at Xpos and Ypos and is green.
---   <pre>
---   createGauge("healthBar", 300, 20, 30, 300, nil, 0, 255, 0)
---   </pre>
--- @usage The second example is using the same names you'd use for something like fg() or bg().
---   <pre>
---   createGauge("healthBar", 300, 20, 30, 300, nil, "green")
---   </pre>
--- @usage Finally we'll add some text to our gauge.
---   <pre>
---   createGauge("healthBar", 300, 20, 30, 300, "Now with some text", "green")
---   </pre>
--- @usage You can add an orientation argument as well now:
---   <pre>
---   createGauge("healthBar", 300, 20, 30, 300, "Now with some text", "green", "horizontal, vertical, goofy, or batty")
---   </pre>
function createGauge(windowname, gaugeName, width, height, x, y, gaugeText, r, g, b, orientation)
  --Make windowname optional
  if type(gaugeName) == "number" then
    orientation = b
    b = g
    g = r
    r = gaugeText
    gaugeText = y
    y = x
    x = height
    height = width
    width = gaugeName
    gaugeName = windowname
    windowname = nil
  end
  windowname = windowname or "main"
  gaugeText = gaugeText or ""
  if type(r) == "string" then
    orientation = g
    r, g, b = getRGB(r)
  elseif r == nil then
    orientation = orientation or g
    -- default colors
    r, g, b = 128, 128, 128
  end

  assert(type(x) == 'number', 'createGauge: expected x to be a number (got '..type(x)..'!)')
  assert(type(y) == 'number', 'createGauge: expected y to be a number (got '..type(y)..'!)')
  assert(type(width) == 'number', 'createGauge: expected width to be a number (got '..type(width)..'!)')
  assert(type(height) == 'number', 'createGauge: expected height to be a number (got '..type(height)..'!)')

  orientation = orientation or "horizontal"
  assert(table.contains({ "horizontal", "vertical", "goofy", "batty" }, orientation), "createGauge: orientation must be horizontal, vertical, goofy, or batty")
  local tbl = { width = width, height = height, x = x, y = y, text = gaugeText, r = r, g = g, b = b, orientation = orientation, value = 1 }
  createLabel(windowname, gaugeName .. "_back", 0, 0, 0, 0, 1)
  setBackgroundColor(gaugeName .. "_back", r, g, b, 100)

  createLabel(windowname, gaugeName .. "_front", 0, 0, 0, 0, 1)
  setBackgroundColor(gaugeName .. "_front", r, g, b, 255)

  createLabel(windowname, gaugeName .. "_text", 0, 0, 0, 0, 1)
  setBackgroundColor(gaugeName .. "_text", 0, 0, 0, 0)

  -- save new values in table
  gaugesTable[gaugeName] = tbl
  resizeGauge(gaugeName, tbl.width, tbl.height)
  moveGauge(gaugeName, tbl.x, tbl.y)
  setGaugeText(gaugeName, gaugeText, "black")
  showGauge(gaugeName)
end



--- Use this function when you want to change the gauges look according to your values.
--- Typical usage would be in a prompt with your current health or whatever value, and throw
--- in some variables instead of the numbers.
---
--- @usage In that example, we'd change the looks of the gauge named healthBar and make it fill
---   to half of its capacity. The height is always remembered.
---   <pre>
---   setGauge("healthBar", 200, 400)
---   </pre>
--- @usage Change the text on your gauge.
---   <pre>
---   setGauge("healthBar", 200, 400, "some text")
---   </pre>
function setGauge(gaugeName, currentValue, maxValue, gaugeText)
  assert(gaugesTable[gaugeName], "setGauge: no such gauge exists.")
  assert(type(currentValue) == 'number', 'setGauge: bad argument #2 type (expected number, got '..type(currentValue)..'!)')
  assert(type(maxValue) == 'number', 'setGauge: bad argument #3 type (expected number, got '..type(maxValue)..'!)')
  local value = currentValue / maxValue
  -- save new values in table
  gaugesTable[gaugeName].value = value
  local info = gaugesTable[gaugeName]
  local x, y, w, h = info.x, info.y, info.width, info.height

  if info.orientation == "horizontal" then
    resizeWindow(gaugeName .. "_front", w * value, h)
    moveWindow(gaugeName .. "_front", x, y)
  elseif info.orientation == "vertical" then
    resizeWindow(gaugeName .. "_front", w, h * value)
    moveWindow(gaugeName .. "_front", x, y + h * (1 - value))
  elseif info.orientation == "goofy" then
    resizeWindow(gaugeName .. "_front", w * value, h)
    moveWindow(gaugeName .. "_front", x + w * (1 - value), y)
  elseif info.orientation == "batty" then
    resizeWindow(gaugeName .. "_front", w, h * value)
    moveWindow(gaugeName .. "_front", x, y)
  end
  if gaugeText then
    setGaugeText(gaugeName, gaugeText)
  end
end



--- Make a new console window with ease. The default background is black and text color white.
--- If you wish to change the color you can easily do this when updating your text or manually somewhere, using
--- setFgColor() and setBackgroundColor().
---
--- @usage This will create a miniconsole window that has a font size of 8pt, will display 80 characters in width,
---   hold a maximum of 20 lines and be place at 200x400 of your Mudlet window.
---   <pre>
---   createConsole("myConsoleWindow", 8, 80, 20, 200, 400)
---   </pre>
function createConsole(windowName, consoleName, fontSize, charsPerLine, numberOfLines, Xpos, Ypos)
  if Ypos == nil then
    Ypos = Xpos
    Xpos = numberOfLines
    numberOfLines = charsPerLine
    charsPerLine = fontSize
    fontSize = consoleName
    consoleName = windowName
    windowname = "main"
  end
  assert(type(windowName) == 'string', 'createConsole: invalid type for windowName (expected string, got '..type(windowName)..'!)')
  assert(type(consoleName) == 'string', 'createConsole: invalid type for consoleName (expected string, got '..type(consoleName)..'!)')
  assert(type(fontSize) == 'number', 'createConsole: invalid type for fontSize (expected number, got '..type(fontSize)..'!)')
  assert(type(charsPerLine) == 'number', 'createConsole: invalid type for charsPerLine (expected number, got '..type(charsPerLine)..'!)')
  assert(type(numberOfLines) == 'number', 'createConsole: invalid type for numberOfLines (expected number, got '..type(numberOfLines)..'!)')
  assert(type(Xpos) == 'number', 'createConsole: invalid type for Xpos (expected number, got '..type(Xpos)..'!)')
  assert(type(Ypos) == 'number', 'createConsole: invalid type for Ypos (expected number, got '..type(Ypos)..'!)')
  createMiniConsole(windowName, consoleName, 0, 0, 1, 1)
  setMiniConsoleFontSize(consoleName, fontSize)
  local x, y = calcFontSize( fontSize )
  resizeWindow(consoleName, x * charsPerLine, y * numberOfLines)
  setWindowWrap(consoleName, charsPerLine)
  moveWindow(consoleName, Xpos, Ypos)

  setBackgroundColor(consoleName, 0, 0, 0, 0)
  setFgColor(consoleName, 255, 255, 255)
end




--- Function will gag the whole line. <b>Use deleteLine() instead.</b>
function gagLine()
  deleteLine()
end



--- Replaces all occurrences of what in the current line with <i>with</i>.
---
--- @usage This will replace all occurrences of John with the word Doe.
---   <pre>
---   replaceAll("John", "Doe")
---
---   -- also handles recursive matches:
---   replaceAll("you", "you and me")
---   </pre>
function replaceAll(word, what, keepColor)
  assert(type(word) == 'string', 'replaceAll: bad argument #1 type (expected string, got '..type(word)..'!)')
  assert(type(what) == 'string', 'replaceAll: bad argument #2 type (expected string, got '..type(what)..'!)')
  local getCurrentLine, selectSection, replace = getCurrentLine, selectSection, replace
  local startp, endp = 1, 1
  while true do
    startp, endp = getCurrentLine():find(word, endp)
    if not startp then
      break
    end
    selectSection(startp - 1, endp - startp + 1)
    replace(what, keepColor)
    endp = endp + (#what - #word) + 1 -- recalculate the new word ending to start search from there
  end
end



--- Replace an entire line with a string you'd like.
---
--- @see deleteLine
function replaceLine(window, text)
  assert(type(window) == 'string', 'replaceLine: bad argument #1 type (expected string, got '..type(window)..'!)')
  if not text then
    selectCurrentLine()
    text = window
  else
    selectCurrentLine(window)
  end
  replace("")
  insertText(text)
end



--- Default resizeEvent handler function. Overwrite this function to make a custom event handler
--- if the main window is being resized. <br/><br/>
---
--- The standard implementation of this function does nothing. However, this function gets called whenever
--- the main window is being manually resized. You can overwrite this function in your own scripts to handle window
--- resize events yourself and e.g. adjust the screen position and size of your mini console windows, labels or
--- other relevant GUI elements in your scripts that depend on the size of the main Window. To override this
--- function you can simply put a function with the same name in one of your scripts thus overwriting the
--- original empty implementation of this function.
---   <pre>
---   function handleWindowResizeEvent()
---      -- determine the size of your screen
---      WindowWidth=0;
---      WindowHeight=0;
---      WindowWidth, WindowHeight = getMainWindowSize();
---      -- move mini console "sys" to the far right side of the screen whenever the screen gets resized
---      moveWindow("sys",WindowWidth-300,0)
---   end
---   </pre>
function handleWindowResizeEvent()
end


--- Sets current background color to a named color.
---
--- @usage Set background color to magenta.
---   <pre>
---   bg("magenta")
---
---   bg("my miniconsole", "blue")
---   </pre>
---
--- @see fg
--- @see showColors
function bg(console, colorName)
  local colorName = colorName or console
  if colorName == nil then
    error("bg: bad argument #1 type (color name as string expected, got nil)!")
  end
  if not color_table[colorName] then
    error(string.format("bg: '%s' color doesn't exist - see showColors()", colorName))
  end
  local alpha = color_table[colorName][4] or 255

  if console == colorName or console == "main" then
    setBgColor(color_table[colorName][1], color_table[colorName][2], color_table[colorName][3], alpha)
  else
    setBgColor(console, color_table[colorName][1], color_table[colorName][2], color_table[colorName][3], alpha)
  end
end



--- Sets current foreground color to a named color.
---
--- @usage Set foreground color to black.
---   <pre>
---   fg("black")
---   </pre>
---
--- @see bg
--- @see showColors
function fg(console, colorName)
  local colorName = colorName or console
  if colorName == nil then
    error("fg: bad argument #1 type (color name as string expected, got nil)!")
  end
  if not color_table[colorName] then
    error(string.format("fg: '%s' color doesn't exist - see showColors()", colorName))
  end

  if console == colorName or console == "main" then
    setFgColor(color_table[colorName][1], color_table[colorName][2], color_table[colorName][3])
  else
    setFgColor(console, color_table[colorName][1], color_table[colorName][2], color_table[colorName][3])
  end
end



--- Replaces the given wildcard (as a number) with the given text.
---
--- @usage Replace "goodbye" with "hello" on a trigger of "^You wave (goodbye)\.$"
---   <pre>
---   replaceWildcard(2, "hello")
---   </pre>
---   Is equivalent to doing:
---   <pre>
---   selectString(matches[2], 1)
---   replace("hello")
---   </pre>
function replaceWildcard(what, replacement, keepColor)
  if replacement == nil or what == nil then
    return
  end
  selectCaptureGroup(what)
  replace(replacement, keepColor)
end

-- internal sorting function, sorts first by hue, then luminosity, then value
local sortColorsByHue = function(lhs,rhs)
  local lh,ll,lv = unpack(lhs.sort)
  local rh,rl,rv = unpack(rhs.sort)
  if lh < rh then
    return true
  elseif lh > rh then
    return false
  elseif ll < rl then
    return true
  elseif ll > rl then
    return false
  else
    return lv < rv
  end
end

-- internal sorting function, removes _ from snake_case and compares to camelCase
local sortColorsByName = function(a,b)
  local aname = string.gsub(string.lower(a.name), "_", "")
  local bname = string.gsub(string.lower(b.name), "_", "")
  return aname < bname
end

-- internal function, converts rgb to hsv
-- found at https://github.com/EmmanuelOga/columns/blob/master/utils/color.lua#L89
local rgbToHsv = function(r, g, b)
  r, g, b = r / 255, g / 255, b / 255
  local max, min = math.max(r, g, b), math.min(r, g, b)
  local h, s, v
  v = max

  local d = max - min
  if max == 0 then
    s = 0
  else
    s = d / max
  end

  if max == min then
    h = 0 -- achromatic
  else
    if max == r then
      h = (g - b) / d
      if g < b then h = h + 6 end
    elseif max == g then
      h = (b - r) / d + 2
    elseif max == b then
      h = (r - g) / d + 4
    end
    h = h / 6
  end

  return h, s, v
end

-- internal stepping function, removes some of the noise for a more pleasing sort
-- cribbed from the python on https://www.alanzucconi.com/2015/09/30/colour-sorting/
local step = function(r,g,b)
  local lum = math.sqrt( .241 * r + .691 * g + .068 * b )
  local reps = 8

  local h, s, v = rgbToHsv(r,g,b)

  local h2 = math.floor(h * reps)
  local v2 = math.floor(v * reps)
  if h2 % 2 == 1 then
    v2 = reps - v2
    lum = reps - lum
  end
  return h2, lum, v2
end

local function calc_luminosity(r,g,b)
  r = r < 11 and r / (255 * 12.92) or ((0.055 + r / 255) / 1.055) ^ 2.4
  g = g < 11 and g / (255 * 12.92) or ((0.055 + g / 255) / 1.055) ^ 2.4
  b = b < 11 and b / (255 * 12.92) or ((0.055 + b / 255) / 1.055) ^ 2.4
  return (0.2126 * r) + (0.7152 * g) + (0.0722 * b)
end


--- Prints out a formatted list of all available named colors (EXCEPT FOR
--- the 256 colors with names of form "ansi_###" where ### is 000 to 255),
--- optional args specifies:
--- * (number) number of columns to print in, defaults to 4;
--- * (string) substring required to match to include in output, defaults to
--- showing all if not supplied;
--- * (boolean) whether to sort the output, defaults to false.
--- @usage Print list in 4 columns by default.
---   <pre>
---   showColors()
---   </pre>
--- @usage Print list in 2 columns.
---   <pre>
---   showColors(2)
---   </pre>
---
--- @see color_table
function showColors(...)
  local cols, search, sort = 4, "", false
  for _, val in ipairs(arg) do
    if type(val) == "string" then
      search = val:lower()
    elseif type(val) == "number" then
      cols = val
    elseif type(val) == "boolean" then
      sort = val
    end
  end

  local colors = {}
  for k, v in pairs(color_table) do
    local color = {}
    color.rgb = v
    color.name = k
    color.sort = {step(unpack(v))}
    if not string.find(k, "ansi_%d%d%d") then
      table.insert(colors,color)
    end
  end

  if sort then
    table.sort(colors, sortColorsByName)
  else
    table.sort(colors,sortColorsByHue)
  end
  local i = 1
  for _, k in ipairs(colors) do
    if k.name:lower():find(search) then
      local v = k.rgb
      local fgc = "white"
      if calc_luminosity(v[1],v[2],v[3]) > 0.5 then
        fgc = "black"
      end
      cechoLink(string.format('<%s:%s> %-23s<reset> ',fgc,k.name,k.name), [[appendCmdLine("]] .. k.name .. [[")]], table.concat(v, ", "), true)
      if i == cols then
        echo("\n")
        i = 1
      else
        i = i + 1
      end
    end
  end
  if i ~= 1 then echo("\n") end
end

--- Prints out a sorted, formatted list of the 256 colors with names of form
--- "ansi_###" where ### is 000 to 255), optional arg specifies:
--- * (number) number of columns to print in, defaults to 4;
--- @usage Print list in 4 columns by default.
---   <pre>
---   showAnsiColors()
---   </pre>
--- @usage Print list in 2 columns.
---   <pre>
---   showAnsiColors(2)
---   </pre>
---
--- @see color_table
function showAnsiColors(...)
  local cols = 8
  for _, val in ipairs(arg) do
    if type(val) == "number" then
      cols = val
    end
  end

  local colors = {}
  for k, v in pairs(color_table) do
    -- Only use the ansi_### 256 colors entries
    if string.find(k, "ansi_%d%d%d") then
      table.insert(colors,k)
    end
  end

  table.sort(colors)

  local i = 1
  for _, k in ipairs(colors) do
    local v = color_table[k]
    local fgc = "white"
    if calc_luminosity(v[1],v[2],v[3]) > 0.5 then
      fgc = "black"
    end
    cechoLink(string.format('<%s:%s> %8s <reset> ',fgc,k,k), [[printCmdLine("]] .. k .. [[")]], table.concat(v, ", "), true)
    if i == cols then
      echo("\n")
      i = 1
    else
      i = i + 1
    end
  end
  if i ~= 1 then echo("\n") end
end


--- <b><u>TODO</u></b> resizeGauge(gaugeName, width, height)
function resizeGauge(gaugeName, width, height)
  assert(gaugesTable[gaugeName], "resizeGauge: no such gauge exists.")
  assert(type(width) == 'number', 'resizeGauge: bad argument #2 type (expected number, got '..type(width)..'!)')
  assert(type(height) == 'number', 'resizeGauge: bad argument #3 type (expected number, got '..type(height)..')')
  resizeWindow(gaugeName .. "_back", width, height)
  resizeWindow(gaugeName .. "_text", width, height)
  -- save new values in table
  gaugesTable[gaugeName].width, gaugesTable[gaugeName].height = width, height
  setGauge(gaugeName, gaugesTable[gaugeName].value, 1)
end



--- <b><u>TODO</u></b> setGaugeStyleSheet(gaugeName, css, cssback)
function setGaugeStyleSheet(gaugeName, css, cssback, csstext)
  if not setLabelStyleSheet then
    return
  end -- mudlet 1.0.5 and lower compatibility
  assert(gaugesTable[gaugeName], "setGaugeStyleSheet: no such gauge exists.")
  assert(type(css) == 'string', 'setGaugeStyleSheet: bad argument #2 type (expected string, got '..type(css)..'!)')
  setLabelStyleSheet(gaugeName .. "_back", cssback or css)
  setLabelStyleSheet(gaugeName .. "_front", css)
  setLabelStyleSheet(gaugeName .. "_text", csstext or "")
end

-- https://wiki.mudlet.org/w/Manual:Lua_Functions#getHTMLformat
-- used by xEcho for creating formatting span tags for labels
-- fmt is a table of format options as returned by getTextFormat
function getHTMLformat(fmt)
  assert(type(fmt) == 'table', 'getHTMLformat: bad argument #1 type (expected table, got '..type(fmt)..'!)')
  -- next two lines effectively invert the colors if fmt.reverse is true
  local type = type
  local sfmt = string.format
  local fore = fmt.foreground
  local back = fmt.background
  if fmt.reverse then
    if type(back) ~= "table" then
      if back:find("Q.-Gradient") then
        -- we can't gradient the foreground, so we'll make the background the foreground, and the foreground the inverse of that
        if type(fore) == "table" then
          back = table.deepcopy(fore)
        else
          local r,g,b = fore:match("rgb%((%d+),%s*(%d+),%s*(%d+))")
          if b then
            back = { r, g, b, 255 }
          else
            local hexstring = fore:match("(#......)")
            if hexstring then
              r,g,b = Geyser.Color.parse(hexstring)
            else
              back = { 255, 255, 255, 255 } -- can't parse the foreground, default to black on white
            end
          end
        end
        for index,value in ipairs(back) do
          fore[index] = 255 - value
        end
      elseif back:find("rgba") then
        local r,g,b = back:match("rgba%((%d+),%s*(%d+),%s*(%d+),%s*%d+%)")
        fore = { r, g, b }
        back = fmt.foreground
      else
        -- back should work as-is as a foreground
        fore = fmt.background
        back = fmt.foreground
      end
    else
      fore = fmt.background
      back = fmt.foreground
    end
  end

  local color,background
  if type(fore) == "table" then
    color = sfmt("color: rgb(%d, %d, %d);", unpack(fore))
  else
    color = sfmt("color: %s;", fore)
  end
  if type(back) == "table" then
    back[4] = back[4] or 255 -- if alpha isn't specified, assume 255
    background = sfmt("background-color: rgba(%d, %d, %d, %d);", unpack(back))
  else
    background = sfmt("background-color: %s;", back)
  end
  local bold = fmt.bold and " font-weight: bold;" or " font-weight: normal;"
  local italic = fmt.italic and " font-style: italic;" or " font-style: normal;"
  local textDecoration
  if not (fmt.underline or fmt.overline or fmt.strikeout) then
    textDecoration = " text-decoration: none;"
  else
    textDecoration = sfmt(" text-decoration:%s%s%s;", fmt.overline and " overline" or "", fmt.underline and " underline" or "", fmt.strikeout and " line-through" or "")
  end
  local result = sfmt([[<span style="%s%s%s%s%s">]], color, background, bold, italic, textDecoration)
  return result
end

-- https://wiki.mudlet.org/w/Manual:Lua_Functions#getLabelFormat
-- used by xEcho for getting the default format for a label, taking into account
-- the background color setting and stylesheet
function getLabelFormat(win)
  assert(win, "getLabelFormat: requires at least one argument")
  local r,g,b = 192, 192, 192
  local reset = {
    foreground = { r, g, b },
    background = "rgba(0, 0, 0, 0)",
    bold = false,
    italic = false,
    overline = false,
    reverse = false,
    strikeout = false,
    underline = false,
  }
  local stylesheet = getLabelStyleSheet(win)
  if stylesheet ~= "" then
    if stylesheet:find(";") then
      local styleTable = {}
      stylesheet = stylesheet:gsub("[\r\n]", "")
      for _,element in ipairs(stylesheet:split(";")) do
        element = element:trim()
        local attr, val = element:match("^(.-):(.+)$")
        if attr and val then
          styleTable[attr:trim()] = val:trim()
        end
      end

      if styleTable.color then
        reset.foreground = styleTable.color
      end

      if styleTable["text-decoration"] then
        local td = styleTable["text-decoration"]
        if td:match("underline") then
          reset.underline = true
        end
        if td:match("overline") then
          reset.overline = true
        end
        if td:match("line%-through") then
          reset.strikeout = true
        end
      end

      if styleTable.font then
        reset.bold = styleTable.font:match("bold") and true or false
        reset.italic = styleTable.font:match("italic") and true or false
      end

      if styleTable["font-weight"] and styleTable["font-weight"]:match("bold") then
        reset.bold = true
      end

      if styleTable["font-style"] and styleTable["font-style"]:match("italic") then
        reset.italic = true
      end
    end
  end
  return reset
end

local processedEchoToHTML

if rex then
  _Echos = {
    Patterns = {
      Hex = {
        [[(\x5c?(?:#|\|c)?(?:[0-9a-fA-F]{6}|(?:#,|\|c,)[0-9a-fA-F]{6,8})(?:,[0-9a-fA-F]{6,8})?)|(?:\||#)(\/?[biruso])]],
        rex.new [[(?:#|\|c)(?:([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?(?:,([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})?)?]],
      },
      Decimal = {
        [[(<[0-9,:]+>)|<(/?[biruso])>]],
        rex.new [[<(?:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?(?::(?=>))?(?::([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}),?([0-9]{1,3})?)?>]],
      },
      Color = {
        [[(</?[a-zA-Z0-9_,:]+>)]],
        rex.new [[<([a-zA-Z0-9_]+)?(?:[:,](?=>))?(?:[:,]([a-zA-Z0-9_]+))?>]],
      },
      Ansi = {
        [[(<[0-9,:]+>)]],
        rex.new [[<([0-9]{1,2})?(?::([0-9]{1,2}))?>]],
      },
    },
    Process = function(str, style)
      local t = {}
      local tonumber, _Echos, color_table = tonumber, _Echos, color_table

      -- s: A subject section (can be an empty string)
      -- c: colour code
      -- r: reset code
      for s, c, r in rex.split(str, _Echos.Patterns[style][1]) do
        if c and (c:byte(1) == 92) then
          c = c:sub(2)
          if s then
            s = s .. c else s = c
          end
          c = nil
        end
        if s then
          t[#t + 1] = s
        end
        if r == 'r' then
          t[#t + 1] = "\27reset"
        elseif r == "b" then
          t[#t + 1] = "\27bold"
        elseif r == "/b" then
          t[#t + 1] = "\27boldoff"
        elseif r == "i" then
          t[#t + 1] = "\27italics"
        elseif r == "/i" then
          t[#t + 1] = "\27italicsoff"
        elseif r == "u" then
          t[#t + 1] = "\27underline"
        elseif r == "/u" then
          t[#t + 1] = "\27underlineoff"
        elseif r == "s" then
          t[#t + 1] = "\27strikethrough"
        elseif r == "/s" then
          t[#t + 1] = "\27strikethroughoff"
        elseif r == "o" then
          t[#t + 1] = "\27overline"
        elseif r == "/o" then
          t[#t + 1] = "\27overlineoff"
        end
        if c then
          if style == 'Hex' or style == 'Decimal' then
            local fr, fg, fb, br, bg, bb, ba = _Echos.Patterns[style][2]:match(c)
            local color = {}
            if style == 'Hex' then
              -- hex has alpha value in front
              if ba then
                local temp = ba
                ba = br
                br = bg
                bg = bb
                bb = temp
              else
                ba = "ff"
              end
              if fr and fg and fb then
                fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
              end
              if br and bg and bb and ba  then
                ba, br, bg, bb = tonumber(ba, 16), tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
              end
            end
            if fr and fg and fb then
              color.fg = { fr, fg, fb }
            end
            ba = ba or 255
            if br and bg and bb and ba then
              color.bg = { br, bg, bb, ba }
            end

            -- if the colour failed to match anything, then what we captured in <> wasn't a colour -
            -- pass it into the text stream then
            t[#t + 1] = ((fr or br) and color or c)
          elseif style == 'Color' then
            if c == "<reset>" or c == "<r>" then
              t[#t + 1] = "\27reset"
            elseif c == "<b>" then
              t[#t + 1] = "\27bold"
            elseif c == "</b>" then
              t[#t + 1] = "\27boldoff"
            elseif c == "<i>" then
              t[#t + 1] = "\27italics"
            elseif c == "</i>" then
              t[#t + 1] = "\27italicsoff"
            elseif c == "<u>" then
              t[#t + 1] = "\27underline"
            elseif c == "</u>" then
              t[#t + 1] = "\27underlineoff"
            elseif c == "<s>" then
              t[#t + 1] = "\27strikethrough"
            elseif c == "</s>" then
              t[#t + 1] = "\27strikethroughoff"
            elseif c == "<o>" then
              t[#t + 1] = "\27overline"
            elseif c == "</o>" then
              t[#t + 1] = "\27overlineoff"
            else
              local fcolor, bcolor = _Echos.Patterns[style][2]:match(c)
              local color = {}
              if fcolor and color_table[fcolor] then
                color.fg = color_table[fcolor]
              end
              if bcolor and color_table[bcolor] then
                color.bg = color_table[bcolor]
              end
              if color.fg or color.bg then
                t[#t + 1] = color
              else
                t[#t + 1] = c
              end
            end
          end
        end
      end
      return t
    end,
  }

-- internal function which takes a processed echo table and a table of 'default'
-- formatting options and returns it as an html string. used by xEcho for Label
-- outputs and the html output for c/d/hecho2html functions.
processedEchoToHTML = function(t, reset)
  reset = reset or {
    background = { 0, 0, 0 },
    bold = false,
    foreground = { 255, 255, 255 },
    italic = false,
    overline = false,
    reverse = false,
    strikeout = false,
    underline = false
  }
  local format = table.deepcopy(reset)
  local result = getHTMLformat(format)
  for _,v in ipairs(t) do
    local formatChanged = false
    if type(v) == "table" then
      if v.fg then
        format.foreground = {v.fg[1], v.fg[2], v.fg[3]}
        formatChanged = true
      end
      if v.bg then
        format.background = {v.bg[1], v.bg[2], v.bg[3]}
        formatChanged = true
      end
    elseif v == "\27bold" then
      format.bold = true
      formatChanged = true
    elseif v == "\27boldoff" then
      format.bold = false
      formatChanged = true
    elseif v == "\27italics" then
      format.italic = true
      formatChanged = true
    elseif v == "\27italicsoff" then
      format.italic = false
      formatChanged = true
    elseif v == "\27underline" then
      format.underline = true
      formatChanged = true
    elseif v == "\27underlineoff" then
      format.underline = false
      formatChanged = true
    elseif v == "\27strikethrough" then
      format.strikeout = true
      formatChanged = true
    elseif v == "\27strikethroughoff" then
      format.strikeout = false
      formatChanged = true
    elseif v == "\27overline" then
      format.overline = true
      formatChanged = true
    elseif v == "\27overlineoff" then
      format.overline = false
      formatChanged = true
    elseif v == "\27reset" then
      format = table.deepcopy(reset)
      formatChanged = true
    end
    v = formatChanged and getHTMLformat(format) or v
    result = result .. v
  end
  return result
end

  --- Generic color echo and insert function (allowing hecho, decho, cecho, hinsertText, dinsertText and cinsertText).
  ---
  --- @param style Hex, Decimal or Color
  --- @param insert boolean flag to determine echo/insert behaviour
  --- @param win windowName optional
  --- @param str text with embedded color information
  ---
  --- @see cecho
  --- @see decho
  --- @see hecho
  --- @see cinsertText
  --- @see dinsertText
  --- @see hinsertText
  function xEcho(style, func, ...)
    local win, str, cmd, hint, fmt
    local out
    local args = { ... }
    local n = #args

    assert(type(args[1]) == 'string', style:sub(1,1):lower() .. func .. ': bad argument #1, string expected, got '..type(args[1])..'!)')

    if string.find(func, "Link") then
      if n < 3 then
        error 'Insufficient arguments, usage: ([window, ] string, command, hint)'
      elseif n == 3 then
        str, cmd, hint = ...
      elseif n == 4 and type(args[4]) == 'boolean' then
        str, cmd, hint, fmt = ...
      elseif n >= 4 and type(args[4]) == 'string' then
        win, str, cmd, hint, fmt = ...
      else
        error 'Improper arguments, usage: ([window, ] string, command, hint)'
      end
    elseif string.find(func, "Popup") then
      if n < 3 then
        error 'Insufficient arguments, usage: ([window, ] string, {commands}, {hints})'
      elseif n == 3 then
        str, cmd, hint = ...
      elseif n == 4 and type(args[4]) == 'boolean' then
        str, cmd, hint, fmt = ...
      elseif n >= 4 and type(args[4]) == 'table' then
        win, str, cmd, hint, fmt = ...
      else
        error 'Improper arguments, usage: ([window, ] string, {commands}, {hints})'
      end

    else
      if args[1] and args[2] and args[1] ~= "main" then
        win, str = args[1], args[2]
      elseif args[1] and args[2] and args[1] == "main" then
        str = args[2]
      else
        str = args[1]
      end
    end
    win = win or "main"

    out = function(...)
      _G[func](...)
    end

    if windowType(win) == "label" and win ~= "main" then
      str = str:gsub("\n", "<br>")
      local t = _Echos.Process(str, style)
      if func ~= "echo" then
        return nil, "you cannot use echoLink, echoPopup, or insertText with Labels"
      end
      local reset = getLabelFormat(win)
      local result = processedEchoToHTML(t, reset)
      echo(win, result)
    else
      local t = _Echos.Process(str, style)
      deselect(win)
      resetFormat(win)
      for _, v in ipairs(t) do
        if type(v) == 'table' then
          if v.fg then
            local fr, fg, fb = unpack(v.fg)
            setFgColor(win, fr, fg, fb)
          end
          if v.bg then
            local br, bg, bb, ba = unpack(v.bg)
            ba = ba or 255
            setBgColor(win, br, bg, bb, ba)
          end
        elseif v == "\27bold" then
          setBold(win, true)
        elseif v == "\27boldoff" then
          setBold(win, false)
        elseif v == "\27italics" then
          setItalics(win, true)
        elseif v == "\27italicsoff" then
          setItalics(win, false)
        elseif v == "\27underline" then
          setUnderline(win, true)
        elseif v == "\27underlineoff" then
          setUnderline(win, false)
        elseif v == "\27strikethrough" then
          setStrikeOut(win, true)
        elseif v == "\27strikethroughoff" then
          setStrikeOut(win, false)
        elseif v == "\27overline" then
          setOverline(win, true)
        elseif v == "\27overlineoff" then
          setOverline(win, false)
        elseif v == "\27reset" then
          resetFormat(win)
        else
          if func == 'echo' or func == 'insertText' then
            out(win, v)
            if func == 'insertText' then
              moveCursor(win, getColumnNumber(win) + string.len(v), getLineNumber(win))
            end
          else
            -- if fmt then setUnderline(win, true) end -- not sure if underline is necessary unless asked for
            out(win, v, cmd, hint, (fmt == true and true or false))
          end
        end
      end
      resetFormat(win)
    end
  end



  --- Echo string with embedded hex color information. <br/><br/>
  ---
  --- Color changes can be made within the string using the format |cFRFGFB,BRBGBB where FR is the foreground red value,
  --- FG is the foreground green value, FB is the foreground blue value, BR is the background red value, etc., BRBGBB is optional.
  --- |r can be used within the string to reset the colors to default.
  ---
  --- @usage Print red test on green background.
  ---   <pre>
  ---   hecho("|cff0000,00ff00test")
  ---   </pre>
  ---
  --- @see xEcho
  --- @see hinsertText
  function hecho(...)
    xEcho("Hex", "echo", ...)
  end



  --- Echo string with embedded decimal color information. <br/><br/>
  ---
  --- Color changes can be made using the format &lt;FR,FG,FB:BR,BG,BB&gt; where each field is a number from 0 to 255.
  --- The background portion can be omitted using &lt;FR,FG,FB&gt; or the foreground portion can be omitted using &lt;:BR,BG,BB&gt;.
  ---
  --- @usage Print red test on green background.
  ---   <pre>
  ---   decho("&lt;255,0,0:0,255,0&gt;test")
  ---   </pre>
  ---
  --- @see xEcho
  --- @see dinsertText
  function decho(...)
    xEcho("Decimal", "echo", ...)
  end



  --- Echo string with embedded color name information.
  ---
  --- @usage Consider following example:
  ---   <pre>
  ---   cecho("&lt;green&gt;green text &lt;blue&gt;blue text &lt;red&gt;red text")
  ---   </pre>
  ---
  --- @see xEcho
  --- @see cinsertText
  function cecho(...)
    xEcho("Color", "echo", ...)
  end


  --- Inserts string with embedded hex color information.
  ---
  --- @see xEcho
  --- @see hecho
  function hinsertText(...)
    xEcho("Hex", "insertText", ...)
  end


  --- Inserts string with embedded decimal color information.
  ---
  --- @see xEcho
  --- @see decho
  function dinsertText(...)
    xEcho("Decimal", "insertText", ...)
  end


  --- Inserts string with embedded color name information.
  ---
  --- @see xEcho
  --- @see cecho
  function cinsertText(...)
    xEcho("Color", "insertText", ...)
  end


  --- Echos a link with embedded hex color information.
  ---
  --- @usage hechoLink([window, ] string, command, hint)
  ---
  --- @see xEcho
  --- @see hecho
  function hechoLink(...)
    xEcho("Hex", "echoLink", ...)
  end


  --- Echos a link with embedded decimal color information.
  ---
  --- @usage dechoLink([window, ] string, command, hint)
  ---
  --- @see xEcho
  --- @see decho
  function dechoLink(...)
    xEcho("Decimal", "echoLink", ...)
  end


  --- Echos a link with embedded color name information.
  ---
  --- @usage cechoLink([window, ] string, command, hint)
  ---
  --- @see xEcho
  --- @see cecho
  function cechoLink(...)
    xEcho("Color", "echoLink", ...)
  end

  --- Inserts a link with embedded color name information at the current position
  ---
  --- @usage cinsertLink([window, ] string, command, hint)
  ---
  --- @see xEcho
  --- @see cecho
  function cinsertLink(...)
    xEcho("Color", "insertLink", ...)
  end

  --- Inserts a link with embedded decimal color information at the current position
  ---
  --- @usage dinsertLink([window, ] string, command, hint)
  ---
  --- @see xEcho
  --- @see decho
  function dinsertLink(...)
    xEcho("Decimal", "insertLink", ...)
  end

  --- Inserts a link with embedded hex color information at the current position
  ---
  --- @usage hinsertLink([window, ] string, command, hint)
  ---
  --- @see xEcho
  --- @see hecho
  function hinsertLink(...)
    xEcho("Hex", "insertLink", ...)
  end

  --- Echos a popup with embedded color name information.
  ---
  --- @usage cechoPopup([window, ] string, {commands}, {hints})
  ---
  --- @see xEcho
  --- @see cecho
  function cechoPopup(...)
    xEcho("Color", "echoPopup", ...)
  end

  --- Echos a popup with embedded color name information.
  ---
  --- @usage dechoPopup([window, ] string, {commands}, {hints})
  ---
  --- @see xEcho
  --- @see decho
  function dechoPopup(...)
    xEcho("Decimal", "echoPopup", ...)
  end

  --- Echos a popup with embedded hex color information.
  ---
  --- @usage hechoPopup([window, ] string, {commands}, {hints})
  ---
  --- @see xEcho
  --- @see hecho
  function hechoPopup(...)
    xEcho("Hex", "echoPopup", ...)
  end

  --- Echos a popup with embedded color name information.
  ---
  --- @usage cinsertPopup([window, ] string, {commands}, {hints})
  ---
  --- @see xEcho
  --- @see cecho
  function cinsertPopup(...)
    xEcho("Color", "insertPopup", ...)
  end

  --- Echos a popup with embedded decimal color information.
  ---
  --- @usage dinsertPopup([window, ] string, {commands}, {hints})
  ---
  --- @see xEcho
  --- @see decho
  function dinsertPopup(...)
    xEcho("Decimal", "insertPopup", ...)
  end

  --- Echos a popup with embedded hex color information.
  ---
  --- @usage hinsertPopup([window, ] string, {commands}, {hints})
  ---
  --- @see xEcho
  --- @see hecho
  function hinsertPopup(...)
    xEcho("Hex", "insertPopup", ...)
  end


  -- Backwards compatibility
  checho = cecho

  -- table to facilitate converting color names to ansi escapes
  local ctable =
  {
    black = "0",
    red = "1",
    green = "2",
    yellow = "3",
    blue = "4",
    magenta = "5",
    cyan = "6",
    white = "7",
    light_black = "8",
    light_red = "9",
    light_green = "10",
    light_yellow = "11",
    light_blue = "12",
    light_magenta = "13",
    light_cyan = "14",
    light_white = "15",
    lightBlack = "8",
    lightRed = "9",
    lightGreen = "10",
    lightYellow = "11",
    lightBlue = "12",
    lightMagenta = "13",
    lightCyan = "14",
    lightWhite = "15",
  }
  for i = 0, 255 do
    local key = tostring(i)
    ctable[key] = key
    ctable["ansi_" .. key] = key
  end

  -- local lookup table to find ansi escapes for to ansi conversions
  local resets = {
    ["r"]     = "\27[0m",
    ["reset"] = "\27[0m",
    ["i"]     = "\27[3m",
    ["/i"]    = "\27[23m",
    ["b"]     = "\27[1m",
    ["/b"]    = "\27[22m",
    ["u"]     = "\27[4m",
    ["/u"]    = "\27[24m",
    ["s"]     = "\27[9m",
    ["/s"]    = "\27[29m",
    ["o"]     = "\27[53m",
    ["/o"]    = "\27[55m"
  }

  -- take a color name and turn it into an ANSI escape string
  local function colorToAnsi(colorName)
    local result = ""
    local cols = colorName:split(":")
    local fore = cols[1]
    local back = cols[2]
    if fore ~= "" then
      local res = resets[fore]
      if res then
        result = result .. res
      else
        local colorNumber = ctable[fore]
        if colorNumber then
          result = string.format("%s\27[38:5:%sm", result, colorNumber)
        elseif color_table[fore] then
          local rgb = color_table[fore]
          result = string.format("%s\27[38:2::%s:%s:%sm", result, rgb[1], rgb[2], rgb[3])
        end
      end
    end
    if back then
      local colorNumber = ctable[back]
      if colorNumber then
        result = string.format("%s\27[48:5:%sm", result, colorNumber)
      elseif color_table[back] then
        local rgb = color_table[back]
        result = string.format("%s\27[48:2::%s:%s:%sm", result, rgb[1], rgb[2], rgb[3])
      end
    end
    return result
  end

  -- converts decho color information to ansi escape sequences
  local function rgbToAnsi(rgb)
    local result = ""
    local cols = rgb:split(":")
    local fore = cols[1]
    local back = cols[2]
    if fore ~= "" then
      local components = fore:split(",")
      result = string.format("%s\27[38:2::%s:%s:%sm", result, components[1] or "0", components[2] or "0", components[3] or "0")
    end
    if back then
      local components = back:split(",")
      result = string.format("%s\27[48:2::%s:%s:%sm", result, components[1] or "0", components[2] or "0", components[3] or "0")
    end
    return result
  end

  -- converts a 6 digit hex color code to ansi escape sequence
  local function hexToAnsi(hexcode)
    local result = ""
    local cols = hexcode:split(",")
    local fore = cols[1]
    local back = cols[2]
    if fore ~= "" then
      local components = {
        tonumber(fore:sub(1,2),16),
        tonumber(fore:sub(3,4),16),
        tonumber(fore:sub(5,6),16)
      }
      result = string.format("%s\27[38:2::%s:%s:%sm", result, components[1] or "0", components[2] or "0", components[3] or "0")
    end
    if back then
      local components = {
        tonumber(back:sub(1,2),16),
        tonumber(back:sub(3,4),16),
        tonumber(back:sub(5,6),16)
      }
      result = string.format("%s\27[48:2::%s:%s:%sm", result, components[1] or "0", components[2] or "0", components[3] or "0")
    end
    return result
  end

  function cecho2ansi(text)
    assert(type(text) == 'string', 'cecho2ansi: bad argument #1 type (expected string, got '..type(text)..'!)')
    local colorPattern = _Echos.Patterns.Color[1]
    local result = ""
    for str, color in rex.split(text, colorPattern) do
      result = result .. str
      if color then
        result = result .. colorToAnsi(color:match("<(.+)>"))
      end
    end
    return result
  end

  --- feedTriggers with cecho style color information.
  -- Valid colors are  black,red,green,yellow,blue,magenta,cyan,white and light_* versions of same
  -- Can also pass in a number between 0 and 255 to use the expanded ansi 255 colors. IE <124> will set foreground to the color ANSI124
  -- Will also take ansi colors as ansi_#, IE <ansi_124>
  -- Reset using <r> or <reset>
  --@param text the text to pump into feedTriggers
  --@see cecho
  --@see cinsertText
  function cfeedTriggers(text)
    assert(type(text) == 'string', 'cfeedTriggers: bad argument #1 type (expected string, got '..type(text)..'!)')
    feedTriggers(cecho2ansi(text) .. "\n")
    echo("")
  end

  --- Returns a string with decho style color codes converted to ANSI color
  -- IE <128,0,0> for red, <0,128,0> for green, <0,128,0:128,0,0> for green on red background.
  -- <r> to reset
  --@param text the text to convert to ansi colors
  --@see decho
  --@see dinsertText
  function decho2ansi(text)
    assert(type(text) == 'string', 'decho2ansi: bad argument #1 type (expected string, got '..type(text)..'!)')
    local colorPattern = _Echos.Patterns.Decimal[1]
    local result = ""
    for str, color, res in rex.split(text, colorPattern) do
      result = result .. str
      if color then
        result = result .. rgbToAnsi(color:match("<(.+)>"))
      end
      if res then
        result = result .. resets[res]
      end
    end
    return result
  end

  --- feedTriggers with decho style color information.
  -- IE <128,0,0> for red, <0,128,0> for green, <0,128,0:128,0,0> for green on red background.
  -- <r> to reset
  --@param text the text to pump into feedTriggers
  --@see decho
  --@see dinsertText
  function dfeedTriggers(text)
    assert(type(text) == 'string', 'dfeedTriggers: bad argument #1 type (expected string, got '..type(text)..'!)')
    feedTriggers(decho2ansi(text) .. "\n")
    echo("")
  end

  --- turn hecho style color information into an ANSI color string
  -- IE #800000 for red, #008000 for green, #008000,800000 for green on red background
  -- #r to reset
  --@param text the text convert to ansi colors
  --@see hecho
  --@see hinsertText
  function hecho2ansi(text)
    assert(type(text) == 'string', 'hecho2ansi: bad argument #1 type (expected string, got '..type(text)..'!)')
    local colorPattern = _Echos.Patterns.Hex[1]
    local result = ""
    for str, color, res in rex.split(text, colorPattern) do
      result = result .. str
      if color then
        if color:sub(1,1) == "|" then color = color:gsub("|c", "#") end
        result = result .. hexToAnsi(color:sub(2,-1))
      end
      if res then
        result = result .. resets[res]
      end
    end
    return result
  end

  --- feedTriggers with hecho style color information.
  -- IE #800000 for red, #008000 for green, #008000,800000 for green on red background
  -- #r to reset
  --@param text the text to pump into feedTriggers
  --@see hecho
  --@see hinsertText
  function hfeedTriggers(text)
    assert(type(text) == 'string', 'hfeedTriggers: bad argument #1 type (expected string, got '..type(text)..'!)')

    feedTriggers(hecho2ansi(text) .. "\n")
    echo("")
  end

else
  -- NOT using rex module:

  -- NOT LUADOC
  -- See xEcho/another cecho for description.
  function cecho(window, text)
    assert(type(window) == 'string', 'cecho: bad argument #1 type (expected string, got '..type(window)..'!)')
    local win = text and window
    local s = text or window
    if win == "main" then
      win = nil
    end

    if win then
      resetFormat(win)
    else
      resetFormat()
    end
    for color, text in string.gmatch("<white>" .. s, "<([a-z_0-9, :]+)>([^<>]+)") do
      local colist = string.split(color .. ":", "%s*:%s*")
      local fgcol = colist[1] ~= "" and colist[1] or "white"
      local bgcol = colist[2] ~= "" and colist[2] or "black"
      local FGrgb = color_table[fgcol] or string.split(fgcol, ",")
      local BGrgb = color_table[bgcol] or string.split(bgcol, ",")
      local alpha = BGrgb[4] or 255

      if win then
        setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
        setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3], alpha)
        echo(win, text)
      else
        setFgColor(FGrgb[1], FGrgb[2], FGrgb[3])
        setBgColor(BGrgb[1], BGrgb[2], BGrgb[3], alpha)
        echo(text)
      end
    end

    if win then
      resetFormat(win)
    else
      resetFormat()
    end
  end


  -- NOT LUADOC
  -- See xEcho/another decho for description.
  function decho(window, text)
    assert(type(window) == 'string', 'decho: bad argument #1 type (expected string, got '..type(window)..'!)')
    local win = text and window
    local s = text or window
    if win == "main" then
      win = nil
    end
    local reset
    if win then
      reset = function()
        resetFormat(win)
      end
    else
      reset = function()
        resetFormat()
      end
    end
    reset()
    for color, text in s:gmatch("<([0-9,:]+)>([^<>]+)") do
      if color == "reset" then
        reset()
        if win then
          echo(win, text) else echo(text)
        end
      else
        local colist = string.split(color .. ":", "%s*:%s*")
        local fgcol = colist[1] ~= "" and colist[1] or "white"
        local bgcol = colist[2] ~= "" and colist[2] or "black"
        local FGrgb = color_table[fgcol] or string.split(fgcol, ",")
        local BGrgb = color_table[bgcol] or string.split(bgcol, ",")
        local alpha = BGrgb[4] or 255
        if win then
          setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
          setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3], alpha)
          echo(win, text)
        else
          setFgColor(FGrgb[1], FGrgb[2], FGrgb[3])
          setBgColor(BGrgb[1], BGrgb[2], BGrgb[3], alpha)
          echo(text)
        end
      end
    end
    reset()
  end


end

-- improve replace to have a third argument, keepcolor
do
  local oldreplace = replace
  function replace(arg1, arg2, arg3)
    local windowname, text, keepcolor

    if arg1 and arg2 and arg3 ~= nil then
      windowname, text, keepcolor = arg1, arg2, arg3
    elseif arg1 and type(arg2) == "string" then
      windowname, text = arg1, arg2
    elseif arg1 and type(arg2) == "boolean" then
      text, keepcolor = arg1, arg2
    else
      text = arg1
    end

    local selection = {getSelection()}
    if _comp(selection, {"", 0, 0}) then
      return nil, "replace: nothing is selected to be replaced. Did selectString return -1?"
    end
    text = text or ""

    if keepcolor then
      if not windowname then
        setBgColor(getBgColor())
        setFgColor(getFgColor())
      else
        setBgColor(windowname, getBgColor(windowname))
        setFgColor(windowname, getFgColor(windowname))
      end
    end

    if windowname then
      oldreplace(windowname, text)
    else
      oldreplace(text)
    end
  end
end


-- function for converting a color formatted string to 'plaintext' string
local function x2string(text, style)
  local ttbl = _Echos.Process(text, style)
  local result = ""
  for _, val in ipairs(ttbl) do
    if type(val) == "string" and not val:starts("\27") then
      result = result .. val
    end
  end
  return result
end

-- function to convert a cecho formatted string to a nonformatted string
function cecho2string(text)
  assert(type(text) == 'string', 'cecho2string: bad argument #1 type (expected string, got '..type(text)..'!)')
  return x2string(text, "Color")
end

-- function to convert a decho formatted string to a nonformatted string
function decho2string(text)
  assert(type(text) == 'string', 'decho2string: bad argument #1 type (expected string, got '..type(text)..'!)')
  return x2string(text, "Decimal")
end

-- function to convert a hecho formatted string to a nonformatted string
function hecho2string(text)
  assert(type(text) == 'string', 'hecho2string: bad argument #1 type (expected string, got '..type(text)..'!)')
  return x2string(text, "Hex")
end

local ansiPattern = rex.new("\\e\\[([0-9:;]*?)m")

-- function for converting a raw ANSI string into plain strings
function ansi2string(text)
  assert(type(text) == 'string', 'ansi2string: bad argument #1 type (expected string, got '..type(text)..'!)')
  local result = rex.gsub(text, ansiPattern, "")
  return result
end

-- function for converting a raw ANSI string into something decho can process
-- italics and underline not currently supported since decho doesn't support them
-- bold is emulated so it is supported, up to an extent
function ansi2decho(text, ansi_default_color)
  assert(type(text) == 'string', 'ansi2decho: bad argument #1 type (expected string, got '..type(text)..'!)')
  local lastColour = ansi_default_color
  local coloursToUse = nil

  -- match each set of ansi tags, ie [0;36;40m and convert to decho equivalent.
  -- this works since both ansi colours and echo don't need closing tags and map to each other
  local result = rex.gsub(text, ansiPattern, function(s)
    local output = {} -- assemble the output into this table

    local delim = ";"
    if s:find(":") then delim = ":" end
    local t = string.split(s, delim) -- split the codes into an indexed table

    -- given an xterm256 index, returns an rgb string for decho use
    local function convertindex(tag)
      local ansi = string.format("ansi_%03d", tag)
      return color_table[ansi] or false
    end
    local colours = {}
    for i = 0, 7 do
      colours[i] = convertindex(i)
    end
    local lightColours = {}
    for i = 0, 7 do
      lightColours[i] = convertindex(i+8)
    end
    coloursToUse = coloursToUse or colours

    -- since fg/bg can come in different order and we need them as fg:bg for decho, collect
    -- the data first, then assemble it in the order we need at the end
    local fg, bg
    local i = 1
    local floor = math.floor

    while i <= #t do
      local code = t[i]
      local formatCodeHandled = false

      if code == '0' or code == '00' or code == '' then
        -- reset attributes
        output[#output + 1] = "<r>"
        fg, bg = nil, nil
        coloursToUse = colours
        lastColour = ansi_default_color
      elseif code == "1" then
        -- light or bold
        coloursToUse = lightColours
      elseif code == "22" then
        -- not light or bold
        coloursToUse = colours
      elseif code == "3" then
        formatCodeHandled = true
        output[#output+1] = "<i>"
      elseif code == "23" then
        -- turn off italics
        formatCodeHandled = true
        output[#output+1] = "</i>"
      elseif code == "4" then
        -- underline
        formatCodeHandled = true
        output[#output+1] = "<u>"
      elseif code == "24" then
        -- turn off underline
        formatCodeHandled = true
        output[#output+1] = "</u>"
      elseif code == "9" then
        -- strikethrough
        formatCodeHandled = true
        output[#output+1] = "<s>"
      elseif code == "29" then
        -- turn off strikethrough
        formatCodeHandled = true
        output[#output+1] = "</s>"
      elseif code == "53" then
        -- turn on overline
        formatCodeHandled = true
        output[#output+1] = "<o>"
      elseif code == "55" then
        -- turn off overline
        formatCodeHandled = true
        output[#output+1] = "</o>"
      else
        formatCodeHandled = true
        local layerCode = floor(code / 10)  -- extract the "layer": 3 is fore
        --                      4 is back
        local cmd = code - (layerCode * 10) -- extract the actual "command"
        -- 0-7 is a colour, 8 is xterm256
        local colour = nil

        if cmd == 8 and t[i + 1] == '5' then
          -- xterm256, colour indexed
          colour = convertindex(tonumber(t[i + 2]))
          i = i + 2

        elseif cmd == 8 and t[i + 1] == '2' then
          -- xterm256, rgb
          if delim == ";" then
            colour = { t[i + 2] or '0', t[i + 3] or '0', t[i + 4] or '0' }
            i = i + 4
          elseif delim == ":" then
            colour = { t[i + 3] or '0', t[i + 4] or '0', t[i + 5] or '0' }
            i = i + 5
          end
        elseif layerCode == 9 or layerCode == 10 then
          --light colours
          colour = lightColours[cmd]
        elseif layerCode == 4 then
          -- background colours know no "bright" for
          colour = colours[cmd]  -- mudlet
        else -- usual ANSI colour index
          colour = coloursToUse[cmd]
        end

        if layerCode == 3 or layerCode == 9 then
          fg = colour
          lastColour = cmd
        elseif layerCode == 4 or layerCode == 10 then
          bg = colour
        end
      end

      -- If formatCodeHandled is false it means that we've encountered a SGBR
      -- code such as 'bold' or 'dim'.
      -- In those cases, if there's a previous color, we are supposed to
      -- modify it
      if not formatCodeHandled and lastColour then
        fg = coloursToUse[lastColour]
      end

      i = i + 1
    end

    -- assemble and return the data
    if fg or bg then
      output[#output + 1] = '<'

      if fg then
        output[#output + 1] = table.concat(fg, ",")
      end

      if bg then
        output[#output + 1] = ':'
        output[#output + 1] = table.concat(bg, ",")
      end
      output[#output + 1] = '>'
    end

    return table.concat(output)
  end)

  return result, lastColour
end

--- Form of setFgColor that accepts a hex color string instead of decimal values
--- @param windowName Optional name of the window to use the function on
--- @param colorString hex string for the color to use
function setHexFgColor(windowName, colorString)
  assert(type(windowName) == 'string', 'setHexFgColor: bad argument #1 type (expected string, got '..type(windowName)..'!)')

  local win = colorString and windowName
  local col = colorString or windowName

  if win == "main" then
    win = nil
  end

  if #col ~= 6 then
    error("setHexFgColor needs a 6 digit hex color code.")
  end

  local colTable = {
    r = tonumber(col:sub(1, 2), 16),
    g = tonumber(col:sub(3, 4), 16),
    b = tonumber(col:sub(5, 6), 16)
  }

  if win then
    setFgColor(win, colTable.r, colTable.g, colTable.b)
  else
    setFgColor(colTable.r, colTable.g, colTable.b)
  end
end

--- Form of setBgColor that accepts a hex color string instead of decimal values
--- @param windowName Optional name of the window to use the function on
--- @param colorString hex string for the color to use
function setHexBgColor(windowName, colorString)
  assert(type(windowName) == 'string', 'setHexBgColor: bad argument #1 type (expected string, got '..type(windowName)..'!)')

  local win = colorString and windowName
  local col = colorString or windowName

  if win == "main" then
    win = nil
  end

  if #col ~= 6 then
    error("setHexBgColor needs a 6 digit hex color code.")
  end

  local colTable = {
    r = tonumber(col:sub(1, 2), 16),
    g = tonumber(col:sub(3, 4), 16),
    b = tonumber(col:sub(5, 6), 16)
  }

  if win then
    setBgColor(win, colTable.r, colTable.g, colTable.b)
  else
    setBgColor(colTable.r, colTable.g, colTable.b)
  end
end



local insertFuncs = {[echo] = insertText, [cecho] = cinsertText, [decho] = dinsertText, [hecho] = hinsertText}
--- Suffixes text at the end of the current line when used in a trigger.
---
--- @see prefix
function suffix(what, func, fgc, bgc, window)
  assert(type(what) == 'string', 'suffix: bad argument #1 type (expected string, got '..type(what)..'!)')
  window = window or "main"
  func = insertFuncs[func] or func or insertText
  local length = utf8.len(getCurrentLine(window))
  moveCursor(window, length - 1, getLineNumber(window))
  if fgc then fg(window,fgc) end
  if bgc then bg(window,bgc) end
  func(window,what)
  resetFormat(window)
end



--- Prefixes text at the beginning of the current line when used in a trigger.
---
--- @usage Prefix the hours, minutes and seconds onto our prompt even though Mudlet has a button for that.
---   <pre>
---   prefix(os.date("%H:%M:%S "))
---   </pre>
---
--- @see suffix
function prefix(what, func, fgc, bgc, window)
  assert(type(what) == 'string', 'prefix: bad argument #1 type (expected string, got '..type(what)..'!)')
  window = window or "main"
  func = insertFuncs[func] or func or insertText
  moveCursor(window, 0, getLineNumber(window))
  if fgc then fg(window,fgc) end
  if bgc then bg(window,bgc) end
  func(window,what)
  resetFormat(window)
end

--- Moves the cursor in the given window up a specified number of lines
--- @param windowName Optional name of the window to use the function on
--- @param lines Number of lines to move cursor
--- @param keep_horizontal Optional boolean to specify if horizontal position should be retained
function moveCursorUp(window, lines, keep_horizontal)
  if type(window) ~= "string" then lines, window, keep_horizontal = window, "main", lines end
  lines = tonumber(lines) or 1
  if not type(keep_horizontal) == "boolean" then keep_horizontal = false end
  local curLine = getLineNumber(window)
  if not curLine then return nil, "window does not exist" end
  local x = 0
  if keep_horizontal then x = getColumnNumber(window) end
  moveCursor(window, x, math.max(curLine - lines, 0))
end

--- Moves the cursor in the given window down a specified number of lines
--- @param windowName Optional name of the window to use the function on
--- @param lines Number of lines to move cursor
--- @param keep_horizontal Optional boolean to specify if horizontal position should be retained
function moveCursorDown(window, lines, keep_horizontal)
  if type(window) ~= "string" then lines, window, keep_horizontal = window, "main", lines end
  lines = tonumber(lines) or 1
  if not type(keep_horizontal) == "boolean" then keep_horizontal = false end
  local curLine = getLineNumber(window)
  if not curLine then return nil, "window does not exist" end
  local x = 0
  if keep_horizontal then x = getColumnNumber(window) end
  moveCursor(window, x, math.min(curLine + lines, getLastLineNumber(window)))
end

-- internal function that handles coloured replace variants
function xReplace(window, text, type)
  if not text then
    text = window
    window = "main"
  end
  local str, start, stop = getSelection(window)
	if window ~= "main" then
		replace(window, "")
    moveCursor(window, start, getLineNumber(window))
	else
		replace("")
    moveCursor(start, getLineNumber())
	end
  if type == 'c' then
    cinsertText(window, text)
  elseif type == 'd' then
    dinsertText(window, text)
  elseif type == 'h' then
    hinsertText(window, text)
  else
    insertText(window, text)
  end
end

--- version of replace function that allows for color, by way of cinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function creplace(window, text)
  assert(type(window) == 'string', 'creplace: bad argument #1 type (expected string, got '..type(window)..'!)')
  xReplace(window, text, 'c')
end

--- version of replaceLine function that allows for color, by way of cinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function creplaceLine(window, text)
  assert(type(window) == 'string', 'creplaceLine: bad argument #1 type (expected string, got '..type(window)..'!)')
  if not text then
    selectCurrentLine()
  else
    selectCurrentLine(window)
  end
  creplace(window, text)
end

--- version of replace function that allows for color, by way of dinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function dreplace(window, text)
  assert(type(window) == 'string', 'dreplace: bad argument #1 type (expected string, got '..type(window)..'!)')
  xReplace(window, text, 'd')
end

--- version of replaceLine function that allows for color, by way of dinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function dreplaceLine(window, text)
  assert(type(window) == 'string', 'dreplaceLine: bad argument #1 type (expected string, got '..type(window)..'!)')
  if not text then
    selectCurrentLine()
  else
    selectCurrentLine(window)
  end
  dreplace(window, text)
end

--- version of replace function that allows for color, by way of hinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function hreplace(window, text)
  assert(type(window) == 'string', 'hreplace: bad argument #1 type (expected string, got '..type(window)..'!)')
  xReplace(window, text, 'h')
end

--- version of replaceLine function that allows for color, by way of hinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function hreplaceLine(window, text)
  assert(type(window) == 'string', 'hreplaceLine: bad argument #1 type (expected string, got '..type(window)..'!)')
  if not text then
    selectCurrentLine()
  else
    selectCurrentLine(window)
  end
  hreplace(window, text)
end

function resetLabelToolTip(label)
  return setLabelToolTip(label, "")
end

-- functions to move and resize Map Widget
-- be aware that moving or resizing Map Widget puts the Map Widget in floating state
function moveMapWidget(x, y)
  assert(type(x) == 'number', 'moveMapWidget: bad argument #1 type (x-coordinate as number expected, got '..type(x)..'!)')
  assert(type(y) == 'number', 'moveMapWidget: bad argument #2 type (y-coordinate as number expected, got '..type(y)..'!)')
  openMapWidget(x, y)
end

function resizeMapWidget(width, height)
  assert(type(width) == 'number', 'resizeMapWidget: bad argument #1 type (width as number expected, got '..type(width)..'!)')
  assert(type(height) == 'number', 'resizeMapWidget: bad argument #2 type (height as number expected, got '..type(height)..'!)')
  openMapWidget(-1, -1, width, height)
end

--
-- functions to manipulate room label display and offsets
--
-- get offset of room's label (x,y)
-- @param room Room ID
function getRoomNameOffset(room)
  assert(type(room) == 'number', 'getRoomNameOffset: bad argument #1 type (room ID as number expected, got '..type(room)..'!)')

  local d = getRoomUserData(room, "room.ui_nameOffset")
  if d == nil or d == "" then return 0,0 end
  local split = {}
  for w in string.gfind(d, '[%.%d]+') do split[#split+1] = tonumber(w) end
  if #split == 1 then return 0,split[1] end
  if #split >= 2 then return split[1],split[2] end
  return 0,0
end

-- set offset of room's label (x,y)
-- @param room Room ID
-- @param room X shift (positive = to the right)
-- @param room Y shift (positive = down)
function setRoomNameOffset(room, x, y)
  assert(type(room) == 'number', 'setRoomNameOffset: bad argument #1 type (room ID as number expected, got '..type(room)..'!)')
  assert(type(x) == 'number', 'setRoomNameOffset: bad argument #2 type (X shift as number expected, got '..type(x)..'!)')
  assert(type(y) == 'number', 'setRoomNameOffset: bad argument #3 type (y shift as number expected, got '..type(y)..'!)')

  if x == 0 then
    setRoomUserData(room, "room.ui_nameOffset", y)
  else
    setRoomUserData(room, "room.ui_nameOffset", x .. " " .. y)
  end
end

-- show or hide a room's name
-- @param room Room ID
-- @param flag (bool)
function setRoomNameVisible(room, flag)
  assert(type(room) == 'number', 'setRoomNameVisible: bad argument #1 type (room ID as number expected, got '..type(room)..'!)')
  assert(type(flag) == 'boolean', 'setRoomNameVisible: bad argument #2 type (flag as boolean expected, got '..type(flag)..'!)')

  setRoomUserData(room, "room.ui_showName", flag and "1" or "0")
end

--wrapper for createButton
-- createButton is deprecated better use createLabel instead
createButton = createLabel

-- Internal function used by copy2html and copy2decho
local function copy2color(name,win,str,inst)
  local line,err = getCurrentLine(win or "main")
  if err ~= nil then
    win, str, inst = "main", win, str
    line,err = getCurrentLine(win)
    if err ~= nil then
      error(err)
    end
  end

  win = win or "main"
  str = str or line
  inst = inst or 1
  if str == "" then
    -- happens when you try to use copy2decho() on an empty line
    return ""
  end
  local start, len = selectString(win, str, inst), utf8.len(str)
  if not start then
    error(name..": string not found",3)
  end
  local style, endspan, result, r, g, b, rb, gb, bb, cr, cg, cb, crb, cgb, cbb, char
  local selectSection, getFgColor, getBgColor = selectSection, getFgColor, getBgColor
  local conversions = {
    ["¦"] = "&brvbar;",
    ["×"] = "&times;",
    ["«"] = "&#171;",
    ["»"] = "&raquo;",
    ["<"] = "&lt;",
    [">"] = "&gt;",
    ['"'] = "&quot;",
    ["'"] = "&#39;",
    ["&"] = "&amp;"
  }
  if name == "copy2html" then
    style = "%s<span style=\'color: rgb(%d,%d,%d);background: rgb(%d,%d,%d);'>%s"
    endspan = "</span>"
  elseif name == "copy2decho" then
    style = "%s<%d,%d,%d:%d,%d,%d>%s"
    endspan = "<r>"
  end
  for index = start + 1, start + len do
    if win ~= "main" then
      selectSection(win, index - 1, 1)
      r,g,b = getFgColor(win)
      rb,gb,bb = getBgColor(win)
    else
      selectSection(index - 1, 1)
      r,g,b = getFgColor()
      rb,gb,bb = getBgColor()
    end

    char = utf8.sub(line, index, index)
    if name == "copy2html" then
      char = conversions[char] or char -- replace HTML entities (if they are in the table)
    end

    if r ~= cr or g ~= cg or b ~= cb or rb ~= crb or gb ~= cgb or bb ~= cbb then
      cr,cg,cb,crb,cgb,cbb = r,g,b,rb,gb,bb
      result = string.format(style, result and (result..endspan) or "", r, g, b, rb, gb, bb, char)
    else
      result = result .. char
    end
  end
  result = result .. endspan
  return result
end

--- copies text with color information in decho format
--- @param win optional, the window to copy from. Defaults to the main window
--- @param str optional, the string to copy. Defaults to copying the entire line
--- @param inst optional, the instance of the string to copy. Defaults to the first instance.
--- @usage to copy matches[2] with color information and echo it to miniconsole "test"
---   <pre>
---   decho("test", copy2decho(matches[2]))
---   </pre>
---
--- @usage to copy the entire line with color information, then echo it to miniconsole "test"
---   <pre>
---   decho("test", copy2decho())
---   </pre>
function copy2decho(win, str, inst)
  return copy2color("copy2decho", win, str, inst)
end

--- copies text with color information in html format, for echoing to a label for instance
--- @param win optional, the window to copy from. Defaults to the main window
--- @param str optional, the string to copy. Defaults to copying the entire line
--- @param inst optional, the instance of the string to copy. Defaults to the first instance.
--- @usage to copy matches[2] with color information and echo it to label "test"
---   <pre>
---   echo("test", copy2html(matches[2]))
---   </pre>
---
--- @usage to copy the entire line with color information, then echo it to label "test"
---   <pre>
---   echo("test", copy2html())
---   </pre>
function copy2html(win, str, inst)
  return copy2color("copy2html", win, str, inst)
end

function resetLabelCursor(name)
  assert(type(name) == 'string', 'resetLabelCursor: bad argument #1 type (name as string expected, got '..type(name)..'!)')
  return setLabelCursor(name, -1)
end

local setLabelCursorLayer = setLabelCursor
function setLabelCursor(labelname, cursorShape)
  if type(cursorShape) == "string" then
    cursorShape = mudlet.cursor[cursorShape]
  end
  return setLabelCursorLayer(labelname, cursorShape)
end

mudlet.BgImageMode ={
  ["border"] = 1,
  ["center"] = 2,
  ["tile"]   = 3,
  ["style"]  = 4,
}

local setConsoleBackgroundImageLayer = setBackgroundImage
function setBackgroundImage(...)
  local mode = arg[arg.n]
  if type(mode) == "string" then
    mode = mudlet.BgImageMode[mode] or mode
  end
  arg[arg.n] = mode
  return setConsoleBackgroundImageLayer(unpack(arg))
end


--These functions ensure backward compatibility for the setActionCallback functions
--unpack function which also returns the nil values
-- the arg_table (arg) saves the number of arguments in n -> arg_table.n (arg.n)
function unpack_w_nil (arg_table, counter)
  counter = counter or 1
  if counter >= arg_table.n then
    return arg_table[counter]
  end
  return arg_table[counter], unpack_w_nil(arg_table, counter + 1)
end

-- This wrapper gives callback functions the possibility to be used like
-- setCallBackFunction (name,function as string,args)
-- it is used by setLabelCallBack functions and setCmdLineAction
local function setActionCallback(callbackFunc, funcName, name, func, ...)
  local nr = arg.n + 1
  arg.n = arg.n + 1
  if type(func) == "string" then
    func = loadstring("return "..func.."(...)")
  end
  assert(type(func) == 'function', string.format('<%s: bad argument #2 type (function expected, got %s!)>', funcName, type(func)))
  if nr > 1 then
    return callbackFunc(name,
    function(event)
      if not event then
        arg.n = nr - 1
      end
      arg[nr] = event
      func(unpack_w_nil(arg))
    end )
  end
  return callbackFunc(name, func)
end

local callBackFunc = {"setLabelClickCallback", "setLabelDoubleClickCallback", "setLabelReleaseCallback", "setLabelMoveCallback", "setLabelWheelCallback", "setLabelOnEnter", "setLabelOnLeave", "setCmdLineAction"}

for i = 1, #callBackFunc do
  local funcName = callBackFunc[i]
  local callBackFunction = _G[funcName]
  _G[funcName] = function(...) return setActionCallback(callBackFunction, funcName, ...) end
end

function resetUserWindowTitle(windowname)
  return setUserWindowTitle(windowname, "")
end

function resetMapWindowTitle()
  return setMapWindowTitle("")
end

--- This function takes in a color and returns the closest color from color_table. The following all return "ansi_001"
--- closestColor({127,0,0})
--- closestColor(127,0,0)
--- closestColor("#7f0000")
--- closestColor("|c7f0000")
--- closestColor("<127,0,0>")
function closestColor(r,g,b)
  local rtype = type(r)
  local rgb
  if rtype == "table" then
    rgb = {}
    local tmp = r
    local err = f"Could not parse {table.concat(tmp, ',')} into RGB coordinates to look for.\n"
    if #tmp ~= 3 then
      return nil, err
    end
    for index,coord in ipairs(tmp) do
      local num = tonumber(coord)
      if not num or num < 0 or num > 255 then
        return nil, err
      end
      rgb[index] = num
    end
  elseif rtype == "string" and not tonumber(r) then
    if color_table[r] then
      return r
    end
    rgb = {Geyser.Color.parse(r)}
    if rgb[1] == nil then
      return nil, f"Could not parse {r} into a set of RGB coordinates to look for.\n"
    end
  elseif rtype == "number" or tonumber(r) then
    local nr = tonumber(r)
    local ng = tonumber(g)
    local nb = tonumber(b)
    if not nr or not ng or not nb or (nr < 0 or nr > 255) or (ng < 0 or ng > 255) or (nb < 0 or nb > 255) then
      return nil, f"Could not parse {r},{g},{b} into a set of RGB coordinates to look for.\n"
    end
    rgb = {nr,ng,nb}
  else
    return nil, f"Could not parse your parameters into RGB coordinates.\n"
  end
  local least_distance = math.huge
  local cname = ""
  for name, color in pairs(color_table) do
    local color_distance = math.sqrt((color[1] - rgb[1]) ^ 2 + (color[2] - rgb[2]) ^ 2 + (color[3] - rgb[3]) ^ 2)
    if color_distance < least_distance then
      least_distance = color_distance
      cname = name
    end
  end
  return cname
end

--- Scrolls the given window up a specified number of lines
--- @param windowName Optional name of the window to use the function on
--- @param lines Number of lines to scroll
function scrollUp(window, lines)
  if type(window) ~= "string" then window, lines = "main", window end
  lines = tonumber(lines) or 1
  local numLines = getLastLineNumber(window)
  if not numLines then return nil, "window does not exist" end
  local curScroll = getScroll(window)
  scrollTo(window, math.max(curScroll - lines, 0))
end

--- Scrolls the given window down a specified number of lines
--- @param windowName Optional name of the window to use the function on
--- @param lines Number of lines to scroll
function scrollDown(window, lines)
  if type(window) ~= "string" then window, lines = "main", window end
  lines = tonumber(lines) or 1
  local numLines = getLastLineNumber(window)
  if not numLines then return nil, "window does not exist" end
  local curScroll = getScroll(window)
  scrollTo(window, math.min(curScroll + lines, numLines))
end

--[[
The following functions are to allow easily and efficiently converting from
one color echo type to another. So from cecho to decho. decho to hecho.
Also includes an html output option to make html logging of c/d/hecho strings
easy.
--]]

-- lookup tables for formatting strings in c/d/hecho formats
-- table keys chosen to match the ones in _Echos.Patterns
local echoOutputs = {
  Color = {
    ["\27reset"] = "<reset>",
    ["\27bold"] = "<b>",
    ["\27boldoff"] = "</b>",
    ["\27italics"] = "<i>",
    ["\27italicsoff"] = "</i>",
    ["\27underline"] = "<u>",
    ["\27underlineoff"] = "</u>",
    ["\27strikethrough"] = "<s>",
    ["\27strikethroughoff"] = "</s>",
    ["\27overline"] = "<o>",
    ["\27overlineoff"] = "</o>",
  },
  Decimal = {
    ["\27reset"] = "<r>",
    ["\27bold"] = "<b>",
    ["\27boldoff"] = "</b>",
    ["\27italics"] = "<i>",
    ["\27italicsoff"] = "</i>",
    ["\27underline"] = "<u>",
    ["\27underlineoff"] = "</u>",
    ["\27strikethrough"] = "<s>",
    ["\27strikethroughoff"] = "</s>",
    ["\27overline"] = "<o>",
    ["\27overlineoff"] = "</o>",
  },
  Hex = {
    ["\27reset"] = "#r",
    ["\27bold"] = "#b",
    ["\27boldoff"] = "#/b",
    ["\27italics"] = "#i",
    ["\27italicsoff"] = "#/i",
    ["\27underline"] = "#u",
    ["\27underlineoff"] = "#/u",
    ["\27strikethrough"] = "#s",
    ["\27strikethroughoff"] = "#/s",
    ["\27overline"] = "#o",
    ["\27overlineoff"] = "#/o",
  }
}

-- make these items local for easier and swifter use
local echoPatterns = _Echos.Patterns
local echoProcess = _Echos.Process

--- internal function responsible for taking the color information
-- returned as part of the table by _Echos.Process and outputting
-- it for a specific Xecho formatting type.
local function processedColorsToEchoString(colorType, colors)
  colorType = colorType:lower()
  local result
  if colorType == "hex" then
    local fg,bg = "", ""
    if colors.fg then
      fg = string.format("%02x%02x%02x", unpack(colors.fg))
    end
    if colors.bg then
      bg = string.format(",%02x%02x%02x", unpack(colors.bg))
    end
    result = string.format("#%s%s", fg, bg)
  elseif colorType == "color" then
    local fg,bg = "",""
    if colors.fg then
      fg = closestColor(colors.fg)
    end
    if colors.bg then
      -- closestColor chokes if you provide an alpha channel for the background
      bg = ":" .. closestColor(colors.bg[1], colors.bg[2], colors.bg[3])
    end
    result = string.format("<%s%s>", fg, bg)
  elseif colorType == "decimal" then
    local fg,bg = "", ""
    if colors.fg then
      fg = string.format("%d,%d,%d", unpack(colors.fg))
    end
    if colors.bg then
      bg = string.format(":%d,%d,%d", colors.bg[1], colors.bg[2], colors.bg[3])
    end
    result = string.format("<%s%s>", fg, bg)
  end
  return result
end

-- internal function that powers the c/d/hecho2c/d/hecho/html functions below
-- @tparam string str the formatted color string to transform
-- @tparam string from the type of color formatting that str uses. What you're converting from. 'Color', 'Hex', or 'Decimal'
-- @tparam string to the type of color formatting to output. 'Color', 'Hex', 'Decimal', or 'html'
-- @tparam table resetFormat optional table of default formatting options to use when outputting as html.
local function echoConverter(str, from, to, resetFormat)
  local strType, fromType, toType, resetType = type(str), type(from), type(to), type(resetFormat)
  local errTemplate = "bad argument #{argNum} type ({argName} as string expected, got {argType})"
  local argNum, argName, argType
  local err = false
  if strType ~= "string" then
    argNum = 1
    argName = "str"
    argType = strType
    err = true
  elseif fromType ~= "string" then
    argNum = 2
    argName = "from"
    argType = fromType
    err = true
  elseif toType ~= "string" then
    argNum = 3
    argName = "to"
    argType = toType
    err = true
  elseif resetFormat and resetType ~= "table" then
    argType = resetType
    errTemplate = "bad argument #4 type (optional resetFormat as table of formatting options expected, got {argType})"
    err = true
  end
  if err then
    printError(f(errTemplate), true, true)
  end
  from = from:title()
  if not echoPatterns[from] then
    local msg = "argument #4 (from) must be a valid echo type. Valid types are: " .. table.concat(table.keys(echoPatterns), ",")
    printError(msg, true, true)
  end
  local processed = echoProcess(str, from)
  if to:lower() == "html" then
    return processedEchoToHTML(processed, resetFormat)
  end
  local outputs = echoOutputs[to]
  if not outputs then
    local msg = "argument #3 (to) must be a valid echo type. Valid types are: " .. table.concat(table.keys(echoOutputs), ",")
    printError(msg, true, true)
  end
  local result = ""
  for _, token in ipairs(processed) do
    local formatter = outputs[token]
    if formatter and token:find("\27") then
      result = result .. formatter
    elseif type(token) == "table" then
      result = result .. processedColorsToEchoString(to, token)
    else
      result = result .. token
    end
  end
  return result
end

-- converts cecho formatted string to html
-- @tparam string str the string you're converting
-- @tparam table resetFormat optional table of default formatting options, as returned by getTextFormat or getLabelFormat
function cecho2html(str, resetFormat)
  assert(type(str) == "string", "cecho2html: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Color", "html", resetFormat)
end

-- converts cecho formatted string to decho
-- @tparam string str the string you're converting
function cecho2decho(str)
  assert(type(str) == "string", "cecho2decho: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Color", "Decimal")
end

-- converts cecho formatted string to hecho
-- @tparam string str the string you're converting
function cecho2hecho(str)
  assert(type(str) == "string", "cecho2hecho: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Color", "Hex")
end

-- converts decho formatted string to hecho
-- @tparam string str the string you're converting
function decho2hecho(str)
  assert(type(str) == "string", "decho2hecho: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Decimal", "Hex")
end

-- converts decho formatted string to cecho
-- @tparam string str the string you're converting
function decho2cecho(str)
  assert(type(str) == "string", "decho2cecho: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Decimal", "Color")
end

-- converts decho formatted string to html
-- @tparam string str the string you're converting
-- @tparam table resetFormat optional table of default formatting options, as returned by getTextFormat or getLabelFormat
function decho2html(str, resetFormat)
  assert(type(str) == "string", "decho2html: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Decimal", "html", resetFormat)
end

-- converts hecho formatted string to decho
-- @tparam string str the string you're converting
function hecho2decho(str)
  assert(type(str) == "string", "hecho2decho: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Hex", "Decimal")
end

-- converts hecho formatted string to cecho
-- @tparam string str the string you're converting
function hecho2cecho(str)
  assert(type(str) == "string", "hecho2cecho: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Hex", "Color")
end

-- converts hecho formatted string to html
-- @tparam string str the string you're converting
-- @tparam table resetFormat optional table of default formatting options, as returned by getTextFormat or getLabelFormat
function hecho2html(str, resetFormat)
  assert(type(str) == "string", "hecho2html: bad argument #1 type (string expected, got " .. type(str) .. ")")
  return echoConverter(str, "Hex", "html", resetFormat)
end
