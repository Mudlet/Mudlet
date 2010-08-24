
----------------------------------------------------------------------------------
-- Useful global LUA functions that are accessible from within Mudlet
----------------------------------------------------------------------------------
-- These general functions can be used from anywhere within Mudlet scripts
-- They are described in the manual.
--------------------------------------------------------------------------------

-- default variables & function that are called by Mudlet

atcp = {}

function handleWindowResizeEvent()
end

function doSpeedWalk()
end

-----------------------------------------------------------------------------
-- General-purpose useful tools that were needed during development:
-----------------------------------------------------------------------------
if package.loaded["rex_pcre"] then rex = require"rex_pcre" end

-- Tests if a table is empty: this is useful in situations where you find
-- yourself wanting to do 'if my_table == {}' and such.
function table.is_empty(tbl)
   for k, v in pairs(tbl) do
      return false
   end
   return true
end

function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end

function string.ends(String,End)
   return End=='' or string.sub(String,-string.len(End))==End
end


----------------------------------------------------------------------------------
-- Functions written by Blaine von Roeder - December 2009
----------------------------------------------------------------------------------

-- This function flags a variable to be saved by Mudlet's variable persistence system.
-- Usage: remember("varName")
-- Example: remember("table_Weapons")
-- Example: remember("var_EnemyHeight")
-- Variables are automatically unpacked into the global namespace when the profile is loaded.
-- They are saved to "SavedVariables.lua" when the profile is closed or saved.

function remember(varName)
        if not _saveTable then
                _saveTable = {}
        end

    _saveTable[varName] = _G[varName]
end


--- This function should be primarily used by Mudlet. It loads saved settings in from the Mudlet home directory
--- and unpacks them into the global namespace.
function loadVars()
        if string.char(getMudletHomeDir():byte()) == "/" then _sep = "/" else  _sep = "\\" end
        local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
        local lt_VariableHolder = {}
        if (io.exists(l_SettingsFile)) then
                table.load(l_SettingsFile, lt_VariableHolder)
                for k,v in pairs(lt_VariableHolder) do
                                _G[k] = v
                end
        end
end

-- This function should primarily be used by Mudlet. It saves the contents of _saveTable into a file for persistence.

function saveVars()
        if string.char(getMudletHomeDir():byte()) == "/" then _sep = "/" else  _sep = "\\" end
        local l_SettingsFile = getMudletHomeDir() .. _sep .. "SavedVariables.lua"
    for k,_ in pairs(_saveTable) do
        remember(k)
    end
        table.save(l_SettingsFile, _saveTable)
end

-- Move a custom gauge built by createGauge(...)
-- Example: moveGauge("healthBar", 1200, 400)
-- This would move the health bar gauge to the location 1200, 400

function moveGauge(gaugeName, newX, newY)
        local newX, newY = newX, newY

        assert(gaugesTable[gaugeName], "moveGauge: no such gauge exists.")
        assert(newX and newY, "moveGauge: need to have both X and Y dimensions.")

        moveWindow(gaugeName, newX, newY)
        moveWindow(gaugeName .. "_back", newX, newY)

        gaugesTable[gaugeName].xpos, gaugesTable[gaugeName].ypos = newX, newY

end

-- Set the text on a custom gauge built by createGauge(...)
-- Example: setGaugeText("healthBar", "HP: 100%", 40, 40, 40)
-- Example: setGaugeText("healthBar", "HP: 100%", "red")
-- An empty gaugeText will clear the text entirely.
-- Colors are optional and will default to 0,0,0(black) if not passed as args.

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

        local l_EchoString = [[<font color="#]] .. RGB2Hex(red,green,blue) .. [[">]] .. l_labelText .. [[</font>]]


        echo(gaugeName, l_EchoString)
        echo(gaugeName .. "_back", l_EchoString)


        gaugesTable[gaugeName].text = l_EchoString
        gaugesTable[gaugeName].color1, gaugesTable[gaugeName].color2, gaugesTable[gaugeName].color3 = color1, color2, color3
end

-- Converts an RGB value into an HTML compliant(label usable) HEX number
-- This function is colorNames aware and can take any defined global color as its first arg
-- Example: RGB2Hex(255,255,255) returns "FFFFFF"
-- Example: RGB2Hex("white") returns "FFFFFF"
function RGB2Hex(red,green,blue)
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

-- Pads a hex number to ensure a minimum of 2 digits.
-- Example: PadHexNum("F") returns "F0
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

-----------------------------------------------------------
-- Functions written by John Dahlstrom November 2008
-----------------------------------------------------------

-- Example:
--
-- local red, green, blue = getRGB("green")
-- echo(red .. "." .. green .. "." .. blue )
--
-- This would then display 0.255.0 on your screen.

function getRGB(colorName)

        local red = color_table[colorName][1]
        local green = color_table[colorName][2]
        local blue = color_table[colorName][3]

        return red, green, blue

end


-- Make your very own customized gauge with this function.
--
-- Example:
--
-- createGauge("healthBar", 300, 20, 30, 300, nil, 0, 255, 0)
-- or
-- createGauge("healthBar", 300, 20, 30, 300, nil, "green")
--
-- This would make a gauge at that's 300px width, 20px in height, located at Xpos and Ypos and is green.
-- The second example is using the same names you'd use for something like fg() or bg().
--
-- If you wish to have some text on your label, you'll change the nil part and make it look like this:
-- createGauge("healthBar", 300, 20, 30, 300, "Now with some text", 0, 255, 0)
-- or
-- createGauge("healthBar", 300, 20, 30, 300, "Now with some text", "green")

gaugesTable = {} -- first we need to make this table which will be used later to store important data in...

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


-- Use this function when you want to change the gauges look according to your values.
--
-- Example:
--
-- setGauge("healthBar", 200, 400)
--
-- In that example, we'd change the looks of the gauge named healthBar and make it fill
-- to half of its capacity. The height is always remembered.
--
-- If you wish to change the text on your gauge, you'd do the following:
--
-- setGauge("healthBar", 200, 400, "some text")
--
-- Typical usage would be in a prompt with your current health or whatever value, and throw
-- in some variables instead of the numbers.

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


-- Make a new console window with ease. The default background is black and text color white.
--
-- Example:
--
-- createConsole("myConsoleWindow", 8, 80, 20, 200, 400)
--
-- This will create a miniconsole window that has a font size of 8pt, will display 80 characters in width,
-- hold a maximum of 20 lines and be place at 200x400 of your mudlet window.
-- If you wish to change the color you can easily do this when updating your text or manually somewhere, using
-- setFgColor() and setBackgroundColor().

function createConsole(consoleName, fontSize, charsPerLine, numberOfLines, Xpos, Ypos)

   createMiniConsole(consoleName,0,0,1,1)
   setMiniConsoleFontSize(consoleName, fontSize)
   local x,y = calcFontSize( fontSize )
   resizeWindow(consoleName, x*charsPerLine, y*numberOfLines)
   setWindowWrap(consoleName, Xpos)
   moveWindow(consoleName, Xpos, Ypos)

   setBackgroundColor(consoleName,0,0,0,0)
   setFgColor(consoleName, 255,255,255)

end

function sendAll( what, ... )
        if table.maxn(arg) == 0 then
                send( what )
    else
                local echo
                if arg[table.maxn(arg)] == false then echo = false else echo = true end
        send( what, echo )
        for i,v in ipairs(arg) do
                send( v, echo )
            end
    end
end


-- Echo something after your line
function suffix(what, func, fg, bg, window)
        local length = string.len(line)
        moveCursor(window or "main", length-1, getLineNumber())
        if func and (func == cecho or func == decho or func == hecho) then
                func(what, fg, bg, true, window)
        else
                insertText(what)
        end
end


-- Echo something before your line
function prefix(what, func, fg, bg, window)
    moveCursor(window or "main", 0, getLineNumber());
        if func and (func == cecho or func == decho or func == hecho) then
                func(what, fg, bg, true, window)
        else
                insertText(what)
        end
end


-- Gag the whole line
function gagLine()
        --selectString(line, 1)
        --replace("")
 deleteLine()
end


-- Replace all words on the current line by your choice

-- Example: replaceAll("John", "Doe")
-- This will replace the word John with the word Doe, everytime the word John occurs on the current line.
function replaceAll(word, what)
        while selectString(word, 1) > 0 do replace(what) end
end


-- Replace the whole with a string you'd like.
function replaceLine(what)
        selectString(line, 1)
        replace("")
        insertText(what)
end


-----------------------------------
-- some functions from Heiko
----------------------------------

-- default resizeEvent handler function.
-- overwrite this function to make a custom event handler if the main window is being resized
function handleResizeEvent()
end

function deselect()
        selectString("",1);
end

-- Function shows the content of a Lua table on the screen
function printTable( map )
        echo("-------------------------------------------------------\n");
        for k, v in pairs( map ) do
                echo( "key=" .. k .. " value=" .. v .. "\n" )
        end
        echo("-------------------------------------------------------\n");
end

function __printTable( k, v )
  insertText ("\nkey = " .. tostring (k) .. " value = " .. tostring( v )  )
end

-- Function colorizes all matched regex capture groups on the screen
function showCaptureGroups()
    for k, v in pairs ( matches ) do
        selectCaptureGroup( tonumber(k) )
        setFgColor( math.random(0,255), math.random(0,255), math.random(0,255) )
        setBgColor( math.random(0,255), math.random(0,255), math.random(0,255) )
    end
end

-- prints the content of the table multimatches[n][m] to the screen
-- this is meant as a tool to help write multiline trigger scripts
-- This helps you to easily see what your multiline trigger actually captured in all regex
-- You can use these values directly in your script by referring to it with multimatches[regex-number][capturegroup]
function showMultimatches()
    echo("\n-------------------------------------------------------");
    echo("\nThe table multimatches[n][m] contains:");
    echo("\n-------------------------------------------------------");
    for k,v in ipairs(multimatches) do
        echo("\nregex " .. k .. " captured: (multimatches["..k .."][1-n])");
        for k2,v2 in ipairs(v) do
                echo("\n          key="..k2.." value="..v2);
        end
    end
    echo("\n-------------------------------------------------------\n");
end

function listPrint( map )
        echo("-------------------------------------------------------\n");
        for k,v in ipairs( map ) do
                echo( k .. ". ) "..v .. "\n" );
        end
        echo("-------------------------------------------------------\n");
end

function listAdd( list, what )
        table.insert( list, what );
end


function listRemove( list, what )
        for k,v in ipairs( list ) do
                if v == what then
                        table.remove( list, k )
                end
        end
end

-------------------------------------------------------------------
--.... some functions from Tichi 2009
-------------------------------------------------------------------
-- Gets the actual size of a non-numerical table
function table.size(t)
        if not t then
                return 0
        end

        local i = 0
        for k, v in pairs(t) do
                i = i + 1
        end
        return i
end

-- Checks to see if a file exists
function io.exists(file)
        local f = io.open(file)
        if f then
                io.close(f)
                return true
        end
        return false
end

-- Splits a string
function string:split(delimiter)
        local result = { }
        local from  = 1
        local delim_from, delim_to = string.find( self, delimiter, from  )
        while delim_from do
                table.insert( result, string.sub( self, from , delim_from-1 ) )
                from  = delim_to + 1
                delim_from, delim_to = string.find( self, delimiter, from  )
        end
        table.insert( result, string.sub( self, from  ) )
        return result
end

-- Determines if a table contains a value as a key or as a value (recursive)
function table.contains(t, value)
        for k, v in pairs(t) do
                if v == value then
                        return true
                elseif k == value then
                        return true
                elseif type(v) == "table" then
                        if table.contains(v, value) then return true end
                end
        end

        return false
end



-----------------------------------------------------------
-- some functions from Vadim Peretrokin 2009
-----------------------------------------------------------

-------------------------------------------------------------------------------
--                     Color Definitions
--------------------------------------------------------------------------------
-- These color definitions are intended to be used in conjunction with fg()
-- and bg() colorizer functions that are defined further below
--------------------------------------------------------------------------------

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


-----------------------------------------------------------------------------------
--            Color Functions
-------------------------------------------------------------------------------------
-- sets current background color to a named color see table above
-- usage: bg( "magenta" )

function bg(name)
    setBgColor(color_table[name][1], color_table[name][2], color_table[name][3])
end

-- sets current foreground color see bg( ) above
function fg(name)
    setFgColor(color_table[name][1], color_table[name][2], color_table[name][3])
end

---------------------------------------------------------------------------------------
--              Save & Load Variables
---------------------------------------------------------------------------------------
-- The below functions can be used to save individual Lua tables to disc and load
-- them again at a later time e.g. make a database, collect statistical information etc.
-- These functions are also used by Mudlet to load & save the entire Lua session variables
--
-- table.load(file)   - loads a serialized file into the globals table (only Mudlet should use this)
-- table.load(file, table) - loads a serialized file into the given table

-- table.save(file)  - saves the globals table (minus some lua enviroment stuffs) into a file (only Mudlet should use this)
-- table.save(file, table) - saves the given table into the given file
--
-- Original code written by CHILLCODE™ on https://board.ptokax.ch, distributed under the same terms as Lua itself.
--
-- Notes:
--  Userdata and indices of these are not saved
--  Functions are saved via string.dump, so make sure it has no upvalues
--  References are saved
--

function table.save( sfile, t )
 if t == nil then t = _G end
 local tables = {}
 table.insert( tables, t )
 local lookup = { [t] = 1 }
 local file = io.open( sfile, "w" )
 file:write( "return {" )
 for i,v in ipairs( tables ) do
  table.pickle( v, file, tables, lookup )
 end
 file:write( "}" )
 file:close()
end

function table.pickle( t, file, tables, lookup )
 file:write( "{" )
 for i,v in pairs( t ) do
  -- escape functions
  if type( v ) ~= "function" and type( v ) ~= "userdata" and (i ~= "string" and i ~= "xpcall" and i ~= "package" and i ~= "os" and i ~= "io" and i ~= "math" and i ~= "debug" and i ~= "coroutine" and i ~= "_G" and i ~= "_VERSION" and i ~= "table") then
   -- handle index
   if type( i ) == "table" then
    if not lookup[i] then
     table.insert( tables, i )
     lookup[i] = table.maxn( tables )
    end
    file:write( "[{"..lookup[i].."}] = " )
   else
    local index = ( type( i ) == "string" and "[ "..string.enclose( i, 50 ).." ]" ) or string.format( "[%d]", i )
    file:write( index.." = " )
   end
   -- handle value
   if type( v ) == "table" then
    if not lookup[v] then
     table.insert( tables, v )
     lookup[v] = table.maxn( tables )
    end
    file:write( "{"..lookup[v].."}," )
   else
    local value =  ( type( v ) == "string" and string.enclose( v, 50 ) ) or tostring( v )
    file:write( value.."," )
   end
  end
 end
 file:write( "},\n" )
end

-- enclose string by long brakets ( string, maxlevel )
function string.enclose( s, maxlevel )
 s = "["..s.."]"
 local level = 0
 while 1 do
  if maxlevel and level == maxlevel then
   error( "error: maxlevel too low, "..maxlevel )
  --
  elseif string.find( s, "%["..string.rep( "=", level ).."%[" ) or string.find( s, "]"..string.rep( "=", level ).."]" ) then
   level = level + 1
  else
   return "["..string.rep( "=", level )..s..string.rep( "=", level ).."]"
  end
 end
end

function table.load( sfile, loadinto )
 local tables = dofile( sfile )
 if tables then
  if loadinto ~= nil and type(loadinto) == "table" then
   table.unpickle( tables[1], tables, loadinto )
  else
   table.unpickle( tables[1], tables, _G )
  end
 end
end

function table.unpickle( t, tables, tcopy, pickled )
 pickled = pickled or {}
 pickled[t] = tcopy
 for i,v in pairs( t ) do
  local i2 = i
  if type( i ) == "table" then
   local pointer = tables[ i[1] ]
   if pickled[pointer] then
    i2 = pickled[pointer]
   else
    i2 = {}
    table.unpickle( pointer, tables, i2, pickled )
   end
  end
  local v2 = v
  if type( v ) == "table" then
   local pointer = tables[ v[1] ]
   if pickled[pointer] then
    v2 = pickled[pointer]
   else
    v2 = {}
    table.unpickle( pointer, tables, v2, pickled )
   end
  end
  tcopy[i2] = v2
 end
end

-- Replaces the given wildcard (as a number) with the given text.
--
-- -- Example: replaceWildcard(1, "hello") on a trigger of `^You wave (goodbye)\.$`
function replaceWildcard(what, replacement)
    if replacement == nil or what == nil then
        return
    end
    selectCaptureGroup(what)
    replace(replacement)
end

----------------------------------------------------------------------------------
-- function by Ryan: pretty print function for tables
----------------------------------------------------------------------------------
-- usage: display( mytable )
----------------------------------------
-- pretty display function
function display(what, numformat, recursion)
  recursion = recursion or 0

  if recursion == 0 then
    echo("\n")
--    echo("-------------------------------------------------------\n")
  end
  echo(printable(what, numformat))

  -- Do all the stuff inside a table
  if type(what) == 'table' then
    echo(" {")

    local firstline = true   -- a kludge so empty tables print on one line
    for k, v in pairs(what) do
      if firstline then echo("\n"); firstline = false end
      echo(indent(recursion))
      echo(printable(k))
      echo(": ")
      if not (v == _G) then display(v, numformat, recursion + 1) end
    end

    -- so empty tables print as {} instead of {..indent..}
    if not firstline then echo(indent(recursion - 1)) end
    echo("}")
  end

  echo("\n")
  if recursion == 0 then
--    echo ("-------------------------------------------------------\n")
  end
end

-- Basically like tostring(), except takes a numformat
-- and is a little better suited for working with display()
function printable(what, numformat)
  local ret

  if type(what) == 'string' then
    ret = "'"..what.."'"
--    ret = string.format("%q", what)    -- this was ugly

  elseif type(what) == 'number' then
    if numformat then ret = string.format(numformat, what)
    else ret = what end

  elseif type(what) == 'boolean' then
    ret = tostring(what)

  elseif type(what) == 'table' then
    ret = what.__customtype or type(what)

  else
    ret = type(what)
--    ret = tostring(what)               -- this was ugly
  end

  return ret
end

-- Handles indentation
do local indents = {}  -- simulate a static variable
        function indent(num)

          if not indents[num] then
            indents[num] = ""
            for i = 0, num do
              indents[num] = indents[num].."  "
            end
          end

          return indents[num]
        end
end

function resizeGauge(gaugeName, width, height)
    assert(gaugesTable[gaugeName], "resizeGauge: no such gauge exists.")
    assert(width and height, "resizeGauge: need to have both width and height.")

        resizeWindow(gaugeName, width, height)
        resizeWindow(gaugeName .. "_back", width, height)

    -- save in the table
    gaugesTable[gaugeName].width, gaugesTable[gaugeName].height = width, height
end

function setGaugeStyleSheet(gaugeName, css, cssback)
    if not setLabelStyleSheet then return end-- mudlet 1.0.5 and lower compatibility
    assert(gaugesTable[gaugeName], "setGaugeStyleSheet: no such gauge exists.")

    setLabelStyleSheet(gaugeName, css)
    setLabelStyleSheet(gaugeName .. "_back", cssback or css)
end

----------------------------------------------------------------------------------
-- Functions written by Benjamin Smith - December 2009
----------------------------------------------------------------------------------

--[[------------------------------------------------------------------------------
Color echo functions: hecho, decho, cecho

Function: hecho()
Arg1: String to echo
Arg2: String containing value for foreground color in hexadecimal RGB format
Arg3: String containing value for background color in hexadecimal RGB format
Arg4: Bool that tells the function to use insertText() rather than echo()
Arg5: Name of the console to echo to. Defaults to main.

Color changes can be made within the string using the format |cFRFGFB,BRBGBB
where FR is the foreground red value, FG is the foreground green value, FB
is the foreground blue value, BR is the background red value, etc. ,BRBGBB
is optional. |r can be used within the string to reset the colors to default.

The colors in arg2 and arg3 replace the normal defaults for your console.
So if you use cecho("|cff0000Testing |rTesting", "00ff00", "0000ff"),
the first Testing would be red on black and the second would be green on blue.

Function: decho()
Arg1: String to echo
Arg2: String containing value for foreground color in decimal format
Arg3: String containing value for background color in decimal format
Arg4: Bool that tells the function to use insertText() rather than echo()
Arg5: Name of the console to echo to. Defaults to main.

Color changes can be made using the format <FR,FG,FB:BR,BG,BB> where
each field is a number from 0 to 255. The background portion can be omitted
using <FR,FG,FB> or the foreground portion can be omitted using <:BR,BG,BB>
Arguments 2 and 3 set the default fore and background colors for the string
using the same format as is used within the string, sans angle brackets,
e.g.  decho("test", "255,0,0", "0,255,0")

Function: cecho()
Arg1: String to echo
Arg2: String containing value for foreground color as a named color
Arg3: String containing value for background color as a named color
Arg4: Bool that tells the function to use insertText() rather than echo()
Arg5: Name of the console to echo to. Defaults to main.

Color changes can be made using the format <foreground:background>
where each field is one of the colors listed by showColors()
The background portion can be omitted using <foreground> or the foreground
portion can be omitted using <:background>
Arguments 2 and 3 to set the default colors take named colors as well.
--]]------------------------------------------------------------------------------

if rex then
        Echos = {
                Patterns = {
                        Hex = {
                                [[(\x5c?\|c[0-9a-fA-F]{6}?(?:,[0-9a-fA-F]{6})?)|(\|r)]],
                                rex.new[[\|c(?:([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?(?:,([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2}))?]],
                                },
                        Decimal = {
                                [[(\x5c?<[0-9,:]+>)|(<r>)]],
                                rex.new[[<(?:([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?(?::(?=>))?(?::([0-9]{1,3}),([0-9]{1,3}),([0-9]{1,3}))?>]],
                                },
                        Color = {
                                [[(\x5c?<[a-zA-Z_,:]+>)]],
                                rex.new[[<([a-zA-Z_]+)?(?:[:,](?=>))?(?:[:,]([a-zA-Z_]+))?>]],
                                },
                        Ansi = {
                                [[(\x5c?<[0-9,:]+>)]],
                                rex.new[[<([0-9]{1,2})?(?::([0-9]{1,2}))?>]],
                                },
                        },
                Process = function(str, style)
                        local t = {}
                        for s, c, r in rex.split(str, Echos.Patterns[style][1]) do
                                if c and (c:byte(1) == 92) then
                                        c = c:sub(2)
                                        if s then s = s .. c else s = c end
                                        c = nil
                                end
                                if s then table.insert(t, s) end
                                if r then table.insert(t, "r") end
                                if c then
                                        if style == 'Hex' or style == 'Decimal' then
                                                local fr, fg, fb, br, bg, bb = Echos.Patterns[style][2]:match(c)
                                                local color = {}
                                                if style == 'Hex' then
                                                        if fr and fg and fb then fr, fg, fb = tonumber(fr, 16), tonumber(fg, 16), tonumber(fb, 16) end
                                                        if br and bg and bb then br, bg, bb = tonumber(br, 16), tonumber(bg, 16), tonumber(bb, 16) end
                                                end
                                                if fr and fg and fb then color.fg = { fr, fg, fb } end
                                                if br and bg and bb then color.bg = { br, bg, bb } end
                                                table.insert(t, color)
                                        elseif style == 'Color' then
                                                if c == "<reset>" then table.insert(t, "r")
                                                else
                                                        local fcolor, bcolor = Echos.Patterns[style][2]:match(c)
                                                        local color = {}
                                                        if fcolor and color_table[fcolor] then color.fg = color_table[fcolor] end
                                                        if bcolor and color_table[bcolor] then color.bg = color_table[bcolor] end
                                                        table.insert(t, color)
                                                end
                                        end
                                end
                        end
                        return t
                end,
                }

        function xEcho(style, insert, win, str)
                if not str then str = win; win = nil end
                local reset, out
                if insert then
                        if win then
                                out = function(win, str)
                                        insertText(win, str)
                                end
                        else
                                out = function(str)
                                        insertText(str)
                                end
                        end
                else
                        if win then
                                out = function(win, str)
                                        echo(win, str)
                                end
                        else
                                out = function(str)
                                        echo(str)
                                end
                        end
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

                local t = Echos.Process(str, style)

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
                        elseif v == "r" then
                                reset()
                        else
                                if win then out(win, v) else out(v) end
                        end
                end
                if win then resetFormat(win) else resetFormat() end
        end

        function hecho(...) xEcho("Hex", false, ...) end
        function decho(...) xEcho("Decimal", false, ...) end
        function cecho(...) xEcho("Color", false, ...) end
        function hinsertText(...) xEcho("Hex", true, ...) end
        function dinsertText(...) xEcho("Decimal", true, ...) end
        function cinsertText(...) xEcho("Color", true, ...) end
        checho = cecho
else
-- HEIKO: using this as a replacement until the problems of the real function
--        are fixed.
    function cecho(window,text)
       local win = text and window
       local s = text or window
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

--	function cecho(window, text)
--		local win = text and window
--		local s = text or window
--		local reset
--		if win then
--			reset = function() resetFormat(win) end
--		else
--			reset = function() resetFormat() end
--		end
--		reset()
--		for color, text in s:gmatch("<([a-zA-Z_:]+)>([^<>]+)") do
--			if color == "reset" then
--				reset()
--				if win then echo(win, text) else echo(text) end
--			else
--				local colist  =   string.split(color..":", "%s*:%s*")
--				local fgcol   =   colist[1] ~= "" and colist[1] or "white"
--				local bgcol   =   colist[2] ~= "" and colist[2] or "black"
--				local FGrgb   =   color_table[fgcol]
--				local BGrgb   =   color_table[bgcol]
--
--				if win then
--					setFgColor(win, FGrgb[1], FGrgb[2], FGrgb[3])
--					setBgColor(win, BGrgb[1], BGrgb[2], BGrgb[3])
--					echo(win,text)
--				else
--					setFgColor(FGrgb[1], FGrgb[2], FGrgb[3])
--					setBgColor(BGrgb[1], BGrgb[2], BGrgb[3])
--					echo(text)
--				end
--			end
--		end
--		reset()
--	end

        function decho(window, text)
                local win = text and window
                local s = text or window
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

-- Extending default libraries makes Babelfish happy.
setmetatable( _G, {
        ["__call"] = function(func, ...)
                if type(func) == "function" then
                        return func(...)
                else
                        local h = metatable(func).__call
                        if h then
                                return h(func, ...)
                        elseif _G[type(func)][func] then
                                _G[type(func)][func](...)
                        else
                                debug("Error attempting to call function " .. func .. ", function does not exist.")
                        end
                end
        end,
        })

function xor(a, b) if (a and (not b)) or (b and (not a)) then return true else return false end end

function getOS()
    if string.char(getMudletHomeDir():byte()) == "/" then
        if string.find(os.getenv("HOME"), "home") == 2 then return "linux" else return "mac" end
        else return "windows"
    end
end

-- Opens URL in default browser
function openURL(url)
        local os = getOS()
        if os == "linux" then _G.os.execute("xdg-open " .. url)
        elseif os == "mac" then _G.os.execute("open " .. url)
        elseif os == "windows" then _G.os.execute("start " .. url) end
end

-- Prints out a formatted list of all available named colors, optional arg specifies number of columns to print in, defaults to 3
function showColors(...)
        local cols = ... or 3
        local i = 1
        for k,v in pairs(color_table) do
                local fg
                local luminosity = (0.2126 * ((v[1]/255)^2.2)) + (0.7152 * ((v[2]/255)^2.2)) + (0.0722 * ((v[3]/255)^2.2))
                if luminosity > 0.5 then
                        fg = "black"
                else
                        fg = "white"
                end
                cecho("<"..fg..":"..k..">"..k..string.rep(" ", 23-k:len()))
                echo"  "
                if i == cols then
                        echo"\n"
                        i = 1
                else
                        i = i + 1
                end
        end
end

-- Capitalize first character in a string
function string:title()
        self = self:gsub("^%l", string.upper, 1)
        return self
end

--// Set functions //

function _comp(a, b)
        if type(a) ~= type(b) then return false end
        if type(a) == 'table' then
                for k, v in pairs(a) do
                        if not b[k] then return false end
                        if not _comp(v, b[k]) then return false end
                end
        else
                if a ~= b then return false end
        end
        return true
end

--[[-----------------------------------------------------------------------------------------
Table Union
Returns a table that is the union of the provided tables. This is a union of key/value
pairs. If two or more tables contain different values associated with the same key,
that key in the returned table will contain a subtable containing all relevant values.
See table.n_union() for a union of values. Note that the resulting table may not be
reliably traversable with ipairs() due to the fact that it preserves keys. If there
is a gap in numerical indices, ipairs() will cease traversal.

Ex.
tableA = {
        [1] = 123,
        [2] = 456,
        ["test"] = "test",
        }

tableB = {
        [1] = 23,
        [3] = 7,
        ["test2"] = function() return true end,
        }

tableC = {
        [5] = "c",
        }

table.union(tableA, tableB, tableC) will return:
{
        [1] = {
                123,
                23,
                },
        [2] = 456,
        [3] = 7,
        [5] = "c",
        ["test"] = "test",
        ["test2"] = function() return true end,
}

--]]-----------------------------------------------------------------------------------------
function table.union(...)
        local sets = arg
        local union = {}

        for _, set in ipairs(sets) do
                for key, val in pairs(set) do
                        if union[key] and union[key] ~= val then
                                if type(union[key]) == 'table' then
                                        table.insert(union[key], val)
                                else
                                        union[key] = { union[key], val }
                                end
                        else
                                union[key] = val
                        end
                end
        end

        return union
end

--[[-----------------------------------------------------------------------------------------
Table Union
Returns a numerically indexed table that is the union of the provided tables. This is
a union of unique values. The order and keys of the input tables are not preserved.
--]]-----------------------------------------------------------------------------------------
function table.n_union(...)
        local sets = arg
        local union = {}
        local union_keys = {}

        for _, set in ipairs(sets) do
                for key, val in pairs(set) do
                        if not union_keys[val] then
                                union_keys[val] = true
                                table.insert(union, val)
                        end
                end
        end

        return union
end

--[[-----------------------------------------------------------------------------------------
Table Intersection
Returns a table that is the intersection of the provided tables. This is an
intersection of key/value pairs. See table.n_intersection() for an intersection of values.
Note that the resulting table may not be reliably traversable with ipairs() due to
the fact that it preserves keys. If there is a gap in numerical indices, ipairs() will
cease traversal.

Ex.
tableA = {
        [1] = 123,
        [2] = 456,
        [4] = { 1, 2 },
        [5] = "c",
        ["test"] = "test",
        }

tableB = {
        [1] = 123,
        [2] = 4,
        [3] = 7,
        [4] = { 1, 2 },
        ["test"] = function() return true end,
        }

tableC = {
        [1] = 123,
        [4] = { 1, 2 },
        [5] = "c",
        }

table.intersection(tableA, tableB, tableC) will return:
{
        [1] = 123,
        [4] = { 1, 2 },
}

--]]-----------------------------------------------------------------------------------------
function table.intersection(...)
        if #arg < 2 then return false end

        local intersection = {}

        local function intersect(set1, set2)
                local result = {}
                for key, val in pairs(set1) do
                        if set2[key] then
                                if _comp(val, set2[key]) then result[key] = val end
                        end
                end
                return result
        end

        intersection = intersect(arg[1], arg[2])

        for i, _ in ipairs(arg) do
                if i > 2 then
                        intersection = intersect(intersection, arg[i])
                end
        end

        return intersection
end

--[[-----------------------------------------------------------------------------------------
Table Intersection
Returns a numerically indexed table that is the intersection of the provided tables.
This is an intersection of unique values. The order and keys of the input tables are
not preserved.
--]]-----------------------------------------------------------------------------------------
function table.n_intersection(...)
        if #arg < 2 then return false end

        local intersection = {}

        local function intersect(set1, set2)
                local intersection_keys = {}
                local result = {}
                for _, val1 in pairs(set1) do
                        for _, val2 in pairs(set2) do
                                if _comp(val1, val2) and not intersection_keys[val1] then
                                        table.insert(result, val1)
                                        intersection_keys[val1] = true
                                end
                        end
                end
                return result
        end

        intersection = intersect(arg[1], arg[2])

        for i, _ in ipairs(arg) do
                if i > 2 then
                        intersection = intersect(intersection, arg[i])
                end
        end

        return intersection
end

--[[-----------------------------------------------------------------------------------------
Table Complement
Returns a table that is the relative complement of the first table with respect to
the second table. Returns a complement of key/value pairs.
--]]-----------------------------------------------------------------------------------------
function table.complement(set1, set2)
        if not set1 and set2 then return false end
        if type(set1) ~= 'table' or type(set2) ~= 'table' then return false end

        local complement = {}

        for key, val in pairs(set1) do
                if not _comp(set2[key], val) then
                        complement[key] = val
                end
        end
        return complement
end

--[[-----------------------------------------------------------------------------------------
Table Complement
Returns a table that is the relative complement of the first table with respect to
the second table. Returns a complement of values.
--]]-----------------------------------------------------------------------------------------
function table.n_complement(set1, set2)
        if not set1 and set2 then return false end

        local complement = {}

        for _, val1 in pairs(set1) do
                local insert = true
                for _, val2 in pairs(set2) do
                        if _comp(val1, val2) then
                                insert = false
                        end
                end
                if insert then table.insert(complement, val1) end
        end

        return complement
end

-- Contribution from Iocun
walklist = {}
walkdelay = 0

function speedwalktimer()
        send(walklist[1])
        table.remove(walklist, 1)
        if #walklist>0 then
                tempTimer(walkdelay, [[speedwalktimer()]])
        end
end

function speedwalk(dirString, backwards, delay)
        local dirString		= dirString:lower()
        walklist			= {}
        walkdelay			= delay
        local reversedir	= {
                n	= "s",
                en	= "sw",
                e	= "w",
                es	= "nw",
                s	= "n",
                ws	= "ne",
                w	= "e",
                wn	= "se",
                u	= "d",
                d	= "u",
                ni	= "out",
                tuo	= "in"
        }

        if not backwards then
                for count, direction in string.gmatch(dirString, "([0-9]*)([neswudio][ewnu]?t?)") do
                        count = (count == "" and 1 or count)
                        for i=1, count do
                                if delay then walklist[#walklist+1] = direction
                                else send(direction)
                                end
                        end
                end
        else
                for direction, count in string.gmatch(dirString:reverse(), "(t?[ewnu]?[neswudio])([0-9]*)") do
                        count = (count == "" and 1 or count)
                        for i=1, count do
                                if delay then walklist[#walklist+1] = reversedir[direction]
                                else send(reversedir[direction])
                                end
                        end
                end
        end

        if walkdelay then speedwalktimer() end
end

--[[-----------------------------------------------------------------------------------------
Variable Persistence
--]]-----------------------------------------------------------------------------------------

SavedVariables = { }

function SavedVariables:Add(tbl)
        if type(tbl) == 'string' then
                self[tbl] = _G[tbl]
        elseif type(tbl) == 'table' then
                for k,v in pairs(_G) do
                        if _comp(v, tbl) then
                                self[k] = tbl
                        end
                end
        else
                hecho"|cff0000Error registering table for persistence: invalid argument to SavedVariables:Add()"
        end
end
