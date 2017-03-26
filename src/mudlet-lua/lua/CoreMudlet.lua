----------------------------------------------------------------------------------
--- Mudlet Core Lua Functions
--- (file holds LuaDoc for all function implemented with in Mudlet Core)
----------------------------------------------------------------------------------


-- ensure that those function will not get defined
if false then



--- Pastes the previously copied rich text (including text formats like color) into user window name.
---
--- @usage
---   <pre>
---   selectString(line, 1)
---   copy()
---   appendBuffer("chat")
---   </pre>
---
--- @see selectString
--- @see copy
--- @see echo
function appendBuffer(windowName) end



--- This returns you the height and the length of letters for the given font size. As the primary intended
--- usage is for calculating the needed dimensions of a miniConsole, it doesn't accept a font argument -
--- as the miniConsoles currently only work with the default font for the sake of portability.
---
--- @usage Get font dimensions in pixes.
---   <pre>
---   x,y = calcFontSize(10)
---   </pre>
--- @usage Create a miniConsole that is 45 letters in length and 25 letters in height for font size 7
---   <pre>
---   font_size = 7
---   local x, y = calcFontSize( font_size )
---   createMiniConsole("map",0,0,0,0)
---   setMiniConsoleFontSize("map", font_size)
---   resizeWindow( "map", x*45, y*25 )
---   </pre>
function calcFontSize(fontSize) end



--- The <i>channel102 table</i> is used by Aardwolf mud for returning various information about you state. <br/>
--- Read <i>http://www.aardwolf.com/blog/2008/07/10/telnet-negotiation-control-mud-client-interaction/</i>
--- page for details.
---
--- @usage Display content of channel10 table.
---   <pre>
---   display(channel102)
---   </pre>
--- @usage Function for detecting AFK status on Aardwolf mud.
---   <pre>
---   function amIafk()
---      for k,v in pairs(channel102) do
---         if k==100 and v==4 then
---            return true
---         end
---      end
---      return false
---   end
---   </pre>
---
--- @see sendTelnetChannel102
---
--- @class function
--- @name channel102
channel102 = {}



--- Clears the user window or a mini console with the name given as argument.
--- <b><u>TODO</u></b> is this still working?
function clearUserWindow(windowName) end



--- <b><u>TODO</u></b>  clearWindow - TLuaInterpreter::clearUserWindow
function clearWindow(windowName) end



--- Clears the user window or a mini console with the name given as argument.
---
--- @param windowName optinal
function closeUserWindow(windowName) end



--- The <i>command variable</i> holds initial user command e.g. unchanged by any aliases or triggers.
--- This is typically used in alias scripts.
---
--- @see line
---
--- @class function
--- @name command
command = ""



--- Copies the current selection to the clipboard. This function operates on rich text, i.e.
--- the selected text including all its format codes like colors, fonts etc. in the clipboard
--- until it gets overwritten by another copy operation. example: This script copies the current
--- line on the main screen to a user window (mini console) named chat and gags the output on
--- the main screen.
---
--- @usage
---   <pre>
---   selectString(line)
---   copy()
---   appendBuffer("chat")
---   replace("This line has been moved to the chat window!")
---   </pre>
---
--- @see paste
--- @see appendBuffer
function copy() end



--- Creates a named buffer for formatted text, much like a user terminal window, but the buffer
--- cannot be shown on the screen; intended for temporary buffer work
function createBuffer(name) end



--- <b><u>TODO</u></b>  createButton - TLuaInterpreter::createButton
function createButton() end



--- Labels are intended for very small variable or prompt displays or images. labels are clickable and if you specify a callback function with
--- setLabelClickCallback( labelName, myLabelOnClickFunction ) your function will get called if the user clicks on the label with the mouse.
--- If fillBackground = 0, the background will be hidden, if fillBackground = 1 the background will be shown i.e. you can see the background color.
--- labels can be transparent. You can place labels anywhere within then main display, also als overlays over the main displays e.g. for on
--- screen buttons, micro display, etc. DON'T use labels for larger text displays because they are a lot slower than the highspeed mini consoles.
--- <br/><br/>
--- Labels accept some HTML and CSS code for text formating.
---
--- @return true or false
---
--- @usage This example creates a transparent overlay message box to show a big warning message "You are under attack!" in the middle of the screen.
---   Because the background color has a transparency level of 150 (0-255, with 0 being completely transparent and 255 non-transparent) the background
---   text can still be read through. The message box will disappear after 2.3 seconds.
---   <pre>
---   local width, height = getMainWindowSize()
---   createLabel("messageBox",(width/2)-300,(height/2)-100,250,150,1)
---   resizeWindow("messageBox",500,70)
---   moveWindow("messageBox", (width/2)-300,(height/2)-100 )
---   setBackgroundColor("messageBox", 150,100,100,200)
---   echo("messageBox", [[&lt;p style=&quot;font-size:35px&quot;&gt;&lt;b&gt;&lt;center&gt;&lt;font color=&quot;red&quot;&gt;You are under attack!&lt;/font&gt;&lt;/center&gt;&lt;/b&gt;&lt;/p&gt;]] )
---   showWindow("messageBox")
---   -- close the warning message box after 2.3 seconds
---   tempTimer(2.3, [[hideWindow("messageBox")]] )
---   </pre>
--- @usage see forum for more examples
---  <pre>
---  http://mudlet.sourceforge.net/phpBB3/viewtopic.php?f=6&t=95
---  http://mudlet.sourceforge.net/phpBB3/viewtopic.php?f=6&t=865
---  </pre>
---
--- @see createMiniConsole
--- @see hideWindow
--- @see showWindow
--- @see resizeWindow
--- @see setLabelClickCallback
--- @see setTextFormat
--- @see moveWindow
--- @see setMiniConsoleFontSize
--- @see handleWindowResizeEvent
--- @see setBorderTop
--- @see setBorderColor
function createLabel(name, posX, posY, width, height, fillBackground) end



--- Opens a console window inside the main window of Mudlet. MiniConsole is openes at position posX/posY with size
--- according to width/height (values depend on your own screen resolution usually between 0-1600 for x and 0-1024
--- for y). This console is the ideal fast colored text display for everything that requires a bit more text e.g.
--- status screens, log windows, chat windows etc.. You can use clearWindow/moveCursor etc. functions for this window
--- for custom printing as well as copy & paste functions for colored text copies from the main window or normal
--- echoUserWindow(name, text) for normal printing. To set word wrap see setWindowWrap. To move the main window to
--- make room for miniconsole windows on your screen (if you want to do this as you can also layer mini console
--- and label windows) see setBorderTop, setBorderColor To have your screen layout adjusted after the window size
--- of the main screen gets resized see handleWindowResizeEvent
---
--- @usage Set up the small system message window in the top right corner
---   <pre>
---   -- determine the size of your screen
---   WindowWidth = 0
---   WindowHeight = 0
---   WindowWidth, WindowHeight = getMainWindowSize()
---
---   createMiniConsole("sys",WindowWidth-650,0,650,300)
---   setBackgroundColor("sys",85,55,0,255)
---   setMiniConsoleFontSize("sys", 8)
---   -- wrap lines in window "sys" at 65 characters per line
---   setWindowWrap("sys", 40)
---   -- set default font colors and font style for window "sys"
---   setTextFormat("sys",0,35,255,50,50,50,0,0,0)
---
---   echo("sys","Hello world!")
---   </pre>
---
--- @see createLabel
--- @see hideWindow
--- @see showWindow
--- @see resizeWindow
--- @see setTextFormat
--- @see moveWindow
--- @see setMiniConsoleFontSize
--- @see handleWindowResizeEvent
--- @see setBorderTop
--- @see setBorderColor
--- @see setWindowWrap
---
--- @return true or false
function createMiniConsole(name, posX, posY, width, height) end



--- This function creates a stop watch. It is high resolution time
--- measurement tool. Stop watches can be started, stopped, reset and asked
--- how much time has passed since the stop watch has been started. Returns
--- the ID of a high resolution clock with milliseconds to measure time more
--- accurately than what is possible with std. Lua routines ?
--- startStopWatch, stopStopWatch, resetStopWatch, getStopWatchTime example:
--- In a global script you create all stop watches that you need in your
--- system and store the respective stopWatch-IDs in global variables:
---
--- @return stop watch ID
---
--- @usage
---   <pre>
---   fightStopWatchID = createStopWatch()
---   -- you store the watchID in a global variable to access it from anywhere
---   startStopWatch(fightStopWatch)
---   -- To stop the watch and measure its time in e.g. a trigger script you can write:
---   fightTime = stopStopWatch(fightStopWatchID)
---   echo("The fight lasted for " .. fightTime .. " seconds.")
---   resetStopWatch(fightStopWatchID)
---   </pre>
---
--- @see startStopWatch
--- @see resetStopWatch
--- @see getStopWatchTime
function createStopWatch() end



--- <b><u>TODO</u></b>  cut - TLuaInterpreter::cut
function cut() end



--- Deletes the current Line under the user cursor. Note: This is a high speed gagging tool
--- and it is very good at this task. It is meant to be used when the line can be omitted
--- entirely in the output. If you want to replace this line with something else have a look
--- at the replace() functions below. <br/><br/>
---
--- Note that scripts such as: deleteLine(); echo("this line is gone"); will not work because
--- lines that have been gagged with deleteLine() will not be rendered even if there is text
--- in the buffer. See wrapLine for details on how to force a re-render if this is necessary
--- for some reason. This is not the recommended way of replacing text.
---
--- @see wrapLine
--- @see replace
function deleteLine() end



--- Disables/deactivates an alias with the given name. This means that when you type in text that should
--- match it's pattern, it won't match and will be sent to the MUD. If several aliases have this name, they'll all be disabled.
function disableAlias(name) end



--- Uses trigger name as id or the id returned by tempTrigger() <b><u>TODO tempKey?</b></u>
function disableKey(name) end



--- Disables a timer from running it's script when it fires - so the timer
--- cycles will still be happening, just no action on them. If you'd like to
--- permanently delete it, use killTimer() instead. <br/><br/>
---
--- Use timer name or the id returned by tempTimer() to identify the timer
--- that you want to disable.
---
--- @see tempTimer
--- @see killTimer
function disableTimer(name) end



--- Use trigger name or the id returned by tempTrigger() to identify the
--- timer that you want to disable.
---
--- @see tempTrigger
function disableTrigger(name) end



--- Disconnect from current session without a proper logout.
---
--- @see reconnect
function disconnect() end



--- This function appends text at the end of the current line. The current cursor position is ignored.
--- Use moveCursor() and insertText() if you want to print at a different cursor position. <br/>
--- If the first argument is omitted the main console is used, otherwise the mini console windowName.
---
--- @param windowName optional
---
--- @usage Writes text to main window.
---   <pre>
---   echo("Hello world\n")
---   </pre>
--- @usage Writes text to the mini console named "info" if such a window exists.
---   <pre>
---   echo("info", "Hello this is the info window\n")
---   </pre>
---
--- @see moveCursor
--- @see insertText
function echo(windowName, text) end



--- Echos a piece of text as a clickable link. <br/>
---
--- @release Mudlet 1.1.0-pre1
---
--- @usage Following will create "click me now" link.
---   <pre>
---   echoLink("hi", [[echo("hey bub!")]], "click me now")
---   </pre>
---
--- @param windowName optional window name
--- @param text to display in the echo. Same as a normal echo().
--- @param command lua code to do when the link is clicked.
--- @param hint text for the tooltip to be displayed when the mouse is over the link.
--- @param useCurrentFormat true/false - controls whenever the current text formatting (colors, underline, bold, etc)
--- should be used, or the default link format.
---
--- @see setLink
--- @see insertLink
function echoLink(windowName, text, command, hint, useCurrentFormat) end



--- Same as setPopup() except it doesn't require a selection. Method echoPopup creates a link from the given text that it echos.
---
--- @release Mudlet 1.1.0-pre1
---
--- @usage
---   <pre>
---   echoPopup("activities to do", {[[send "sleep"]], [[send "sit"]], [[send "stand"]]}, {"sleep", "sit", "stand"})
---   </pre>
---
--- @see setPopup
--- @see insertPopup
function echoPopup() end



--- This function will print text to both mini console windows, dock windows and labels.
--- <b>This function is outdated. Use echo( windowName, text ) instead. </b>
---
--- @see echo
function echoUserWindow(windowName, text) end



--- Enables/activates the alias by it's name. If several aliases have this name, they'll all be enabled.
function enableAlias(name) end



--- Enable key or key group "name" (hot keys or action keys).
function enableKey(name) end



--- Enables or activates a timer that was previously disabled. The parameter
--- "name" expects the timer ID that was returned by tempTimer() on creation
--- of the timer or the name of the timer in case of a GUI timer
function enableTimer(name) end



--- Enables a Trigger. see enableTimer() for more details.
---
--- @see enableTimer
function enableTrigger(name) end



--- Tells you how many things of the given type exist.
---
--- @param type can be "alias", "trigger", or "timer".
---
--- @return number
---
--- @usage
---   <pre>
---   echo("I have " .. exists("my trigger", "trigger") .. " triggers called 'my trigger'!")
---   </pre>
--- @usage You can also use this alias to avoid creating duplicate things, for example:
---   <pre>
---   -- this code doesn't check if an alias already exists and will keep creating new aliases
---   permAlias("Attack", "General", "^aa$", [[send ("kick rat")]])
---
---   -- while this code will make sure that such an alias doesn't exist first
---   -- we do == 0 instead of 'not exists' because 0 is considered true in Lua
---   if exists("Attack", "alias") == 0 then
---       permAlias("Attack", "General", "^aa$", [[send ("kick rat")]])
---   end
---   </pre>
---
--- @see isActive
function exists(name, type) end



--- Like send(), but without bypassing alias expansion. This function may lead to <b>infinite recursion</b> if
--- you are not careful! <br/><br/>
---
--- Now, while in Mudlet you can call another alias with the expandAlias() function, this is strongly
--- discouraged. What you do instead if create a function (for example, send()  and echo()  are functions)
--- that you can then call from your alias or trigger. This has many advantages - it's faster, you can
--- easily give your function values, and your function can return you values, too.<br/><br/>
---
--- Note: The variable "command" contains what was entered in the command line or issued via the expandAlias()
--- function. If you use expandAlias( command ) inside an alias script the command would be doubled. You have
--- to use send( ) inside an alias script to prevent recursion. This will send the data directly and bypass
--- the alias expansion.
---
--- @see send
function expandAlias(command, print=1) end



--- This function will have Mudlet parse the given text as if it came from the MUD - one great application
--- is trigger testing. You can use \n to represent a new line. The function also accept ANSI color codes that
--- are used in MUDs. A sample table can be found here.
---
--- @usage Usage of new line characters.
---   <pre>
---   feedTriggers("\nYou sit yourself down.\n")
---   </pre>
--- @usage Demonstration of ANSI color. <br>
---	  <pre>
---   feedTriggers("\nThis is \27[1;32mgreen\27[0;37m, \27[1;31mred\27[0;37m, " ..
---      "\27[46mcyan background\27[0;37m,\27[32;47mwhite background and green foreground\27[0;37m.\n")
---   </pre>
function feedTriggers(text) end



--- Get the RGB values of the first character of the current selection.
---
--- @param windowName optional
---
--- @usage Getting RGB component.
---   <pre>
---   r,g,b = getBgColor(windowName)
---   </pre>
---
--- @see getFgColor
function getBgColor(windowName) end



--- This function can be used in checkbox button scripts (2-state buttons) to determine the current state of the checkbox.
---
--- @usage
---   <pre>
---   checked = getButtonStated()
---   if checked == 1 then
---       hideExits()
---   else
---       showExits()
---   end
---   </pre>
---
--- @return numneric state; state = 1 button is checked and state = 0, button is not checked
function getButtonState() end



--- Gets the absolute column number of the current user cursor.
function getColumnNumber() end



--- Returns the content of the current line under the user cursor in the
--- buffer. The Lua variable line holds the content of getCurrentLine()
--- before any triggers have been run on this line. When triggers change the
--- content of the buffer, the variable line will not be adjusted and thus
--- hold an outdated string. line = getCurrentLine() will update line to the
--- real content of the current buffer. This is important if you want to
--- copy the current line after it has been changed by some triggers.
--- selectString( line,1 ) will return false and won't select anything
--- because line no longer equals getCurrentLine(). Consequently,
--- selectString( getCurrentLine(), 1 ) is what you need.
---
--- @usage Update line variable with content of current line.
---   <pre>
---   line = getCurrentLine()
---   </pre>
---
--- @see selectString
function getCurrentLine() end



--- This function returns the RGB values of the color of the first character of the current selection
--- on mini console (window) windowName.
---
--- @param windowName optional - if windowName is omitted Mudlet will use the main screen.
---
--- @usage
---   <pre>
---   r,g,b = getFgColor(windowName)
---   </pre>
--- @usage
---   <pre>
---   local r,g,b
---   selectString("troll", 1)
---   r,g,b = getFgColor()
---   if r == 255 and g == 0 and b == 0 then
---       echo("HELP! troll is written in red letters, the monster is aggressive!\n")
---   end
---   </pre>
---
--- @see getBgColor
function getFgColor(windowName) end



--- Returns number of the last line in the text buffer.
function getLastLineNumber() end



--- Gets the absolute amount of lines in the current console buffer.
---
--- @return number
function getLineCount() end



--- Gets the absolute line number of the current user cursor.
function getLineNumber() end



--- Returns a Lua table with the content of the lines on a per line basis. Absolute line numbers are used.
---
--- @return section of the content of the screen text buffer. The form of the return value is: Lua_table[relative_linenumber, content]
function getLines(from_line_number, to_line_number) end



--- Return window width and window height (function have two return values). This is useful for calculating the window dimensions and
--- placement of custom GUI toolkit items like labels, buttons, mini consoles etc.
---
--- @usage
---   <pre>
---   mainWindowWidth, mainWindowHeight = getMainWindowSize()
---   </pre>
---
--- @return 2 numbers, width and height in pixels
function getMainWindowSize() end



--- Returns the current home directory of the current profile. This can be used to store data,
--- save statistical information or load resource files.
---
--- @usage Get directory to current profile.
---   <pre>
---   homedir = getMudletHomeDir()
---   </pre>
function getMudletHomeDir() end



--- Returns the last measured response time between the sent command and the server reply.
---
--- @return number of seconds (0.058 (=58 milliseconds lag) or 0.309 (=309 milliseconds))
function getNetworkLatency() end



--- Returns the time without stoping stop watch (milliseconds based) in form of 0.058
--- (= clock ran for 58 milliseconds before it was stopped).
--- @see createStopWatch
function getStopWatchTime(watchID) end



--- Return time information.
---
--- @release Mudlet 1.0.6
---
--- @usage Get time as a table.
---   <pre>
---   getTime()
---   </pre>
--- @usage Get time as a string (with default formatting).
---   <pre>
---   getTime(true)
---   </pre>
--- @usage Get time as a string with user defined formatting.
---   <pre>
---   getTime(true, "hh:mm")
---   </pre>
---
--- @param returnType Takes a boolean value (in Lua anything but false or nil will translate to true). If true,
---   the function will return a table in the following format: <br/>
---   { hour = #, min = #, sec = #, msec = # } <br/>
---   If false or nil, it will return the time as a string using a format passed to the second arg or the
---   default of hh:mm:ss.zzz
---
--- @param format Format expressions built from following elements:
---   <pre>
---   h               the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display)
---   hh              the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display)
---   H               the hour without a leading zero (0 to 23, even with AM/PM display)
---   HH              the hour with a leading zero (00 to 23, even with AM/PM display)
---   m               the minute without a leading zero (0 to 59)
---   mm              the minute with a leading zero (00 to 59)
---   s               the second without a leading zero (0 to 59)
---   ss              the second with a leading zero (00 to 59)
---   z               the milliseconds without leading zeroes (0 to 999)
---   zzz             the milliseconds with leading zeroes (000 to 999)
---   AP or A         use AM/PM display. AP will be replaced by either "AM" or "PM".
---   ap or a         use am/pm display. ap will be replaced by either "am" or "pm".
---
---   d               the day as number without a leading zero (1 to 31)
---   dd              the day as number with a leading zero (01 to 31)
---   ddd             the abbreviated localized day name (e.g. 'Mon' to 'Sun'). Uses QDate::shortDayName().
---   dddd            the long localized day name (e.g. 'Monday' to 'Qt::Sunday'). Uses QDate::longDayName().
---   M               the month as number without a leading zero (1-12)
---   MM              the month as number with a leading zero (01-12)
---   MMM             the abbreviated localized month name (e.g. 'Jan' to 'Dec'). Uses QDate::shortMonthName().
---   MMMM            the long localized month name (e.g. 'January' to 'December'). Uses QDate::longMonthName().
---   yy              the year as two digit number (00-99)
---   yyyy            the year as four digit number
---   </pre>
---   All other input characters will be ignored. Any sequence of characters that are enclosed in singlequotes will be treated as text and not be
---   used as an expression. Two consecutive singlequotes ("''") are replaced by a singlequote in the output.
function getTime(returnType, format) end



--- Returns the timestamp string as it's seen when you enable the timestamps view (blue 'i' button bottom right of the Mudlet screen).
---
--- @release Mudlet 1.1.0-pre1
---
--- @usage Echo the timestamp of the current line in a trigger.
---   <pre>
---   echo( getTimestamp(getLineCount()) )
---   </pre>
---
--- @param console_name optional parameter
function getTimestamp(console_name, lineNumber) end



--- <b><u>TODO</u></b>  hasFocus - TLuaInterpreter::hasFocus
function hasFocus() end



--- Hides tool bar name and makes it disappear. If all tool bars of a tool bar area (top, left, right)
--- are hidden, the entire tool bar area disappears automatically.
function hideToolBar(name) end



--- This function hides a mini console label. To show it again use showWindow.
---
--- @see showWindow
--- @see createMiniConsole
function hideWindow(name) end



--- <b><u>TODO</u></b>  insertHTML - TLuaInterpreter::insertHTML
function insertHTML() end



--- Same as echoLink() but inserts the text at the current cursor position, while echoLink inserts at the end
--- of the current line.
---
--- @release Mudlet 1.1.0-pre1
---
--- @see echoLink
--- @see setLink
function insertLink(windowName, text, command, hint, useCurrentFormat) end



--- Same as echoPopup(), but inserts text at the current cursor position.
---
--- @release Mudlet 1.1.0-pre1
---
--- @see setPopup
--- @see echoPopup
function insertPopup(windowName, text, commands, hints, useCurrentFormat) end



--- Inserts text at the current cursor position in the main window. If the cursor has not been explicitly
--- moved this function will always print at the beginning of the line whereas the echo() function will
--- always print at the end of the line.
---
--- @param windowName optional
---
--- @see echo
function insertText(windowName, text) end



--- Opens a file chooser dialog, allowing the user to select a file or a folder visually. The function returns
--- the selected path or nil if there was none chosen.
---
--- @usage
---   <pre>
---   local path = invokeFileDialog(false, "Find me the gfx folder, please")
---   if path then
---      echo(path)
---   end
---   </pre>
---
--- @param selection true for file selection, false for folder selection.
--- @param dialogTitle name that the dialog (file chooser window) will have
function invokeFileDialog(selection, dialogTitle) end



--- You can use this function to check if something, or somethings, are active. Type can be either
--- "alias", "trigger", or "timer", and name is the name of the said item you'd like to check.
---
--- @param type could be "alias", "trigger", or "timer"
---
--- @usage This will check is my "my trigger" is active.
---   <pre>
---   echo("I have " .. isActive("my trigger", "trigger") .. " currently active triggers called 'my trigger'!")
---   </pre>
function isActive(name, type) end



--- This function tests if the first character of the current selection has the background color specified by ansiBgColorCode.
---
--- @see isAnsiFgColor
function isAnsiBgColor(ansiBgColorCode) end



--- This function tests if the first character of the current selection has the foreground color specified by
--- ansiFgColorCode. Codes are:
---
---   <pre>
---   0 = default text color
---   1 = light black
---   2 = dark black
---   3 = light red
---   4 = dark red
---   5 = light green
---   6 = dark green
---   7 = light yellow
---   8 = dark yellow
---   9 = light blue
---   10 = dark blue
---   11 = light magenta
---   12 = dark magenta
---   13 = light cyan
---   14 = dark cyan
---   15 = light white
---   16 = dark white
---   </pre>
---
--- @usage
---   <pre>
---   selectString(matches[1], 1)
---   if isAnsiFgColor(5) then
---       bg("red")
---       resetFormat()
---       echo("yes, the text is light green")
---   else
---       echo( "no sorry, some other foreground color" )
---   end
---   </pre>
---
---   Note that matches[1] holds the matched trigger pattern - even in substring, exact match, begin of line substring trigger
---   patterns or even color triggers that do not know about the concept of capture groups. Consequently, you can always test
---   if the text that has fired the trigger has a certain color and react accordingly. This function is faster than using
---   getFgColor() and then handling the color comparison in Lua.
function isAnsiFgColor(ansiFgColorCode) end



--- Returns true or false depending on if the current line being processed is a prompt. This infallible
--- feature is available for MUDs that supply GA events (to check if yours is one, look to bottom-right
--- of the main window - if it doesn't say &lt;No&nbsp;GA&gt;, then it supplies them).
---
--- @return true/false
function isPrompt() end



--- Deletes an alias with the given name. If several aliases have this name, they'll all be deleted.
---
--- @see killTimer
--- @see killTrigger
function killAlias(name) end



--- Deletes a tempTimer. Use the Timer ID returned by tempTimer() as name parameter. ID is a string and not a number.
--- This function returns true on success and false if the timer id doesn't exist anymore (=timer has already fired)
--- or the timer is not a temp timer. Note that non-temporary timers that you have set up in the GUI cannot be deleted
--- with this function. Use disableTimer() to turn them on or off.
---
--- @see killAlias
--- @see killTrigger
---
--- @return true or false
function killTimer(id) end



--- Deletes a tempTrigger according to trigger ID. ID is a string value, not a number.
---
--- @see killAlias
--- @see killTimer
---
--- @return true or false
function killTrigger(id) end



--- The <i>line variable</> holds the content of the current line as being processed by the trigger engine.
--- The engine runs all triggers on each line as it arrives from the MUD.
---
--- @see command
---
--- @class function
--- @name line
line = ""



--- <b><u>TODO</u></b>  loadRawFile - TLuaInterpreter::loadRawFile
function loadRawFile() end



--- The <i>matches table</i> contains captured group. This available only within trigger context.
--- First item of matches table (matches[1]) holds current line, all other contains capture groups
--- defined by regular expression trigger.
---
--- @usage Let say that you defined following trigger to detect and get dropped items.
---   <pre>
---   RegEx:
---      ^The (.*) drops (.*)\.$
---   Code:
---      display(matches)
---      send("get " .. matches[3])
---   </pre>
---
--- Now let's say that MUD will send you following text (which will invoke your trigger).
---   <pre>
---   The skeleton drops scimitar.
---   </pre>
---
--- This will send "get scimitar" to your MUD and also print following table:
---   <pre>
---   table {
---     1: 'The skeleton drops scimitar.'
---     2: 'skeleton'
---     3: 'scimitar'
---   }
---   </pre>
---
--- @see showCaptureGroups
--- @see multimatches
--- @fee feedTriggers
---
--- @class function
--- @name matches
matches = {}



--- Moves the user cursor of the window windowName to the absolute point (x,y). This function returns false
--- if such a move is impossible e.g. the coordinates don't exist. To determine the correct coordinates use
--- getLineNumber(), getColumnNumber() and getLastLineNumber(). The trigger engine will always place the user
--- cursor at the beginning of the current line before the script is run. If you omit the windowName argument,
--- the main screen will be used.
---
--- @return true or false
---
--- @usage Set up the small system message window in the top right corner.
---   <pre>
---   -- determine the size of your screen
---   WindowWidth=0;
---   WindowHeight=0;
---   WindowWidth, WindowHeight = getMainWindowSize();
---
---   -- define a mini console named "sys" and set its background color
---   createMiniConsole("sys",WindowWidth-650,0,650,300)
---   setBackgroundColor("sys",85,55,0,255);
---
---   -- you *must* set the font size, otherwise mini windows will not work properly
---   setMiniConsoleFontSize("sys", 12);
---   -- wrap lines in window "sys" at 65 characters per line
---   setWindowWrap("sys", 60);
---   -- set default font colors and font style for window "sys"
---   setTextFormat("sys",0,35,255,50,50,50,0,0,0);
---   -- clear the window
---   clearUserWindow("sys")
---
---   moveCursorEnd("sys")
---   setFgColor("sys", 10,10,0)
---   setBgColor("sys", 0,0,255)
---   echo("sys", "test1---line1\n<this line is to be deleted>\n<this line is to be deleted also>\n")
---   echo("sys", "test1---line2\n")
---   echo("sys", "test1---line3\n")
---   setTextFormat("sys",158,0,255,255,0,255,0,0,0);
---   --setFgColor("sys",255,0,0);
---   echo("sys", "test1---line4\n")
---   echo("sys", "test1---line5\n")
---   moveCursor("sys", 1,1)
---
---   -- deleting lines 2+3
---   deleteLine("sys");
---   deleteLine("sys");
---
---   -- inserting a line at pos 5,2
---   moveCursor("sys", 5,2);
---   setFgColor("sys", 100,100,0)
---   setBgColor("sys", 255,100,0)
---   insertText("sys","############## line inserted at pos 5/2 ##############");
---
---   -- inserting a line at pos 0,0
---   moveCursor("sys", 0,0)
---   selectCurrentLine("sys");
---   setFgColor("sys", 255,155,255);
---   setBold( "sys", true );
---   setUnderline( "sys", true );
---   setItalics( "sys", true );
---   insertText("sys", "------- line inserted at: 0/0 -----\n");
---
---   setBold( "sys", true )
---   setUnderline( "sys", false )
---   setItalics( "sys", false )
---   setFgColor("sys", 255,100,0)
---   setBgColor("sys", 155,155,0)
---   echo("sys", "*** This is the end. ***\n");
---   </pre>
function moveCursor(windowName, x, y) end



--- Moves the cursor to the end of the buffer. "main" is the name of the main window, otherwise use the name of your user window.
---
--- @return true or false
---
--- @see moveCursor
function moveCursorEnd(windowName) end



--- This function moves window name to the given x/y coordinate. The main screen cannot
--- be moved. Instead you'll have to set appropriate border values in preferences to move
--- the main screen e.g. to make room for chat or information mini consoles, or other GUI
--- elements. In the future moveWindow() will set the border values automatically if the name
--- parameter is omitted.
---
--- @see createMiniConsole
--- @see createLabel
--- @see handleWindowResizeEvent
--- @see resizeWindow
--- @see setBorderTop
function moveWindow(name, x, y) end



--- The <i>multimatches table</i> is being used by Mudlet in the context of multiline triggers that use Perl regular expression.
--- It holds the table matches[n] as described above for each Perl regular expression based condition of the multiline trigger.
--- multimatches[5][4] may hold the 3rd capture group of the 5th regex in the multiline trigger. This way you can examine and
--- process all relevant data within a single script. <br/><br/>
---
--- What makes multiline triggers really shine is the ability to react to MUD output that is spread over multiple lines
--- and only fire the action (=run the script) if all conditions have been fulfilled in the specified amount of lines.
---
--- @usage Have a look at this example (can be tested on the MUD batmud.bat.org). <br/>
--- In the case of a multiline trigger with these 2 Perl regex as conditions:
---   <pre>
---   ^You have (\w+) (\w+) (\w+) (\w+)
---   ^You are (\w+).*(\w+).*
---   </pre>
--- The command "score" generates the following output on batMUD:
---   <pre>
---   You have an almost non-existent ability for avoiding hits.
---   You are irreproachably kind.
---   You have not completed any quests.
---   You are refreshed, hungry, very young and brave.
---   Conquer leads the human race.
---   Hp:295/295 Sp:132/132 Ep:182/181 Exp:269 >
---   </pre>
--- If you add this script to the trigger:
---   <pre>
---   showMultimatches()
---   </pre>
--- The script, i.e. the call to the function showMultimatches() generates this output:
---   <pre>
---   -------------------------------------------------------
---   The table multimatches[n][m] contains:
---   -------------------------------------------------------
---   regex 1 captured: (multimatches[1][1-n])
---             key=1 value=You have not completed any quests
---             key=2 value=not
---             key=3 value=completed
---             key=4 value=any
---             key=5 value=quests
---   regex 2 captured: (multimatches[2][1-n])
---             key=1 value=You are refreshed, hungry, very young and brave
---             key=2 value=refreshed
---             key=3 value=young
---             key=4 value=and
---             key=5 value=brave
---   -------------------------------------------------------
---   </pre>
--- The function showMultimatches() prints out the content of the table multimatches[n][m]. You can now see what the table
--- multimatches[][] contains in this case. The first trigger condition (=regex 1) got as the first full match
--- "You have not completed any quests". This is stored in multimatches[1][1] as the value of key=1 in the sub-table matches[1]
--- which, in turn, is the value of key=1 of the table multimatches[n][m]. <br/><br/>
---
--- Following script would use matched values from previously defined regex in the multiline trigger:
---   <pre>
---   myGold = myGold + tonumber( multimatches[1][2] )
---   mySilver = mySilver + tonumber( multimatches[1][3] )
---   </pre>
---
--- @see showMultimatches
--- @see matches
---
--- @class function
--- @name multimatches
multimatches = {}



--- Opens a user dockable console window for user output e.g. statistics, chat etc. If a window of such
--- a name already exists, nothing happens. You can move these windows, dock them, make them into notebook
--- tabs or float them.<br/><br/>
---
--- Note: There isn't currently way how to set size and position of user windows at the moment, so you might
--- consider to use mini console instead.
---
--- @usage This command will open new user windows with name "Chat".
---   <pre>
---   openUserWindow("Chat")
---   </pre>
---
--- @see createMiniConsole
function openUserWindow(windowName) end



--- Pastes the previously copied text including all format codes like color, font etc. at the current user
--- cursor position. The copy() and paste() functions can be used to copy formated text from the main window
--- to a user window without losing colors e. g. for chat windows, map windows etc.
---
--- @see copy
function paste(windowName) end



--- <b><u>TODO</u></b>  pasteWindow - TLuaInterpreter::pasteWindow
function pasteWindow() end



--- Creates a persistent alias that stays after Mudlet is restarted and shows up in the Script Editor. <br/><br/>
---
--- Note that Mudlet by design allows duplicate names - so calling permAlias with the same name will keep creating
--- new aliases. You can check if an alias already exists with the exists() function.
---
--- @param name is the name you'd like the alias to have.
--- @param parent is the name of the group, or another alias you want the trigger to go in - however if such a group/alias
--- doesn't exist, it won't do anything. Use "" to make it not go into any groups.
--- @param regex is the pattern that you'd like the alias to use.
--- @param luaCode is the script the alias will do when it matches.
---
--- @usage Creates an alias called "new alias" in a group called "my group".
---   <pre>
---   permAlias("new alias", "my group", "^test$", [[echo ("say it works! This alias will show up in the script editor too.")]])
---   </pre>
---
--- @see exists
function permAlias(name, parent, regex, luaCode) end



--- Creates a persistent trigger with a begin of line substring pattern that shows up in the Script Editor
--- and stays after Mudlet is restarted. <br/>
--- <br/>
--- Note that Mudlet by design allows duplicate names - so calling permBeginOfLineStringTrigger with the same
--- name will keep creating new triggers. You can check if a trigger already exists with the exists() function.
---
--- @param name is the name you'd like the trigger to have.
--- @param parent is the name of the group, or another trigger you want the trigger to go in - however
---   if such a group/trigger doesn't exist, it won't do anything. Use "" to make it not go into any groups.
--- @param patternTable is a table of patterns that you'd like the trigger to use - it can be one or many.
--- @param luaCode is the script the trigger will do when it matches.
---
--- @usage Create a trigger that will match on anything that starts with "You sit" and do "stand".
---   It will not go into any groups, so it'll be on the top.
---   <pre>
---   permBeginOfLineStringTrigger("Stand up", "", {"You sit"}, [[send ("stand")]])
---   </pre>
--- @usage Another example - lets put our trigger into a "General" folder and give it several patterns.
---   <pre>
---   permBeginOfLineStringTrigger("Stand up", "General", {"You sit", "You fall", "You are knocked over by"}, [[send ("stand")]])
---   </pre>
function permBeginOfLineStringTrigger(name, parent, patternTable, luaCode) end



--- Creates a persistent trigger with a regex pattern that stays after Mudlet is restarted and shows up in the
--- Script Editor. <br/><br/>
---
--- Note that Mudlet by design allows duplicate names - so calling permRegexTrigger with the same name will keep creating
--- new aliases. You can check if an alias already exists with the exists() function.
---
--- @param name is the name you'd like the alias to have.
--- @param parent is the name of the group, or another alias you want the trigger to go in - however if such a group/alias
---   doesn't exist, it won't do anything. Use "" to make it not go into any groups.
--- @param regex is the pattern that you'd like the alias to use.
--- @param luaCode is the script the alias will do when it matches.
---
--- @usage Create a regex trigger that will match on the prompt to record your status.
---   <pre>
---   permRegexTrigger("Prompt", "", {"^(\d+)h, (\d+)m"}, [[health = tonumber(matches[2]; mana = tonumber(matches[3])]]
---   </pre>
---
--- @see exists
function permRegexTrigger(name, parent, pattern, luaCode) end



--- Creates a persistent trigger with a substring pattern that stays after Mudlet is restarted and shows up in
--- the Script Editor. <br/><br/>
---
--- Note that Mudlet by design allows duplicate names - so calling permSubstringTrigger with the same name will keep creating
--- new aliases. You can check if an alias already exists with the exists() function.
---
--- @param name is the name you'd like the alias to have.
--- @param parent is the name of the group, or another alias you want the trigger to go in - however if such a group/alias
---   doesn't exist, it won't do anything. Use "" to make it not go into any groups.
--- @param regex is the pattern that you'd like the alias to use.
--- @param luaCode is the script the alias will do when it matches.
---
--- @usage Create a trigger to highlight the word "pixie" for us.
---   <pre>
---   permSubstringTrigger("Highlight stuff", "General", {"pixie"}, [[selectString(line, 1) bg("yellow") resetFormat()]])
---   </pre>
--- @usage Another trigger to highlight several different things.
---   <pre>
---   permSubstringTrigger("Highlight stuff", "General", {"pixie", "cat", "dog", "rabbit"}, [[selectString(line, 1) fg ("blue") bg("yellow") resetFormat()]])
---   </pre>
---
--- @see exists
function permSubstringTrigger(name, parent, pattern, luaCode) end



--- Creates a persistent timer that stays after Mudlet is restarted and shows up in the Script Editor. <br/><br/>
---
--- Note that Mudlet by design allows duplicate names - so calling permTimer with the same name will keep creating
--- new timers. You can check if a timer already exists with the exists() function.
---
--- @param name timer name, parent is the name of the timer group you want the timer to go in.
--- @param seconds number specifying a delay after which the timer will execute the lua code.
--- @param luaCode code to execute
---
--- @usage Creates new time that will tick each 4.5s.
---   <pre>
---   permTimer("my timer", "first timer group", 4.5, [[send ("my timer that's in my first timer group fired!")]])
---   </pre>
function permTimer(name, parent, seconds, luaCode) end



--- This function plays a sound file. To make sound work on your operating system you may need to install additional packages: <br/>
--- Microsoft Windows: The underlying multimedia system is used; only WAVE format sound files are supported. (works out of the box) <br/>
--- Mac OS X: NSSound is used. All formats that NSSound supports, including QuickTime formats, are supported by Qt for Mac OS X
--- (should work out of the box). <br/>
--- X11: The Network Audio System is used if available, otherwise all operations work silently. NAS supports WAVE and AU files. Please use
--- following workaround for Linux systems:
---   <pre>
---   if "linux" == getOS() then
---	     os.execute("aplay /usr/share/sounds/alsa/Front_Center.wav")
---   end
---   </pre>
function playSoundFile(fileName) end



--- Raises the event event_name. The event system will call the main
--- function (the one that is called exactly like the script name) of all
--- such scripts that have registered event handlers. If an event is raised,
--- but no event handler scripts have been registered with the event system,
--- the event is ignored and nothing happens. This is convenient as you can
--- raise events in your triggers, timers, scripts etc. without having to
--- care if the actual event handling has been implemented yet - or more
--- specifically how it is implemented. Your triggers raise an event to tell
--- the system that they have detected a certain condition to be true or
--- that a certain event has happened. How - and if - the system is going to
--- respond to this event is up to the system and your trigger scripts don't
--- have to care about such details. For small systems it will be more
--- convenient to use regular function calls instead of events, however, the
--- more complicated your system will get, the more important events will
--- become because they help reduce complexity very much. <br/><br/>
---
--- The corresponding event handlers that listen to the events raised with
--- raiseEvent() need to use the script name as function name and take the
--- correct number of arguments. NOTE: If you raise an event with 5
--- arguments but your event handlers functions only take 2,10 or 0
--- arguments, the functions will not be called. For example:
--- raiseEvent("fight") a correct event handler function would be:
--- myScript(event_name). In this example raiseEvent uses minimal arguments, name
--- the event name. There can only be one event handler function per script,
--- but a script can still handle multiple events as the first argument is
--- always the event name. So you can call your own special handlers for
--- individual events. The reason behind this is that you should rather use
--- many individual scripts instead of one huge script that has all your
--- function code etc. Scripts can be organized very well in trees and thus
--- help reduce complexity on large systems. <br/><br>
---
--- As an example, your prompt trigger could raise an onPrompt event if you want to attach 2 functions to it.
--- In your prompt trigger, all you'd need to do is raiseEvent("onPrompt") Now we go about creating functions
--- that attach to the event. Lets say the first one is check_health_stuff() and the other is check_salve_stuff().
--- We would like these to be executed when the event is raised. So create a script and give it a name of check_health_stuff.
--- In the Add user defined event handler, type onPrompt, and press enter to add it to the list. In the script box,
--- create: function check_health_stuff()blah blah end. When the onPrompt event comes along, that script catches it,
--- and does check_health_stuff() for you. <br/><br/>
---
--- <b>Default events raised by Mudlet</b><br/>
--- Mudlet itself also creates events for your scripts to hook on. The following events are generated currently:<br/>
---
--- <b><i>sysWindowResizeEvent</i></b><br/>
--- Raised when the main window is resized, with the new height and width coordinates passed to the event.
--- A common usecase for this event is to move/resize your UI elements according to the new dimensions.
--- This sample code will echo whenever a resize happened with the new dimensions:
---   <pre>
---   function resizeEvent( event, x, y )
---      echo("RESIZE EVENT: event="..event.." x="..x.." y="..y.."\n")
---   end
---   </pre>
---
--- <b><i>sysWindowMousePressEvent</i></b><br/>
--- Raised when a mouse button is pressed down anywhere on the main window (note that a click is composed of a mouse
--- press and mouse release). The button number and the x,y coordinates of the click are reported.
---   <pre>
---   function onClickHandler( event, button, x, y )
---      echo("CLICK event:"..event.." button="..button.." x="..x.." y="..y.."\n")
---   end
---   </pre>
---
--- <b><i>sysWindowMouseReleaseEvent</i></b><br/>
--- Raised when a mouse button is released after being pressed down anywhere on the main window (note that a click is composed
--- of a mouse press and mouse release). See sysWindowMousePressEvent for example use.
---
--- <b><i>ATCP events</i></b><br/><br/>
--- Mudlets ATCP implementation generates events for each message that comes, allowing you to trigger on them easily.
--- Since ATCP messages vary in name, event names will vary as well. See the atcp section on how to use them.
function raiseEvent(eventName, ...) end



--- Reconnect to currect session.
---
--- @see disconnect
function reconnect() end



--- Replaces the currently selected text with the new text. To select text, use selectString() and similar function.
---
--- @usage Replace word "troll" with "cute trolly".
---   <pre>
---   selectString("troll",1)
---   replace("cute trolly")
---   </pre>
---
--- @usage Lets replace the whole line. If you'd like to delete/gag the whole line, use deleteLine()!
---   <pre>
---   selectString(line, 1)
---   replace("Out with the old, in with the new!")
---   </pre>
---
--- @see selectString
--- @see deleteLine
function replace(with) end



--- Resets the character format to default. This should be used after you have highlighted some text
--- or changed the current foreground or background color, but you don't want to keep using these
--- colors for further prints. If you set a foreground or background color, the color will be used
--- until you call resetFormat() on all further print commands.
---
--- @param windowName optional
function resetFormat(windowName) end



--- This function resets the time to 0:0:0.0, but does not start the stop watch. You can start it with startStopWatch.
---
--- @see createStopWatch
--- @see startStopWatch
function resetStopWatch(watchID) end



--- Resizes a mini console or label.
---
--- @see createMiniConsole
--- @see createLabel
--- @see handleWindowResizeEvent
--- @see setBorderTop
function resizeWindow(name, width, height) end



--- Selects the content of the capture group number in your Perl regular expression e.g. "you have (\d+) Euro".
--- If you want to color the amount of money you have green you do:
---
--- @usage
---   <pre>
---   setFgColor(0,255,0)
---   selectCaptureGroup(1)
---   </pre>
---
--- @param groupNumber with first group = 1
function selectCaptureGroup() end



--- Selects the content of the current buffer line.
--- <pre><b><u>TODO</u></b> It this valid? selectCurrentLine("sys")</pre>
function selectCurrentLine() end



--- Select text on the line under the current cursor position. Use absolute column number for start of selection
--- and length of selection The function returns true on success and false if the selection is not possible.
---
--- @param windowName is optional
function selectSection(windowName, from, lengthOfString) end



--- Selects a substring from the line where the user cursor is currently positioned. You can move
--- the user cursor with moveCursor(). When a new line arrives from the MUD, the user cursor is
--- positioned at the beginning of the line. However, if one of your trigger scripts moves the
--- cursor around you need to take care of the cursor position yourself and make sure that the
--- cursor is in the correct line if you want to call one of the select functions. To deselect
--- text, see deselect().
---
--- @usage Select "big monster" in the line.
---   <pre>
---   selectString("big monster", 1)
---   </pre>
--- @usage Note: To prevent selection of random data use the error return if not found like this.
---   <pre>
---   if selectString("big monster", 1) > -1 then
---      setFgColor(255,0,0)
---   end
---   </pre>
--- @usage In a trigger, lets color all words on the current line green.
---   <pre>
---   selectString(line, 1)
---   fg("green")
---   resetFormat()
---   </pre>
---
--- @return returns position in line or -1 on error (text not found in line)
---
--- @see deselect
function selectString(text, numberOfMatch) end



--- This sends "command" directly to the network layer, skipping the alias matching. The optional
--- second argument of type boolean (print) determines if the outgoing command is to be echoed on the screen.
--- Send honours command separator defined within Mudlet settings. If you want your command to be checked
--- if it's an alias, use expandAlias() instead.
---
--- @usage Echos the command on the screen.
---   <pre>
---   send("Hello Jane")
---   </pre>
--- @usage Echos the command on the screen.
---   <pre>
---   send("Hello Jane", true)
---   </pre>
--- @usage Does not echo the command on the screen.
---   <pre>
---   send("Hello Jane", false)
---   </pre>
---
--- @param echoTheValue optional boolean flag (default value is true) which determine if value should
---   be echoed back on client.
---
--- @see expandAlias
--- @see sendAll
function send(command, echoTheValue) end



--- <b><u>TODO</u></b>  sendATCP - TLuaInterpreter::sendATCP
--- @see atcp
function sendATCP() end



--- Send telnet channel 102 commands to MUD. This is widely used on Aardwolf for setting up user tags.
---
--- @usage Following command will enable all Aardwolf channel tags. You need to have active session.
---   <pre>
---   sendTelnetChannel102("\5\1")
---   </pre>
---
--- @see channel102
function sendTelnetChannel102() end



--- Sets RGB color values and the transparency for the given window. Colors are from 0 to 255 (0 being black),
--- and transparency is from - to 255 (0 being completely transparent). <br/><br/>
---
--- Note that transparency only works on labels, not miniConsoles for efficiency reasons.
function setBackgroundColor(windowName, red, green, blue, transparency) end



-- Loads an image file (png) as a background image for a label. This can be used to display clickable images etc.
function setBackgroundImage(labelName, imageFileName) end



--- Sets the current text background color in window windowName (or in main windows if you haven't specified that). If you have selected
--- text prior to this call, the selection will be highlightd otherwise the current text background color will be changed. If you set a
--- foreground or background color, the color will be used until you call resetFormat() on all further print commands.
---
--- @param windowName optional
---
--- @usage Highlights the first occurrence of the string "Tom" in the current line with a red background color.
---   <pre>
---   selectString( "Tom", 1 )
---   setBgColor( 255,0,0 )
---   </pre>
---
--- @usage Prints "Hello" on red background and "You" on blue.
---   <pre>
---   setBgColor(255,0,0)
---   echo("Hello")
---   setBgColor(0,0,255)
---   echo(" You!")
---   resetFormat()
---   </pre>
---
--- @see setFgColor
function setBgColor(windowName, r, g, b) end



--- Sets the current text font to bold (true) or non-bold (false) mode. If the windowName parameters omitted, the main screen will be used.
---
--- @param windowName optional
function setBold(windowName, bool) end



--- Sets the height of the bottom border to size pixel and thus effectively moves down the main console
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
---
--- @see setBorderTop
--- @see setBorderLeft
--- @see setBorderRight
function setBorderBottom(size) end



--- Sets the color of the border in RGB color.
---
--- @usage Sets the border to red.
---   <pre>
---   setBorderColor( 255, 0, 0 )
---   </pre>
function setBorderColor(red, green, blue) end



--- Sets the width of the left border and thus effectively moves down the main console
--- window by size pixels to make room for e.g. mini console windows, buttons etc..

--- @see setBorderTop
--- @see setBorderBottom
--- @see setBorderRight
function setBorderLeft(size) end



--- Sets the width of the right border and thus effectively moves down the main console
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
---
--- @see setBorderTop
--- @see setBorderBottom
--- @see setBorderLeft
function setBorderRight(size) end



--- Sets the height of the top border to size pixel and thus effectively moves down the main console
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
---
--- @see setBorderBottom
--- @see setBorderLeft
--- @see setBorderRight
function setBorderTop(size) end



--- Set the scrollback buffer size to linesLimit and determine how many lines are deleted at once in
--- case the lines limit is reached. The lower the limit the less memory being used. On machines with
--- low RAM you should consider limiting the size of buffers that don't need a lot of scrollback
--- e.g. system notification windows, chat windows etc.. <br/>
--- Default values are linesLimit = 100000 lines with 10000 lines of batch deletion. <br/>
--- Minimum buffer size is 100 lines with 10 lines batch deletion.
function setConsoleBufferSize(consoleName, linesLimit, sizeOfBatchDeletion) end



--- Sets the current text foreground color in the main window. Values are RGB: red, green, blue ranging from 0-255 e.g.
---
--- @param windowName optional
---
--- @usage Set blue foreground.
---   <pre>
---   setBgColor(0,0,255)
---   </pre>
--- @param windowName optional
---
--- @setBgColor
function setFgColor(windowName, r, g, b) end



--- Sets the current text font to italics/non-italics mode. If the windowName parameters omitted, the main screen will be used.
---
--- @param windowName optional
function setItalics(windowName, bool) end



--- Specify a Lua function to be called if the user clicks on the
--- label/image. E.g. setLabelClickCallback( "compassNorthImage", "onClickGoNorth" ) <br/><br/>
---
--- UPDATE: this function can now pass any number of string or integer number values as additional parameters.
--- These parameters are then used in the callback. Thus you can associate data with the label/button.
--- Check the forum for more information on how to use this. <b>FIXME</b>
function setLabelClickCallback(labelName, luaFunctionName, ...) end



--- <b><u>TODO</u></b>  setLabelStyleSheet - TLuaInterpreter::setLabelStyleSheet
function setLabelStyleSheet() end



--- Turns the selected text into a clickable link - upon being clicked, the link will do the command code.
--- Tooltip is a string which will be displayed when the mouse is over the selected text.
---
--- @release Mudlet 1.1.0-pre1
---
--- @usage In a sewer grate substring trigger, the following code will make clicking on the words do the send("enter grate") command:
---   <pre>
---   selectString(matches[1], 1)
---   setLink([[send("enter grate")]], "Clicky to enter grate")
---   </pre>
---
--- @see echoLink
--- @see insertLink
function setLink(command, tooltip) end



--- Sets the font size of the mini console.
---
--- @see createMiniConsole
--- @see createLabel
function setMiniConsoleFontSize(name, fontSize) end



--- Turns the selected text into a left-clickable link, and a right-click menu. The selected text,
--- upon being left-clicked, will do the first command in the list. Upon being right-clicked, it'll
--- display a menu with all possible commands. The menu will be populated with hints, one for each line.
---
--- @release Mudlet 1.1.0-pre1
---
--- @usage In a Raising your hand in greeting, you say "Hello!" exact match trigger, the following code will
---   make left-clicking on Hello show you an echo, while left-clicking will show some commands you can do.
---   <pre>
---   selectString("Hello", 1)
---   setPopup("main", {[[send("bye")]], [[echo("hi!")]]}, {"left-click or right-click and do first item to send bye", "click to echo hi"})
---   </pre>
---
--- @param name the name of the console to operate on. If not using this in a miniConsole, use "main" as the name.
--- @param commands a table of lua code strings to do e.g. {[[send("hello")]], [[echo("hi!"]]}.
--- @param hints a table of strings which will be shown on the popup and right-click menu e.g. {"send the hi command", "echo hi to yourself"}.
function setPopup(name, commands, hints) end



--- Sets current text format of window. A more convenient way to control the text format in a mini console is to
--- use setFgColor, setBold, setItalics, setUnderline etc.
---
--- @param windowName
--- @param fgR foreground color(r,g,b)
--- @param fgG foreground color(r,g,b)
--- @param fgB foreground color(r,g,b)
--- @param bgR background color(r,g,b)
--- @param bgG background color(r,g,b)
--- @param bgB background color(r,g,b)
--- @param bold bold flag (1/0)
--- @param underline underline flag (1/0)
--- @param italics italics flag (1/0)
---
--- @usage This script would create a mini text console and write with yellow foreground color and blue background color "This is a test".
---   <pre>
---   createMiniConsole("con1", 0, 0, 300, 100)
---   setTextFormat("con1", 0,0,255, 255,255,0, 1,1,1)
---   echo("con1", "This is a test")
---   </pre>
---
--- @see createMiniConsole
--- @see setBold
--- @see setBgColor
--- @see setFgColor
--- @see setItalics
--- @see setUnderline
function setTextFormat(windowName, fgR, fgG, fgB, bgR, bgG, bgB, bold, underline, italics) end



--- Set for how many more lines a trigger script should fire or a chain should stay open after the trigger has matched.
--- The main use of this function is to close a chain when a certain condition has been met.
---
--- @param number should be 0 to close the chain, or a positive number to keep the chain open that much longer.
function setTriggerStayOpen(name, number) end



--- Sets the current text font to underline/non-underline mode. If the windowName parameters omitted, the main screen will be used.
---
--- @param windowName optional
function setUnderline(windowName, bool) end



--- Sets at what position in the line the console or miniconsole will start word wrap.
function setWindowWrap(windowName, wrapAt) end



--- <b><u>TODO</u></b>  setWindowWrapIndent - TLuaInterpreter::setWindowWrapIndent
function setWindowWrapIndent() end



--- Shows tool bar name on the screen.
function showToolBar(name) end



--- This function shows a mini console or label. To hide it use hideWindow.
--- @see hideWindow
--- @see createMiniConsole
--- @see createLabel
function showWindow(name) end



--- <b><u>TODO</u></b>  spawn - TLuaInterpreter::spawn
function spawn() end



--- <b><u>TODO</u></b>  startLogging - TLuaInterpreter::startLogging
function startLogging() end



--- Starts the stop watch.
--- @see createStopWatch
function startStopWatch(watchID) end



--- Stops the stop watch and returns the elapsed time in milliseconds in form of 0.001.
--- @see createStopWatch
--- @return returns time as a number
function stopStopWatch() end



--- Creates a temporary (lasts only until the profile is closed) alias. This means that it won't exist anymore after Mudlet restarts.
---
--- @usage This triggers waits for "hi" string.
---   <pre>
---   tempAlias("^hi$", [[send ("hi") echo ("we said hi!")]]
---   </pre>
---
--- @return id
function tempAlias(regex, luaCode) end



--- Makes a color trigger that triggers on the specified foreground and background color. Both colors need to be
--- supplied in form of these simplified ANSI 16 color mode codes:
---
---   <pre>
---   0 = default text color
---   1 = light black
---   2 = dark black
---   3 = light red
---   4 = dark red
---   5 = light green
---   6 = dark green
---   7 = light yellow
---   8 = dark yellow
---   9 = light blue
---   10 = dark blue
---   11 = light magenta
---   12 = dark magenta
---   13 = light cyan
---   14 = dark cyan
---   15 = light white
---   16 = dark white
---   </pre>
---
--- @usage This script will re-highlight all text in blue foreground colors on a black background with a red foreground
---   color on a blue background color until another color in the current line is being met. temporary color triggers
---   do not offer match_all or filter options like the GUI color triggers because this is rarely necessary for scripting.
---   A common usage for temporary color triggers is to schedule actions on the basis of forthcoming text colors
---   in a particular context.
---   <pre>
---   tempColorTrigger(9,2,[[selectString(matches[1],1); fg("red"); bg("blue");]] );
---   </pre>
function tempColorTrigger(foregroundColor, backgqroundColor, luaCode) end



--- <b><u>TODO</u></b> example with luaCode -
--- Temporary trigger that will fire on n consecutive lines following the
--- current line. This is useful to parse output that is known to arrive in
--- a certain line margin or to delete unwanted output from the MUD.
---
--- @return the string ID of the newly created temporary trigger.
---   You can use this ID to enable/disable or kill this trigger later on.
---
--- @usage Following will fire 3 times starting with the line from the MUD.
---   <pre>
---   tempLineTrigger( 1, 3, [[send("shout Help")]])
---   </pre>
--- @usage This will fire 20 lines after the current line and fire twice on 2 consecutive lines.
---   <pre>
---   tempLineTrigger( 20, 2, [[send("shout Help")]])
---   </pre>
function tempLineTrigger(from, howMany, luaCode) end



--- Temporary trigger using Perl regex pattern matching.
---
--- @return the string ID of the newly created temporary trigger.
---   You can use this ID to enable/disable or kill
---   this trigger later on.
---
--- @see killTrigger
--- @see tempTrigger
function tempRegexTrigger(regex, luaCode) end



--- Creates a temporary single shot timer and returns the timer ID for subsequent enableTimer(), disableTimer() and killTimer() calls.
--- You can use 2.3 seconds or 0.45 etc. After it has fired, the timer will be deactivated and killed. <br/><br/>
---
--- Note [[ ]] can be used to quote strings in Lua. The difference to the usual `" " quote syntax is that `[[ ]] also accepts the
---   character ". Consequently, you don't have to escape the " character in the above script. The other advantage is that it can be
---   used as a multiline quote.
---
--- @usage This script will send the command "kill rat" 0.3 seconds after this function has been called. Note that this function
---   does not wait until 0.3 seconds have been passed, but it will start a timer that will run the Lua script that you have provided
---   as a second argument after 0.3 seconds.
---   <pre>
---   tempTimer(0.3, [[send("kill rat")]] )
---   </pre>
---
--- @usage Also note that the Lua code that you provide as an argument is compiled from a string value when the timer fires.
---   This means that if you want to pass any parameters by value e.g. you want to make a function call that uses the value
---   of your variable myGold as a parameter you have to do things like this:
---   <pre>
---   tempTimer(3.8, [[echo("at the time of the tempTimer call I had ]] .. myGold .. [[ gold.")]] )
---   </pre>
---
--- @return timer ID, ID is a string and not a number.
function tempTimer(seconds, luaCode) end



--- This function creates a temporary trigger using substring matching.
--- Contrary to tempTimers, tempTriggers lives throughout the entire session
--- until it is explicitly disabled or killed. Disabled tempTimers can be
--- re-enabled with enableTrigger(). This is much faster than killing the
--- trigger and creating a new one. This is the second fastest trigger (with
--- begin of line substring patterns being the fastest) and should be used
--- instead of regex triggers whenever possible.
---
--- @return trigger ID, ID is a string and not a number.
---
--- @usage You can put the following script into your targeting alias highlight your target. <br/>
---   (Note: trigger will stay active unless you'll kill it.)
---   <pre>
---   target = "rat"
---   if id then
---      killTrigger(id)
---   end
---   id = tempTrigger(target, [[selectString("]] .. target .. [[", 1) fg("gold") resetFormat()]])
---   </pre>
---
--- @see killTrigger
--- @see tempRegexTrigger
function tempTrigger(string, luaCode) end



--- Wait for specified time in milliseconds.
--- <b>Use tempTimer instead! Don't use this function, because it freezes main thread.</b>
---
--- @usage Preferred use of tempTimer - wait for 2 seconds and than send "kill rat".
---   <pre>
---   tempTimer(2, [[send("kill rat")]] )
---   </pre>
--- @usage <b>Discouraged</b> use of wait function.
---   <pre>
---   -- This will freeze Mudlet for 2 seconds!!!
---   wait(2000)
---   send("kill rat")
---   </pre>
--- @usage This example is demonstrating transition from Nexus/Zmud wait.
---   You can simply rewrote following nexus/zmud code bellow with <b>tempTimers</b>.
---   <pre>
---   #send jerk fish
---   #wait 1500
---   #send pull line
---   #wait 500
---   #send jump
---   </pre>
---   Since timers are created instantly, if you want two or more, or means the times
---   for consecutive timers should be to the starting point, unlike relatives times you do with waits.
---   <pre>
---   -- Mudlet code
---   send ("jerk fish")
---   tempTimer (1.5, [[send ("pull line")]])
---   tempTimer (2,   [[send ("jump")]])
---   </pre>
---
--- @see tempTimer
function wait(time) end



--- Wrap line lineNumber of mini console (window) windowName. This function will interpret \n characters,
--- apply word wrap and display the new lines on the screen. This function may be necessary if you use
--- deleteLine() and thus erase the entire current line in the buffer, but you want to do some further
--- echo() calls after calling deleteLine(). You will then need to re-wrap the last line of the buffer
--- to actually see what you have echoed and get you \n interpreted as newline characters properly. <br/><br/>
---
--- Using this function is not good programming practice and should be avoided. There are better ways of
--- handling situations where you would call deleteLine() and echo afterwards e.g.:
---   <pre>
---   selectString(line,1)
---   replace("")
---   </pre>
--- This will effectively have the same result as a call to deleteLine() but the buffer line will not
--- be entirely removed. Consequently, further calls to echo() etc. sort of functions are possible
--- without using wrapLine() unnecessarily.
function wrapLine() end




end
