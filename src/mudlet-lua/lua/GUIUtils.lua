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
  assert(x and y, "moveGauge: need to have both X and Y dimensions.")
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
    windowname= nil
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
  assert(currentValue and maxValue, "setGauge: need to have both current and max values.")
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
function createConsole(windowname, consoleName, fontSize, charsPerLine, numberOfLines, Xpos, Ypos)
  if Ypos == nil then
    Ypos = Xpos
    Xpos = numberOfLines
    numberOfLines = charsPerLine
    charsPerLine = fontSize
    fontSize = consoleName
    consoleName = windowname
    windowname = "main"
  end
  createMiniConsole(windowname, consoleName, 0, 0, 1, 1)
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



--- Replace the whole with a string you'd like.
---
--- @see deleteLine
function replaceLine(what)
  selectString(line, 1)
  replace("")
  insertText(what)
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
  if not color_table[colorName] then
    error(string.format("bg: '%s' color doesn't exist - see showColors()", colorName))
  end

  if console == colorName or console == "main" then
    setBgColor(color_table[colorName][1], color_table[colorName][2], color_table[colorName][3])
  else
    setBgColor(console, color_table[colorName][1], color_table[colorName][2], color_table[colorName][3])
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
  assert(width and height, "resizeGauge: need to have both width and height.")
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
  setLabelStyleSheet(gaugeName .. "_back", cssback or css)
  setLabelStyleSheet(gaugeName .. "_front", css)
  setLabelStyleSheet(gaugeName .. "_text", csstext or "")
end



if rex then
  _Echos = {
    Patterns = {
      Hex = {
        [[(\x5c?(?:#|\|c)(?:[0-9a-fA-F]{6})?(?:,[0-9a-fA-F]{6})?)|(\|r|#r)]],
        rex.new [[(?:#|\|c)(?:([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?(?:,([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?]],
      },
      Decimal = {
        [[(<[0-9,:]+>)|(<r>)]],
        rex.new [[<(?:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?(?::(?=>))?(?::([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?>]],
      },
      Color = {
        [[(<[a-zA-Z0-9_,:]+>)]],
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
        if r then
          t[#t + 1] = "\27reset"
        end
        if c then
          if style == 'Hex' or style == 'Decimal' then
            local fr, fg, fb, br, bg, bb = _Echos.Patterns[style][2]:match(c)
            local color = {}
            if style == 'Hex' then
              if fr and fg and fb then
                fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16)
              end
              if br and bg and bb then
                br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16)
              end
            end
            if fr and fg and fb then
              color.fg = { fr, fg, fb }
            end
            if br and bg and bb then
              color.bg = { br, bg, bb }
            end

            -- if the colour failed to match anything, then what we captured in <> wasn't a colour -
            -- pass it into the text stream then
            t[#t + 1] = ((fr or br) and color or c)
          elseif style == 'Color' then
            if c == "<reset>" then
              t[#t + 1] = "\27reset"
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
    local out, reset
    local args = { ... }
    local n = #args
    
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
    
    local t = _Echos.Process(str, style)
    
    deselect(win)
    resetFormat(win)
    if not str then error(style:sub(1,1):lower() .. func .. ": bad argument #1, string expected, got nil",3) end
    for _, v in ipairs(t) do
      if type(v) == 'table' then
        if v.fg then
          local fr, fg, fb = unpack(v.fg)
          setFgColor(win, fr, fg, fb)
        end
        if v.bg then
          local br, bg, bb = unpack(v.bg)
          setBgColor(win, br, bg, bb)
        end
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

  -- take a color name and turn it into an ANSI escape string
  local function colorToAnsi(colorName)
    local result = ""
    local cols = colorName:split(":")
    local fore = cols[1]
    local back = cols[2]
    if fore ~= "" then
      if fore == "r" or fore == "reset" then
        result = result .. "\27[39;49m"
      else
        local colorNumber = ctable[fore]
        if colorNumber then
          result = string.format("%s\27[38:5:%sm", result, colorNumber)
        end
      end
    end
    if back then
      local colorNumber = ctable[back]
      result = string.format("%s\27[48:5:%sm", result, colorNumber)
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

  --- feedTriggers with cecho style color information.
  -- Valid colors are  black,red,green,yellow,blue,magenta,cyan,white and light_* versions of same
  -- Can also pass in a number between 0 and 255 to use the expanded ansi 255 colors. IE <124> will set foreground to the color ANSI124
  -- Will also take ansi colors as ansi_#, IE <ansi_124>
  -- Reset using <r> or <reset>
  --@param text the text to pump into feedTriggers
  --@see cecho
  --@see cinsertText
  function cfeedTriggers(text)
    local colorPattern = _Echos.Patterns.Color[1]
    local result = ""
    for str, color in rex.split(text, colorPattern) do
      result = result .. str
      if color then
        result = result .. colorToAnsi(color:match("<(.+)>"))
      end
    end
    feedTriggers(result .. "\n")
    echo("")
  end

  --- Returns a string with decho style color codes converted to ANSI color
  -- IE <128,0,0> for red, <0,128,0> for green, <0,128,0:128,0,0> for green on red background.
  -- <r> to reset
  --@param text the text to convert to ansi colors
  --@see decho
  --@see dinsertText
  function decho2ansi(text)
    local colorPattern = _Echos.Patterns.Decimal[1]
    local result = ""
    for str, color, res in rex.split(text, colorPattern) do
      result = result .. str
      if color then
        result = result .. rgbToAnsi(color:match("<(.+)>"))
      end
      if res then
        result = result .. "\27[39;49m"
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
    local colorPattern = _Echos.Patterns.Hex[1]
    local result = ""
    for str, color, res in rex.split(text, colorPattern) do
      result = result .. str
      if color then
        if color:sub(1,1) == "|" then color = color:gsub("|c", "#") end
        result = result .. hexToAnsi(color:sub(2,-1))
      end
      if res then
        result = result .. "\27[39;49m"
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
    feedTriggers(hecho2ansi(text) .. "\n")
    echo("")
  end

else
  -- NOT using rex module:

  -- NOT LUADOC
  -- See xEcho/another cecho for description.
  function cecho(window, text)
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

      if win then
        setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
        setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3])
        echo(win, text)
      else
        setFgColor(FGrgb[1], FGrgb[2], FGrgb[3])
        setBgColor(BGrgb[1], BGrgb[2], BGrgb[3])
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

        if win then
          setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
          setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3])
          echo(win, text)
        else
          setFgColor(FGrgb[1], FGrgb[2], FGrgb[3])
          setBgColor(BGrgb[1], BGrgb[2], BGrgb[3])
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


local colours = {
  [0] = { 0, 0, 0 }, -- black
  [1] = { 128, 0, 0 }, -- red
  [2] = { 0, 179, 0 }, -- green
  [3] = { 128, 128, 0 }, -- yellow
  [4] = { 0, 0, 128 }, --blue
  [5] = { 128, 0, 128 }, -- magenta
  [6] = { 0, 128, 128 }, -- cyan
  [7] = { 192, 192, 192 }, -- white
}

local lightColours = {
  [0] = { 128, 128, 128 }, -- black
  [1] = { 255, 0, 0 }, -- red
  [2] = { 0, 255, 0 }, -- green
  [3] = { 255, 255, 0 }, -- yellow
  [4] = { 0, 0, 255 }, --blue
  [5] = { 255, 0, 255 }, -- magenta
  [6] = { 0, 255, 255 }, -- cyan
  [7] = { 255, 255, 255 }, -- white
}

-- black + 23 tone grayscale up to white
-- The values are to be used for each of te r, g and b values
local grayscaleComponents = {
  [0] = 0,
  [1] = 11,
  [2] = 22,
  [3] = 33,
  [4] = 44,
  [5] = 55,
  [6] = 67,
  [7] = 78,
  [8] = 89,
  [9] = 100,
  [10] = 111,
  [11] = 122,
  [12] = 133,
  [13] = 144,
  [14] = 155,
  [15] = 166,
  [16] = 177,
  [17] = 188,
  [18] = 200,
  [19] = 211,
  [20] = 222,
  [21] = 233,
  [22] = 244,
  [23] = 255
}

local ansiPattern = rex.new("\\e\\[([0-9;]+?)m")
-- function for converting a raw ANSI string into something decho can process
-- italics and underline not currently supported since decho doesn't support them
-- bold is emulated so it is supported, up to an extent
function ansi2decho(text, ansi_default_color)
  assert(type(text) == 'string', 'ansi2decho: bad argument #1 type (expected string, got '..type(text)..'!)')
  local coloursToUse = colours
  local lastColour = ansi_default_color

  -- match each set of ansi tags, ie [0;36;40m and convert to decho equivalent.
  -- this works since both ansi colours and echo don't need closing tags and map to each other
  local result = rex.gsub(text, ansiPattern, function(s)
    local output = {} -- assemble the output into this table

    local t = string.split(s, ";") -- split the codes into an indexed table

    -- given an xterm256 index, returns an rgb string for decho use
    local function convertindex(tag)
      local floor = math.floor
      -- code from Mudlets own decoding in TBuffer::translateToPlainText

      local rgb

      if tag < 8 then
        rgb = colours[tag]
      elseif tag < 16 then
        rgb = lightColours[tag - 8]
      elseif tag < 232 then
        tag = tag - 16 -- because color 1-15 behave like normal ANSI colors

        r = floor(tag / 36)
        g = floor((tag - (r * 36)) / 6)
        b = floor((tag - (r * 36)) - (g * 6))
        rgb = { r * 51, g * 51, b * 51 }
      else
        local component = grayscaleComponents[tag - 232]
        rgb = { component, component, component }
      end

      return rgb
    end

    -- since fg/bg can come in different order and we need them as fg:bg for decho, collect
    -- the data first, then assemble it in the order we need at the end
    local fg, bg
    local i = 1
    local floor = math.floor

    while i <= #t do
      local code = t[i]
      local isColorCode = false

      if code == '0' or code == '00' then
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
      else
        isColorCode = true

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
          colour = { t[i + 2] or '0', t[i + 3] or '0', t[i + 4] or '0' }
          i = i + 4
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

      -- If isColorCode is false it means that we've encountered a SGBR
      -- code such as 'bold' or 'dim'.
      -- In those cases, if there's a previous color, we are supposed to
      -- modify it
      if not isColorCode and lastColour then
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

      output[#output + 1] = ':'

      if bg then
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
  xReplace(window, text, 'c')
end

--- version of replace function that allows for color, by way of dinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function dreplace(window, text)
  xReplace(window, text, 'd')
end

--- version of replace function that allows for color, by way of hinsertText
--- @param windowName Optional name of the window to replace on
--- @param text The text to replace the selection with.
function hreplace(window, text)
  xReplace(window, text, 'h')
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

--wrapper for createButton 
-- createButton is deprecated better use createLabel instead
createButton = createLabel

-- Internal function used by copy2html and copy2decho
local function copy2color(name,win,str,inst)
  local line = getCurrentLine(win or "main")
  if (not str and line == "ERROR: mini console does not exist") or type(str) == "number" then
    win, str, inst = "main", win, str
    line = getCurrentLine(win)
  end
  win = win or "main"
  str = str or line
  inst = inst or 1
  local start, len = selectString(win, str, inst), #str
  if not start then
    error(name..": string not found",3)
  end
  local style, endspan, result, r, g, b, rb, gb, bb, cr, cg, cb, crb, cgb, cbb
  local selectSection, getFgColor, getBgColor = selectSection, getFgColor, getBgColor
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
    
    if r ~= cr or g ~= cg or b ~= cb or rb ~= crb or gb ~= cgb or bb ~= cbb then
      cr,cg,cb,crb,cgb,cbb = r,g,b,rb,gb,bb
      result = string.format(style, result and (result..endspan) or "", r, g, b, rb, gb, bb, line:sub(index, index))
    else
      result = result .. line:sub(index, index)
    end
  end
  result = result .. endspan
  if name == "copy2html" then
    local conversions = {["¦"] = "&brvbar;", ["×"] = "&times;", ["«"] = "&#171;", ["»"] = "&raquo;"}
    for from, to in pairs(conversions) do
      result = string.gsub(result, from, to)
    end
  end
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
local function setActionCallback(callbackFunc, name, func, ...)
  local nr = arg.n + 1
  arg.n = arg.n + 1
  if type(func) == "string" then
    func = loadstring("return "..func.."(...)")
  end
  assert(type(func) == 'function', '<setActionCallback: bad argument #2 type (function expected, got '..type(func)..'!)>')
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
  callbackFunc(name, func) 
end

local setLC = setLC or setLabelClickCallback
function setLabelClickCallback (...)
  setActionCallback(setLC, ...)
end

local setLDC = setLDC or setLabelDoubleClickCallback
function setLabelDoubleClickCallback (...)
  setActionCallback(setLDC, ...)
end

local setLRC = setLRC or setLabelReleaseCallback
function setLabelReleaseCallback(...)
  setActionCallback(setLRC, ...)
end

local setLMC = setLMC or setLabelMoveCallback
function setLabelMoveCallback(...)
  setActionCallback(setLMC, ...)
end

local setLWC = setLWC or setLabelWheelCallback
function setLabelWheelCallback(...)
  setActionCallback(setLWC, ...)
end

local setOnE = setOnE or setLabelOnEnter
function setLabelOnEnter(...)
  setActionCallback(setOnE, ...)
end

local setOnL = setOnL or setLabelOnLeave
function setLabelOnLeave(...)
  setActionCallback(setOnL,...)
end

local setCmdLA = setCmdLA or setCmdLineAction
function setCmdLineAction(...)
  setActionCallback(setCmdLA,...)
end

function resetUserWindowTitle(windowname)
  return setUserWindowTitle(windowname, "")
end

function resetMapWindowTitle()
  return setMapWindowTitle("")
end
