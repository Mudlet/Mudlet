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
--- used in conjunction with fg() and bg() colorizer functions.
---
--- @see showColors
--- @see bg
--- @see fg
---
--- @class function
--- @name color_table
color_table = {
	snow                  = {255, 250, 250},
	ghost_white           = {248, 248, 255},
	GhostWhite            = {248, 248, 255},
	white_smoke           = {245, 245, 245},
	WhiteSmoke            = {245, 245, 245},
	gainsboro             = {220, 220, 220},
	floral_white          = {255, 250, 240},
	FloralWhite           = {255, 250, 240},
	old_lace              = {253, 245, 230},
	OldLace               = {253, 245, 230},
	linen                 = {250, 240, 230},
	antique_white         = {250, 235, 215},
	AntiqueWhite          = {250, 235, 215},
	papaya_whip           = {255, 239, 213},
	PapayaWhip            = {255, 239, 213},
	blanched_almond       = {255, 235, 205},
	BlanchedAlmond        = {255, 235, 205},
	bisque                = {255, 228, 196},
	peach_puff            = {255, 218, 185},
	PeachPuff             = {255, 218, 185},
	navajo_white          = {255, 222, 173},
	NavajoWhite           = {255, 222, 173},
	moccasin              = {255, 228, 181},
	cornsilk              = {255, 248, 220},
	ivory                 = {255, 255, 240},
	lemon_chiffon         = {255, 250, 205},
	LemonChiffon          = {255, 250, 205},
	seashell              = {255, 245, 238},
	honeydew              = {240, 255, 240},
	mint_cream            = {245, 255, 250},
	MintCream             = {245, 255, 250},
	azure                 = {240, 255, 255},
	alice_blue            = {240, 248, 255},
	AliceBlue             = {240, 248, 255},
	lavender              = {230, 230, 250},
	lavender_blush        = {255, 240, 245},
	LavenderBlush         = {255, 240, 245},
	misty_rose            = {255, 228, 225},
	MistyRose             = {255, 228, 225},
	white                 = {255, 255, 255},
	black                 = {0, 0, 0},
	dark_slate_gray       = {47, 79, 79},
	DarkSlateGray         = {47, 79, 79},
	dark_slate_grey       = {47, 79, 79},
	DarkSlateGrey         = {47, 79, 79},
	dim_gray              = {105, 105, 105},
	DimGray               = {105, 105, 105},
	dim_grey              = {105, 105, 105},
	DimGrey               = {105, 105, 105},
	slate_gray            = {112, 128, 144},
	SlateGray             = {112, 128, 144},
	slate_grey            = {112, 128, 144},
	SlateGrey             = {112, 128, 144},
	light_slate_gray      = {119, 136, 153},
	LightSlateGray        = {119, 136, 153},
	light_slate_grey      = {119, 136, 153},
	LightSlateGrey        = {119, 136, 153},
	gray                  = {190, 190, 190},
	grey                  = {190, 190, 190},
	light_grey            = {211, 211, 211},
	LightGrey             = {211, 211, 211},
	light_gray            = {211, 211, 211},
	LightGray             = {211, 211, 211},
	midnight_blue         = {25, 25, 112},
	MidnightBlue          = {25, 25, 112},
	navy                  = {0, 0, 128},
	navy_blue             = {0, 0, 128},
	NavyBlue              = {0, 0, 128},
	cornflower_blue       = {100, 149, 237},
	CornflowerBlue        = {100, 149, 237},
	dark_slate_blue       = {72, 61, 139},
	DarkSlateBlue         = {72, 61, 139},
	slate_blue            = {106, 90, 205},
	SlateBlue             = {106, 90, 205},
	medium_slate_blue     = {123, 104, 238},
	MediumSlateBlue       = {123, 104, 238},
	light_slate_blue      = {132, 112, 255},
	LightSlateBlue        = {132, 112, 255},
	medium_blue           = {0, 0, 205},
	MediumBlue            = {0, 0, 205},
	royal_blue            = {65, 105, 225},
	RoyalBlue             = {65, 105, 225},
	blue                  = {0, 0, 255},
	dodger_blue           = {30, 144, 255},
	DodgerBlue            = {30, 144, 255},
	deep_sky_blue         = {0, 191, 255},
	DeepSkyBlue           = {0, 191, 255},
	sky_blue              = {135, 206, 235},
	SkyBlue               = {135, 206, 235},
	light_sky_blue        = {135, 206, 250},
	LightSkyBlue          = {135, 206, 250},
	steel_blue            = {70, 130, 180},
	SteelBlue             = {70, 130, 180},
	light_steel_blue      = {176, 196, 222},
	LightSteelBlue        = {176, 196, 222},
	light_blue            = {173, 216, 230},
	LightBlue             = {173, 216, 230},
	powder_blue           = {176, 224, 230},
	PowderBlue            = {176, 224, 230},
	pale_turquoise        = {175, 238, 238},
	PaleTurquoise         = {175, 238, 238},
	dark_turquoise        = {0, 206, 209},
	DarkTurquoise         = {0, 206, 209},
	medium_turquoise      = {72, 209, 204},
	MediumTurquoise       = {72, 209, 204},
	turquoise             = {64, 224, 208},
	cyan                  = {0, 255, 255},
	light_cyan            = {224, 255, 255},
	LightCyan             = {224, 255, 255},
	cadet_blue            = {95, 158, 160},
	CadetBlue             = {95, 158, 160},
	medium_aquamarine     = {102, 205, 170},
	MediumAquamarine      = {102, 205, 170},
	aquamarine            = {127, 255, 212},
	dark_green            = {0, 100, 0},
	DarkGreen             = {0, 100, 0},
	dark_olive_green      = {85, 107, 47},
	DarkOliveGreen        = {85, 107, 47},
	dark_sea_green        = {143, 188, 143},
	DarkSeaGreen          = {143, 188, 143},
	sea_green             = {46, 139, 87},
	SeaGreen              = {46, 139, 87},
	medium_sea_green      = {60, 179, 113},
	MediumSeaGreen        = {60, 179, 113},
	light_sea_green       = {32, 178, 170},
	LightSeaGreen         = {32, 178, 170},
	pale_green            = {152, 251, 152},
	PaleGreen             = {152, 251, 152},
	spring_green          = {0, 255, 127},
	SpringGreen           = {0, 255, 127},
	lawn_green            = {124, 252, 0},
	LawnGreen             = {124, 252, 0},
	green                 = {0, 255, 0},
	chartreuse            = {127, 255, 0},
	medium_spring_green   = {0, 250, 154},
	MediumSpringGreen     = {0, 250, 154},
	green_yellow          = {173, 255, 47},
	GreenYellow           = {173, 255, 47},
	lime_green            = {50, 205, 50},
	LimeGreen             = {50, 205, 50},
	yellow_green          = {154, 205, 50},
	YellowGreen           = {154, 205, 50},
	forest_green          = {34, 139, 34},
	ForestGreen           = {34, 139, 34},
	olive_drab            = {107, 142, 35},
	OliveDrab             = {107, 142, 35},
	dark_khaki            = {189, 183, 107},
	DarkKhaki             = {189, 183, 107},
	khaki                 = {240, 230, 140},
	pale_goldenrod        = {238, 232, 170},
	PaleGoldenrod         = {238, 232, 170},
	light_goldenrod_yellow= {250, 250, 210},
	LightGoldenrodYellow  = {250, 250, 210},
	light_yellow          = {255, 255, 224},
	LightYellow           = {255, 255, 224},
	yellow                = {255, 255, 0},
	gold                  = {255, 215, 0},
	light_goldenrod       = {238, 221, 130},
	LightGoldenrod        = {238, 221, 130},
	goldenrod             = {218, 165, 32},
	dark_goldenrod        = {184, 134, 11},
	DarkGoldenrod         = {184, 134, 11},
	rosy_brown            = {188, 143, 143},
	RosyBrown             = {188, 143, 143},
	indian_red            = {205, 92, 92},
	IndianRed             = {205, 92, 92},
	saddle_brown          = {139, 69, 19},
	SaddleBrown           = {139, 69, 19},
	sienna                = {160, 82, 45},
	peru                  = {205, 133, 63},
	burlywood             = {222, 184, 135},
	beige                 = {245, 245, 220},
	wheat                 = {245, 222, 179},
	sandy_brown           = {244, 164, 96},
	SandyBrown            = {244, 164, 96},
	tan                   = {210, 180, 140},
	chocolate             = {210, 105, 30},
	firebrick             = {178, 34, 34},
	brown                 = {165, 42, 42},
	dark_salmon           = {233, 150, 122},
	DarkSalmon            = {233, 150, 122},
	salmon                = {250, 128, 114},
	light_salmon          = {255, 160, 122},
	LightSalmon           = {255, 160, 122},
	orange                = {255, 165, 0},
	dark_orange           = {255, 140, 0},
	DarkOrange            = {255, 140, 0},
	coral                 = {255, 127, 80},
	light_coral           = {240, 128, 128},
	LightCoral            = {240, 128, 128},
	tomato                = {255, 99, 71},
	orange_red            = {255, 69, 0},
	OrangeRed             = {255, 69, 0},
	red                   = {255, 0, 0},
	hot_pink              = {255, 105, 180},
	HotPink               = {255, 105, 180},
	deep_pink             = {255, 20, 147},
	DeepPink              = {255, 20, 147},
	pink                  = {255, 192, 203},
	light_pink            = {255, 182, 193},
	LightPink             = {255, 182, 193},
	pale_violet_red       = {219, 112, 147},
	PaleVioletRed         = {219, 112, 147},
	maroon                = {176, 48, 96},
	medium_violet_red     = {199, 21, 133},
	MediumVioletRed       = {199, 21, 133},
	violet_red            = {208, 32, 144},
	VioletRed             = {208, 32, 144},
	magenta               = {255, 0, 255},
	violet                = {238, 130, 238},
	plum                  = {221, 160, 221},
	orchid                = {218, 112, 214},
	medium_orchid         = {186, 85, 211},
	MediumOrchid          = {186, 85, 211},
	dark_orchid           = {153, 50, 204},
	DarkOrchid            = {153, 50, 204},
	dark_violet           = {148, 0, 211},
	DarkViolet            = {148, 0, 211},
	blue_violet           = {138, 43, 226},
	BlueViolet            = {138, 43, 226},
	purple                = {160, 32, 240},
	medium_purple         = {147, 112, 219},
	MediumPurple          = {147, 112, 219},
	thistle               = {216, 191, 216}
}



--- Move a custom gauge.
---
--- @usage This would move the health bar gauge to the location 1200, 400.
---   <pre>
---   moveGauge("healthBar", 1200, 400)
---   </pre>
---
--- @see createGauge
function moveGauge(gaugeName, newX, newY)
	local newX, newY = newX, newY

	assert(gaugesTable[gaugeName], "moveGauge: no such gauge exists.")
	assert(newX and newY, "moveGauge: need to have both X and Y dimensions.")

	moveWindow(gaugeName, newX, newY)
	moveWindow(gaugeName .. "_back", newX, newY)

	gaugesTable[gaugeName].xpos, gaugesTable[gaugeName].ypos = newX, newY
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

	hideWindow(gaugeName)
	hideWindow(gaugeName .. "_back", newX)
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

	showWindow(gaugeName)
	showWindow(gaugeName .. "_back", newX)
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
function setGaugeText(gaugeName, gaugeText, color1, color2, color3)
	assert(gaugesTable[gaugeName], "setGauge: no such gauge exists.")

	local red,green,blue = 0,0,0
	local l_labelText = gaugeText

	if color1 ~= nil then
		if color2 == nil then
			red, green, blue = getRGB(color1)
		else
			red, green, blue = color1, color2, color3
		end
	end

	-- Check to make sure we had a text to apply, if not, clear the text
	if l_labelText == nil then
		l_labelText = ""
	end

	local l__Echostring = [[<font color="#]] .. RGB2Hex(red,green,blue) .. [[">]] .. l_labelText .. [[</font>]]

	echo(gaugeName, l__Echostring)
	-- do not set the text for the back, because <center> tags mess with the alignment
	--echo(gaugeName .. "_back", l__Echostring)

	gaugesTable[gaugeName].text = l__Echostring
	gaugesTable[gaugeName].color1, gaugesTable[gaugeName].color2, gaugesTable[gaugeName].color3 = color1, color2, color3
end



--- Pads a hex number to ensure a minimum of 2 digits.
---
--- @usage Following command will returns "F0".
---   <pre>
---   PadHexNum("F")
---   </pre>
function PadHexNum(incString)
	local l_Return = incString
	if tonumber(incString,16)<16 then
		if tonumber(incString,16)<10 then
			l_Return = "0" .. l_Return
		elseif tonumber(incString,16)>10 then
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
	local l_Red, l_Green, l_Blue = 0,0,0
	if green == nil then -- Not an RGB but a "color" instead!
		l_Red, l_Green, l_Blue = getRGB(red)
	else -- Nope, true color here
		l_Red, l_Green, l_Blue = red, green, blue
	end

	return PadHexNum(string.format("%X",l_Red)) ..
		PadHexNum(string.format("%X",l_Green)) ..
		PadHexNum(string.format("%X",l_Blue))
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
--- @usage Finally we'll add some text to our gauche.
---   <pre>
---   createGauge("healthBar", 300, 20, 30, 300, "Now with some text", "green")
---   </pre>
function createGauge(gaugeName, width, height, Xpos, Ypos, gaugeText, color1, color2, color3)
	 -- make a nice background for our gauge
	createLabel(gaugeName .. "_back",0,0,0,0,1)
	if color2 == nil then
		local red, green, blue = getRGB(color1)
		setBackgroundColor(gaugeName .. "_back", red , green, blue, 100)
	else
		setBackgroundColor(gaugeName .. "_back", color1 ,color2, color3, 100)
	end
	moveWindow(gaugeName .. "_back", Xpos, Ypos)
	resizeWindow(gaugeName .. "_back", width, height)
	showWindow(gaugeName .. "_back")

	-- make a nicer front for our gauge
	createLabel(gaugeName,0,0,0,0,1)
	if color2 == nil then
		local red, green, blue = getRGB(color1)
		setBackgroundColor(gaugeName, red , green, blue, 255)
	else
		setBackgroundColor(gaugeName, color1 ,color2, color3, 255)
	end
	moveWindow(gaugeName, Xpos, Ypos)
	resizeWindow(gaugeName, width, height)
	showWindow(gaugeName)

	-- store important data in a table
	gaugesTable[gaugeName] = {width = width, height = height, xpos = Xpos, ypos = Ypos,text = gaugeText, color1 = color1, color2 = color2, color3 = color3}

	-- Set Gauge text (Defaults to black)
	-- If no gaugeText was passed, we'll just leave it blank!
	if gaugeText ~= nil then
		setGaugeText(gaugeName, gaugeText, "black")
	else
		setGaugeText(gaugeName)
	end
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

	resizeWindow(gaugeName, gaugesTable[gaugeName].width/100*((100/maxValue)*currentValue), gaugesTable[gaugeName].height)

	-- if we wanted to change the text, we do it
	if gaugeText ~= nil then
		echo(gaugeName .. "_back", gaugeText)
		echo(gaugeName, gaugeText)
		gaugesTable[gaugeName].text = gaugeText
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
	createMiniConsole(consoleName,0,0,1,1)
	setMiniConsoleFontSize(consoleName, fontSize)
	local x,y = calcFontSize( fontSize )
	resizeWindow(consoleName, x*charsPerLine, y*numberOfLines)
	setWindowWrap(consoleName, charsPerLine)
	moveWindow(consoleName, Xpos, Ypos)

	setBackgroundColor(consoleName,0,0,0,0)
	setFgColor(consoleName, 255,255,255)
end



--- Suffixes text at the end of the current line when used in a trigger.
---
--- @see prefix
function suffix(what, func, fg, bg, window)
	local length = string.len(line)
	moveCursor(window or "main", length-1, getLineNumber())
	if func and (func == cecho or func == decho or func == hecho) then
		func(what, fg, bg, true, window)
	else
		insertText(what)
	end
end



--- Prefixes text at the beginning of the current line when used in a trigger.
---
--- @usage Prefix the hours, minutes and seconds onto our prompt even though Mudlet has a button for that.
---   <pre>
---   prefix(os.date("%H:%M:%S "))
---   </pre>
---
--- @see suffix
function prefix(what, func, fg, bg, window)
	moveCursor(window or "main", 0, getLineNumber());
	if func and (func == cecho or func == decho or func == hecho) then
		func(what, fg, bg, true, window)
	else
		insertText(what)
	end
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
  local startp, endp = 1, 1
  while true do
    startp, endp = getCurrentLine():find(word, endp+(#what-#word)+1)
    if not startp then break end
    selectSection(startp-1, endp-startp+1)
    replace(what)
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



--- Prints out a formatted list of all available named colors, optional arg specifies number of columns to print in, defaults to 3
---
--- @usage Print list in 3 columns by default.
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
   local args = {...}
   local n = #args
   local cols, search
   if n > 1 then
      cols, search = args[1], args[2]
   elseif n == 1 and type(args[1]) == "string" then
      search = args[1]
   elseif n == 1 and type(args[1]) == "number" then
      cols = args[1]
   elseif n == 0 then
      cols = 4
      search = ""
   else
      error("showColors: Improper usage. Use showColors(columns, search)")
   end
   cols = cols or 4
   search = search and search:lower() or ""
   local i = 1
   for k,v in pairs(color_table) do
      if k:lower():find(search) then
         local fgc
         local luminosity = (0.2126 * ((v[1]/255)^2.2)) + (0.7152 * ((v[2]/255)^2.2)) + (0.0722 * ((v[3]/255)^2.2))
         if luminosity > 0.5 then
            fgc = "black"
         else
            fgc = "white"
         end
         fg(fgc)
         bg(k)
         echoLink(k..string.rep(" ", 23-k:len()),[[printCmdLine("]]..k..[[")]],v[1] ..", "..v[2]..", "..v[3],true)
         resetFormat()
         echo("  ")
         if i == cols then
            echo"\n"
            i = 1
         else
            i = i + 1
         end
      end
   end
end




--- <b><u>TODO</u></b> resizeGauge(gaugeName, width, height)
function resizeGauge(gaugeName, width, height)
	assert(gaugesTable[gaugeName], "resizeGauge: no such gauge exists.")
	assert(width and height, "resizeGauge: need to have both width and height.")

	resizeWindow(gaugeName, width, height)
	resizeWindow(gaugeName .. "_back", width, height)

	-- save in the table
	gaugesTable[gaugeName].width, gaugesTable[gaugeName].height = width, height
end



--- <b><u>TODO</u></b> setGaugeStyleSheet(gaugeName, css, cssback)
function setGaugeStyleSheet(gaugeName, css, cssback)
	if not setLabelStyleSheet then return end-- mudlet 1.0.5 and lower compatibility
	assert(gaugesTable[gaugeName], "setGaugeStyleSheet: no such gauge exists.")

	setLabelStyleSheet(gaugeName, css)
	setLabelStyleSheet(gaugeName .. "_back", cssback or css)
end



if rex then
	_Echos = {
		Patterns = {
			Hex = {
				[[(\x5c?\|c[0-9a-fA-F]{6}?(?:,[0-9a-fA-F]{6})?)|(\|r)]],
				rex.new[[\|c(?:([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?(?:,([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?]],
				},
			Decimal = {
				-- [[(\x5c?<[0-9,:]+>)|(<r>)]],
				[[(<[0-9,:]+>)|(<r>)]],
				rex.new[[<(?:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?(?::(?=>))?(?::([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?>]],
				},
			Color = {
				[[(<[a-zA-Z_,:]+>)]],
				rex.new[[<([a-zA-Z_]+)?(?:[:,](?=>))?(?:[:,]([a-zA-Z_]+))?>]],
				},
			Ansi = {
				[[(<[0-9,:]+>)]],
				rex.new[[<([0-9]{1,2})?(?::([0-9]{1,2}))?>]],
				},
			},
		Process = function(str, style)
			local t = {}
			local tonumber = tonumber

			for s, c, r in rex.split(str, _Echos.Patterns[style][1]) do
				if c and (c:byte(1) == 92) then
					c = c:sub(2)
					if s then s = s .. c else s = c end
					c = nil
				end
				if s then t[#t+1] = s end
				if r then t[#t+1] = "\27reset" end
				if c then
					if style == 'Hex' or style == 'Decimal' then
						local fr, fg, fb, br, bg, bb = _Echos.Patterns[style][2]:match(c)
						local color = {}
						if style == 'Hex' then
							if fr and fg and fb then fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16) end
							if br and bg and bb then br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16) end
						end
						if fr and fg and fb then color.fg = { fr, fg, fb } end
						if br and bg and bb then color.bg = { br, bg, bb } end
						t[#t+1] = color
					elseif style == 'Color' then
						if c == "<reset>" then t[#t+1] = "\27reset"
						else
							local fcolor, bcolor = _Echos.Patterns[style][2]:match(c)
							local color = {}
							if fcolor and color_table[fcolor] then color.fg = color_table[fcolor] end
							if bcolor and color_table[bcolor] then color.bg = color_table[bcolor] end
							if color.fg or color.bg then
								t[#t+1] = color
							else
								t[#t+1] = c
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
		local args = {...}
		local n = #args

		if func == 'echoLink' then
			if n < 3 then
				error'Insufficient arguments, usage: ([window, ] string, command, hint)'
			elseif n == 3 then
				str, cmd, hint = ...
			elseif n == 4 and type(args[4]) == 'boolean' then
				str, cmd, hint, fmt = ...
			elseif n >= 4 and type(args[4]) == 'string' then
				win, str, cmd, hint = ...
				if win == "main" then win = nil end
			else
				error'Improper arguments, usage: ([window, ] string, command, hint)'
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

		out = function(...)
			_G[func](...)
		end

		if win then
			reset = function()
				resetFormat(win)
			end
		else
			reset = function()
				resetFormat()
			end
		end

		local t = _Echos.Process(str, style)

		deselect()
		reset()

		for _, v in ipairs(t) do
			if type(v) == 'table' then
				if v.fg then
					local fr, fg, fb = unpack(v.fg)
					if win then setFgColor(win, fr, fg, fb) else setFgColor(fr, fg, fb) end
				end
				if v.bg then
					local br, bg, bb = unpack(v.bg)
					if win then setBgColor(win, br, bg, bb) else setBgColor(br, bg, bb) end
				end
			elseif v == "\27reset" then
				reset()
			else
				if func == 'echo' or func == 'insertText' then
					if win then out(win, v) else out(v) end
					if func == 'insertText' then
						moveCursor(window or "main", getColumnNumber() + string.len(v), getLineNumber())
					end
				else
					-- if win and fmt then setUnderline(win, true) elseif fmt then setUnderline(true) end -- not sure if underline is necessary unless asked for
					if win then out(win, v, cmd, hint, (fmt == true and true or false)) else out(v, cmd, hint, (fmt == true and true or false)) end
				end
			end
		end
		reset()
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
	function hecho(...) xEcho("Hex", "echo", ...) end



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
	function decho(...) xEcho("Decimal", "echo", ...) end



--- Echo string with embedded color name information.
---
--- @usage Consider following example:
---   <pre>
---   cecho("&lt;green&gt;green text &lt;blue&gt;blue text &lt;red&gt;red text")
---   </pre>
---
--- @see xEcho
--- @see cinsertText
	function cecho(...) xEcho("Color", "echo", ...) end


--- Inserts string with embedded hex color information.
---
--- @see xEcho
--- @see hecho
	function hinsertText(...) xEcho("Hex", "insertText", ...) end


--- Inserts string with embedded decimal color information.
---
--- @see xEcho
--- @see decho
	function dinsertText(...) xEcho("Decimal", "insertText", ...) end


--- Inserts string with embedded color name information.
---
--- @see xEcho
--- @see cecho
	function cinsertText(...) xEcho("Color", "insertText", ...) end


--- Echos a link with embedded hex color information.
---
--- @usage hechoLink([window, ] string, command, hint)
---
--- @see xEcho
--- @see hecho
	function hechoLink(...) xEcho("Hex", "echoLink", ...) end


--- Echos a link with embedded decimal color information.
---
--- @usage dechoLink([window, ] string, command, hint)
---
--- @see xEcho
--- @see decho
	function dechoLink(...) xEcho("Decimal", "echoLink", ...) end


--- Echos a link with embedded color name information.
---
--- @usage cechoLink([window, ] string, command, hint)
---
--- @see xEcho
--- @see cecho
	function cechoLink(...) xEcho("Color", "echoLink", ...) end


	-- Backwards compatibility
	checho = cecho


else


	-- NOT LUADOC
	-- See xEcho/another cecho for description.
	function cecho(window, text)
       local win = text and window
       local s = text or window
       if win == "main" then win = nil end

       if win then
            resetFormat(win)
       else
            resetFormat()
       end
       for color,text in string.gmatch("<white>"..s, "<([a-z_0-9, :]+)>([^<>]+)") do
          local colist   =   string.split(color..":", "%s*:%s*")
          local fgcol   =   colist[1] ~= "" and colist[1] or "white"
          local bgcol   =   colist[2] ~= "" and colist[2] or "black"
          local FGrgb   =   color_table[fgcol] or string.split(fgcol, ",")
          local BGrgb   =   color_table[bgcol] or string.split(bgcol, ",")

          if win then
             setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
             setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3])
             echo(win,text)
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
		if win == "main" then win = nil end
		local reset
		if win then
			reset = function() resetFormat(win) end
		else
			reset = function() resetFormat() end
		end
		reset()
		for color, text in s:gmatch("<([0-9,:]+)>([^<>]+)") do
			if color == "reset" then
				reset()
				if win then echo(win, text) else echo(text) end
			else
				local colist  =   string.split(color..":", "%s*:%s*")
				local fgcol   =   colist[1] ~= "" and colist[1] or "white"
				local bgcol   =   colist[2] ~= "" and colist[2] or "black"
				local FGrgb   =   color_table[fgcol] or string.split(fgcol, ",")
				local BGrgb   =   color_table[bgcol] or string.split(bgcol, ",")

				if win then
					setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
					setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3])
					echo(win,text)
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

