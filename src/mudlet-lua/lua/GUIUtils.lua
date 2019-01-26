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
color_table = {
  alice_blue                = { 240, 248, 255 },
  AliceBlue                 = { 240, 248, 255 },
  antique_white             = { 250, 235, 215 },
  AntiqueWhite              = { 250, 235, 215 },
  aquamarine                = { 127, 255, 212 },
  azure                     = { 240, 255, 255 },
  beige                     = { 245, 245, 220 },
  bisque                    = { 255, 228, 196 },
  black                     = { 0, 0, 0 },
  blanched_almond           = { 255, 235, 205 },
  BlanchedAlmond            = { 255, 235, 205 },
  blue                      = { 0, 0, 255 },
  blue_violet               = { 138, 43, 226 },
  BlueViolet                = { 138, 43, 226 },
  brown                     = { 165, 42, 42 },
  burlywood                 = { 222, 184, 135 },
  cadet_blue                = { 95, 158, 160 },
  CadetBlue                 = { 95, 158, 160 },
  chartreuse                = { 127, 255, 0 },
  chocolate                 = { 210, 105, 30 },
  coral                     = { 255, 127, 80 },
  cornflower_blue           = { 100, 149, 237 },
  CornflowerBlue            = { 100, 149, 237 },
  cornsilk                  = { 255, 248, 220 },
  cyan                      = { 0, 255, 255 },
  dark_goldenrod            = { 184, 134, 11 },
  DarkGoldenrod             = { 184, 134, 11 },
  dark_green                = { 0, 100, 0 },
  DarkGreen                 = { 0, 100, 0 },
  dark_khaki                = { 189, 183, 107 },
  DarkKhaki                 = { 189, 183, 107 },
  dark_olive_green          = { 85, 107, 47 },
  DarkOliveGreen            = { 85, 107, 47 },
  dark_orange               = { 255, 140, 0 },
  DarkOrange                = { 255, 140, 0 },
  dark_orchid               = { 153, 50, 204 },
  DarkOrchid                = { 153, 50, 204 },
  dark_salmon               = { 233, 150, 122 },
  DarkSalmon                = { 233, 150, 122 },
  dark_slate_blue           = { 72, 61, 139 },
  dark_sea_green            = { 143, 188, 143 },
  DarkSeaGreen              = { 143, 188, 143 },
  DarkSlateBlue             = { 72, 61, 139 },
  dark_slate_gray           = { 47, 79, 79 },
  DarkSlateGray             = { 47, 79, 79 },
  dark_slate_grey           = { 47, 79, 79 },
  DarkSlateGrey             = { 47, 79, 79 },
  dark_turquoise            = { 0, 206, 209 },
  DarkTurquoise             = { 0, 206, 209 },
  dark_violet               = { 148, 0, 211 },
  DarkViolet                = { 148, 0, 211 },
  deep_pink                 = { 255, 20, 147 },
  DeepPink                  = { 255, 20, 147 },
  deep_sky_blue             = { 0, 191, 255 },
  DeepSkyBlue               = { 0, 191, 255 },
  dodger_blue               = { 30, 144, 255 },
  DodgerBlue                = { 30, 144, 255 },
  dim_gray                  = { 105, 105, 105 },
  DimGray                   = { 105, 105, 105 },
  dim_grey                  = { 105, 105, 105 },
  DimGrey                   = { 105, 105, 105 },
  firebrick                 = { 178, 34, 34 },
  floral_white              = { 255, 250, 240 },
  FloralWhite               = { 255, 250, 240 },
  forest_green              = { 34, 139, 34 },
  ForestGreen               = { 34, 139, 34 },
  gainsboro                 = { 220, 220, 220 },
  ghost_white               = { 248, 248, 255 },
  GhostWhite                = { 248, 248, 255 },
  gold                      = { 255, 215, 0 },
  goldenrod                 = { 218, 165, 32 },
  gray                      = { 190, 190, 190 },
  grey                      = { 190, 190, 190 },
  green                     = { 0, 255, 0 },
  green_yellow              = { 173, 255, 47 },
  GreenYellow               = { 173, 255, 47 },
  honeydew                  = { 240, 255, 240 },
  hot_pink                  = { 255, 105, 180 },
  HotPink                   = { 255, 105, 180 },
  indian_red                = { 205, 92, 92 },
  IndianRed                 = { 205, 92, 92 },
  khaki                     = { 240, 230, 140 },
  ivory                     = { 255, 255, 240 },
  lavender                  = { 230, 230, 250 },
  lavender_blush            = { 255, 240, 245 },
  LavenderBlush             = { 255, 240, 245 },
  lawn_green                = { 124, 252, 0 },
  LawnGreen                 = { 124, 252, 0 },
  lemon_chiffon             = { 255, 250, 205 },
  LemonChiffon              = { 255, 250, 205 },
  light_blue                = { 173, 216, 230 },
  LightBlue                 = { 173, 216, 230 },
  light_coral               = { 240, 128, 128 },
  LightCoral                = { 240, 128, 128 },
  light_cyan                = { 224, 255, 255 },
  LightCyan                 = { 224, 255, 255 },
  light_goldenrod           = { 238, 221, 130 },
  LightGoldenrod            = { 238, 221, 130 },
  light_goldenrod_yellow    = { 250, 250, 210 },
  LightGoldenrodYellow      = { 250, 250, 210 },
  light_gray                = { 211, 211, 211 },
  LightGray                 = { 211, 211, 211 },
  light_grey                = { 211, 211, 211 },
  LightGrey                 = { 211, 211, 211 },
  light_pink                = { 255, 182, 193 },
  LightPink                 = { 255, 182, 193 },
  light_salmon              = { 255, 160, 122 },
  LightSalmon               = { 255, 160, 122 },
  light_sea_green           = { 32, 178, 170 },
  LightSeaGreen             = { 32, 178, 170 },
  light_sky_blue            = { 135, 206, 250 },
  LightSkyBlue              = { 135, 206, 250 },
  light_slate_blue          = { 132, 112, 255 },
  LightSlateBlue            = { 132, 112, 255 },
  light_slate_gray          = { 119, 136, 153 },
  LightSlateGray            = { 119, 136, 153 },
  light_slate_grey          = { 119, 136, 153 },
  LightSlateGrey            = { 119, 136, 153 },
  light_steel_blue          = { 176, 196, 222 },
  LightSteelBlue            = { 176, 196, 222 },
  light_yellow              = { 255, 255, 224 },
  LightYellow               = { 255, 255, 224 },
  lime_green                = { 50, 205, 50 },
  LimeGreen                 = { 50, 205, 50 },
  linen                     = { 250, 240, 230 },
  magenta                   = { 255, 0, 255 },
  maroon                    = { 176, 48, 96 },
  medium_aquamarine         = { 102, 205, 170 },
  MediumAquamarine          = { 102, 205, 170 },
  medium_blue               = { 0, 0, 205 },
  MediumBlue                = { 0, 0, 205 },
  medium_orchid             = { 186, 85, 211 },
  MediumOrchid              = { 186, 85, 211 },
  medium_purple             = { 147, 112, 219 },
  MediumPurple              = { 147, 112, 219 },
  medium_sea_green          = { 60, 179, 113 },
  MediumSeaGreen            = { 60, 179, 113 },
  medium_slate_blue         = { 123, 104, 238 },
  MediumSlateBlue           = { 123, 104, 238 },
  medium_spring_green       = { 0, 250, 154 },
  MediumSpringGreen         = { 0, 250, 154 },
  medium_turquoise          = { 72, 209, 204 },
  MediumTurquoise           = { 72, 209, 204 },
  medium_violet_red         = { 199, 21, 133 },
  MediumVioletRed           = { 199, 21, 133 },
  midnight_blue             = { 25, 25, 112 },
  MidnightBlue              = { 25, 25, 112 },
  mint_cream                = { 245, 255, 250 },
  MintCream                 = { 245, 255, 250 },
  misty_rose                = { 255, 228, 225 },
  MistyRose                 = { 255, 228, 225 },
  moccasin                  = { 255, 228, 181 },
  navajo_white              = { 255, 222, 173 },
  NavajoWhite               = { 255, 222, 173 },
  navy                      = { 0, 0, 128 },
  navy_blue                 = { 0, 0, 128 },
  NavyBlue                  = { 0, 0, 128 },
  old_lace                  = { 253, 245, 230 },
  OldLace                   = { 253, 245, 230 },
  olive_drab                = { 107, 142, 35 },
  OliveDrab                 = { 107, 142, 35 },
  orange                    = { 255, 165, 0 },
  orange_red                = { 255, 69, 0 },
  OrangeRed                 = { 255, 69, 0 },
  orchid                    = { 218, 112, 214 },
  pale_goldenrod            = { 238, 232, 170 },
  PaleGoldenrod             = { 238, 232, 170 },
  pale_green                = { 152, 251, 152 },
  PaleGreen                 = { 152, 251, 152 },
  pale_turquoise            = { 175, 238, 238 },
  PaleTurquoise             = { 175, 238, 238 },
  pale_violet_red           = { 219, 112, 147 },
  PaleVioletRed             = { 219, 112, 147 },
  papaya_whip               = { 255, 239, 213 },
  PapayaWhip                = { 255, 239, 213 },
  peach_puff                = { 255, 218, 185 },
  PeachPuff                 = { 255, 218, 185 },
  peru                      = { 205, 133, 63 },
  pink                      = { 255, 192, 203 },
  plum                      = { 221, 160, 221 },
  powder_blue               = { 176, 224, 230 },
  PowderBlue                = { 176, 224, 230 },
  purple                    = { 160, 32, 240 },
  royal_blue                = { 65, 105, 225 },
  RoyalBlue                 = { 65, 105, 225 },
  red                       = { 255, 0, 0 },
  rosy_brown                = { 188, 143, 143 },
  RosyBrown                 = { 188, 143, 143 },
  saddle_brown              = { 139, 69, 19 },
  SaddleBrown               = { 139, 69, 19 },
  salmon                    = { 250, 128, 114 },
  sandy_brown               = { 244, 164, 96 },
  SandyBrown                = { 244, 164, 96 },
  sea_green                 = { 46, 139, 87 },
  SeaGreen                  = { 46, 139, 87 },
  seashell                  = { 255, 245, 238 },
  sienna                    = { 160, 82, 45 },
  sky_blue                  = { 135, 206, 235 },
  SkyBlue                   = { 135, 206, 235 },
  slate_blue                = { 106, 90, 205 },
  SlateBlue                 = { 106, 90, 205 },
  slate_gray                = { 112, 128, 144 },
  SlateGray                 = { 112, 128, 144 },
  slate_grey                = { 112, 128, 144 },
  SlateGrey                 = { 112, 128, 144 },
  snow                      = { 255, 250, 250 },
  steel_blue                = { 70, 130, 180 },
  SteelBlue                 = { 70, 130, 180 },
  spring_green              = { 0, 255, 127 },
  SpringGreen               = { 0, 255, 127 },
  tan                       = { 210, 180, 140 },
  thistle                   = { 216, 191, 216 },
  tomato                    = { 255, 99, 71 },
  turquoise                 = { 64, 224, 208 },
  violet_red                = { 208, 32, 144 },
  VioletRed                 = { 208, 32, 144 },
  violet                    = { 238, 130, 238 },
  wheat                     = { 245, 222, 179 },
  white                     = { 255, 255, 255 },
  white_smoke               = { 245, 245, 245 },
  WhiteSmoke                = { 245, 245, 245 },
  yellow                    = { 255, 255, 0 },
  yellow_green              = { 154, 205, 50 },
  YellowGreen               = { 154, 205, 50 }
}


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
function createGauge(gaugeName, width, height, x, y, gaugeText, r, g, b, orientation)
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
  createLabel(gaugeName .. "_back", 0, 0, 0, 0, 1)
  setBackgroundColor(gaugeName .. "_back", r, g, b, 100)

  createLabel(gaugeName .. "_front", 0, 0, 0, 0, 1)
  setBackgroundColor(gaugeName .. "_front", r, g, b, 255)

  createLabel(gaugeName .. "_text", 0, 0, 0, 0, 1)
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
function createConsole(consoleName, fontSize, charsPerLine, numberOfLines, Xpos, Ypos)
  createMiniConsole(consoleName, 0, 0, 1, 1)
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
function replaceAll(word, what)
  local getCurrentLine, selectSection, replace = getCurrentLine, selectSection, replace
  local startp, endp = 1, 1
  while true do
    startp, endp = getCurrentLine():find(word, endp)
    if not startp then
      break
    end
    selectSection(startp - 1, endp - startp + 1)
    replace(what)
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
function replaceWildcard(what, replacement)
  if replacement == nil or what == nil then
    return
  end
  selectCaptureGroup(what)
  replace(replacement)
end



--- Prints out a formatted list of all available named colors, optional arg specifies number of columns to print in, defaults to 4
---
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
local function calc_lumosity(r,g,b)
  r = r < 11 and r / (255 * 12.92) or ((0.055 + r / 255) / 1.055) ^ 2.4
  g = g < 11 and g / (255 * 12.92) or ((0.055 + g / 255) / 1.055) ^ 2.4
  b = b < 11 and b / (255 * 12.92) or ((0.055 + b / 255) / 1.055) ^ 2.4
  return (0.2126 * r) + (0.7152 * g) + (0.0722 * b)
end

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
    table.insert(colors,k)
  end
  if sort then table.sort(colors) end

  local i = 1
  for _, k in ipairs(colors) do
    if k:lower():find(search) then
      local v = color_table[k]
      local fgc = "white"
      if calc_lumosity(v[1],v[2],v[3]) > 0.5 then
        fgc = "black"
      end
      cechoLink(string.format('<%s:%s>%-23s<reset>  ',fgc,k,k), [[printCmdLine("]] .. k .. [[")]], table.concat(v, ", "), true)
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
        [[(\x5c?\|c[0-9a-fA-F]{6}?(?:,[0-9a-fA-F]{6})?)|(\|r)]],
        rex.new [[\|c(?:([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?(?:,([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?]],
      },
      Decimal = {
        [[(<[0-9,:]+>)|(<r>)]],
        rex.new [[<(?:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?(?::(?=>))?(?::([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?>]],
      },
      Color = {
        [[(<[a-zA-Z_,:]+>)]],
        rex.new [[<([a-zA-Z_]+)?(?:[:,](?=>))?(?:[:,]([a-zA-Z_]+))?>]],
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

    if func == 'echoLink' then
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


  -- Backwards compatibility
  checho = cecho


else


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
  
  -- version of replace function that allows for color, by way of cinsertText
function creplace(window, text)
	if not text then text, window = window, nil end
	window = window or "main"
	local str, start, stop = getSelection(window)
	if window ~= "main" then
		replace(window, "")
	else
		replace("")
	end
	moveCursor(window, start, getLineNumber(window))
	cinsertText(window, text)
end
