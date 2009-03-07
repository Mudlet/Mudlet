

------------------------------------------------------------------------
-- Useful global LUA functions that are accessible from within Mudlet
------------------------------------------------------------------------
-- These general functions can be used from anywhere within Mudlet scripts
-- They are described in the manual.
------------------------------------------------------------------------

-----------------------------------------------------------
-- Functions written by John Dahlstrom November 2008
-----------------------------------------------------------


-- Send any amount of commands to the MUD
-- Example: sendAll("smile", "dance", "laugh")
function sendAll( what, ... )
	if table.maxn(arg) == 0 then
  		send( what )
    else
    	send(what)
    	for i,v in ipairs(arg) do
	    	-- v = the value of the arg
	    	send(v)
	    end
    end
end

         
-- Echo something after your line
function suffix(what)
	local length = string.len(line)
	moveCursor(length-1, 0)
	insertText(what)
end


-- Echo something before your line
function prefix(what)
	insertText(what)
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

-- Function shows the content of a Lua table on the screen
function printTable( map )
    table.foreach( matches, __printTable )
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

