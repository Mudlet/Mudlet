
--
-- This file hold LuaDoc for all function implemented with in Mudlet Core.
-- It allows us to generate single API documentaion from all sources.
-- (Used   grep -R  lua_register *   for getting initial function list.)
--

-- ensure that those function will not get defined
if false then



--- Wait for specified time in milliseconds.
--- @usage wait(2000) <br/>
---   wait for 2 seconds
function wait(time) end


--- Like send(), but without bypassing alias expansion. This function may lead to infinite recursion if 
--- you are not careful. This function can be used to make recursive alias expansion. expandAlias("Hello Tom") 
--- echos the command on the screen expandAlias("Hello Jane") sends the command visually unnoticeable 
--- by the user send. <br/>
--- <br/>
--- Now, while in Mudlet you can call another alias with the expandAlias() function, this is strongly 
--- discouraged. What you do instead if create a function (for example, send()  and echo()  are functions) 
--- that you can then call from your alias or trigger. This has many advantages - it's faster, you can 
--- easily give your function values, and your function can return you values, too.<br/>
--- <br/>
--- Note: The variable "command" contains what was entered in the command line or issued via the expandAlias() 
--- function. If you use expandAlias( command ) inside an alias script the command would be doubled. You have 
--- to use send( ) inside an alias script to prevent recursion. This will send the data directly and bypass 
--- the alias expansion.
---
--- @see send
function expandAlias(command, print=1) end


--- This function appends text at the end of the current line. The current cursor position is ignored. 
--- Use moveCursor() and insertText() if you want to print at a different cursor position. <br/>
--- If the first argument is omitted the main console is used, otherwise the mini console windowName.
---
--- @usage echo("Hello world\n")
--- @usage echo("info", "Hello this is the info window\n") <br/>
---   -- writes text to the mini console named "info" if such a window exists
function echo(windowName, text) end


--- Selects a substring from the line where the user cursor is currently positioned. You can move 
--- the user cursor with moveCursor(). When a new line arrives from the MUD, the user cursor is 
--- positioned at the beginning of the line. However, if one of your trigger scripts moves the 
--- cursor around you need to take care of the cursor position yourself and make sure that the 
--- cursor is in the correct line if you want to call one of the select functions. To deselect 
--- text, see deselect().
---
--- @usage selectString("big monster", 1)
--- @usage if selectString("big monster", 1) > -1 then setFgColor(255,0,0) end <br>
---   Note: To prevent selection of random data use the error return if not found like this.
---
--- @return returns position in line or -1 on error (text not found in line)
---
--- @see deselect
function selectString(text, numberOfMatch) end


--- Select text on the line under the current cursor position. Use absolute column number for start of selection 
--- and length of selection The function returns true on success and false if the selection is not possible.
---
--- @param windowName is optional
function selectSection(windowName, from, lengthOfString) end


--- TODO  replace - TLuaInterpreter::replace
function replace() end


--- TODO  setBgColor - TLuaInterpreter::setBgColor
function setBgColor() end


--- TODO  setFgColor - TLuaInterpreter::setFgColor
function setFgColor() end


--- TODO  tempTimer - TLuaInterpreter::tempTimer
function tempTimer() end


--- TODO  tempTrigger - TLuaInterpreter::tempTrigger
function tempTrigger() end


--- TODO  tempRegexTrigger - TLuaInterpreter::tempRegexTrigger
function tempRegexTrigger() end


--- Opens a user dockable console window for user output e.g. statistics, chat etc. If a window of such 
--- a name already exists, nothing happens. You can move these windows, dock them, make them into notebook 
--- tabs or float them. Most often a mini console is more useful than a dockable window. 
---
--- @see createMiniConsole
function openUserWindow() end


--- TODO  echoUserWindow - TLuaInterpreter::echoUserWindow
function echoUserWindow() end


--- TODO  enableTimer - TLuaInterpreter::enableTimer
function enableTimer() end


--- TODO  disableTimer - TLuaInterpreter::disableTimer
function disableTimer() end


--- TODO  enableKey - TLuaInterpreter::enableKey
function enableKey() end


--- TODO  disableKey - TLuaInterpreter::disableKey
function disableKey() end


--- Clears the user window or a mini console with the name given as argument.
function clearUserWindow(windowName) end


--- TODO  clearWindow - TLuaInterpreter::clearUserWindow
function clearWindow(windowName) end


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


--- TODO  moveCursor - TLuaInterpreter::moveCursor
function moveCursor() end


--- Returns a Lua table with the content of the lines on a per line basis. Absolute line numbers are used.
--- @return section of the content of the screen text buffer. The form of the return value is: Lua_table[relative_linenumber, content] 
function getLines(from_line_number, to_line_number) end


--- Gets the absolute line number of the current user cursor.
function getLineNumber() end


--- TODO  insertHTML - TLuaInterpreter::insertHTML
function insertHTML() end


--- TODO  insertText - TLuaInterpreter::insertText
function insertText() end


--- TODO  enableTrigger - TLuaInterpreter::enableTrigger
function enableTrigger() end


--- TODO  disableTrigger - TLuaInterpreter::disableTrigger
function disableTrigger() end


--- Tells you how many things of the given type exist. 
--- @usage echo("I have " .. exists("my trigger", "trigger") .. " triggers called 'my trigger'!")
--- @usage You can also use this alias to avoid creating duplicate things, for example: <br/>
--- 
--- -- this code doesn't check if an alias already exists and will keep creating new aliases <br/>
--- permAlias("Attack", "General", "^aa$", [[send ("kick rat")]]) <br/>
---  <br/>
--- -- while this code will make sure that such an alias doesn't exist first <br/>
--- -- we do == 0 instead of 'not exists' because 0 is considered true in Lua <br/>
--- if exists("Attack", "alias") == 0 then <br/>
---     permAlias("Attack", "General", "^aa$", [[send ("kick rat")]]) <br/>
--- end <br/>
--- 
--- @param type can be "alias", "trigger", or "timer".
--- @see isActive
--- @return number
function exists(name, type) end


--- Deletes a tempTrigger according to trigger ID. ID is a string value, not a number.
---
--- @see killAlias
--- @see killTimer
---
--- @return true or false
function killTrigger(id) end


--- Gets the absolute amount of lines in the current console buffer.
--- @return number
function getLineCount() end


--- Gets the absolute column number of the current user cursor.
function getColumnNumber() end


--- This sends "command" directly to the network layer, skipping the alias matching. The optional 
--- second argument of type boolean (print) determines if the outgoing command is to be echoed on the screen.
--- If you want your command to be checked if it's an alias, use expandAlias() instead.
---
--- @usage send("Hello Jane") <br/>
---   --echos the command on the screen
--- @usage send("Hello Jane", true) <br/> 
---   --echos the command on the screen
--- @usage send("Hello Jane", false) <br/>
---   --does not echo the command on the screen
---
--- @param echoTheValue true or false, default value is true
---
--- @see expandAlias
function send(command, echoTheValue) end


--- Selects the content of the capture group number in your Perl regular expression e.g. "you have (\d+) Euro". 
--- If you want to color the amount of money you have green you do: 
---
--- @usage selectCaptureGroup(1); <br/>
---   setFgColor(0,255,0)
--- @param groupNumber with first group = 0 <br/>
---   TODO this is probably wrong - first group = 1
function selectCaptureGroup() end


--- TODO  tempLineTrigger - TLuaInterpreter::tempLineTrigger
function tempLineTrigger() end


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
--- respond to this event is up to the system and your trigger scripts don’t 
--- have to care about such details. For small systems it will be more 
--- convenient to use regular function calls instead of events, however, the 
--- more complicated your system will get, the more important events will 
--- become because they help reduce complexity very much. <br/>
--- <br/>
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
--- help reduce complexity on large systems. 
function raiseEvent(eventName, ...) end


--- Deletes the current Line under the user cursor. Note: This is a high speed gagging tool 
--- and it is very good at this task. It is meant to be used when the line can be omitted 
--- entirely in the output. If you want to replace this line with something else have a look 
--- at the replace() functions below. <br/>
--- Note that scripts such as: deleteLine(); echo("this line is gone"); will not work because 
--- lines that have been gagged with deleteLine() will not be rendered even if there is text 
--- in the buffer. See wrapLine for details on how to force a re-render if this is necessary 
--- for some reason. This is not the recommended way of replacing text.
---
--- @see wrapLine
--- @see replace
function deleteLine() end


--- Copies the current selection to the clipboard. This function operates on rich text, i.e. 
--- the selected text including all its format codes like colors, fonts etc. in the clipboard 
--- until it gets overwritten by another copy operation. example: This script copies the current 
--- line on the main screen to a user window (mini console) named chat and gags the output on 
--- the main screen.
---
--- @usage selectString(line); <br/>
---   copy(); <br/>
---   appendBuffer("chat"); <br/>
---   replace("This line has been moved to the chat window!")
---
--- @see paste
--- @see appendBuffer
function copy() end


--- TODO  cut - TLuaInterpreter::cut
function cut() end


--- Pastes the previously copied text including all format codes like color, font etc. at the current user 
--- cursor position. The copy() and paste() functions can be used to copy formated text from the main window 
--- to a user window without losing colors e. g. for chat windows, map windows etc.
---
--- @see copy
function paste(windowName) end


--- TODO  pasteWindow - TLuaInterpreter::pasteWindow
function pasteWindow() end


--- Sets at what position in the line the console or miniconsole will start word wrap.
function setWindowWrap(windowName, wrapAt) end


--- TODO  setWindowWrapIndent - TLuaInterpreter::setWindowWrapIndent
function setWindowWrapIndent() end


--- Resets the character format to default. This should be used after you have highlighted some text 
--- or changed the current foreground or background color, but you don't want to keep using these 
--- colors for further prints. If you set a foreground or background color, the color will be used 
--- until you call resetFormat() on all further print commands.
function resetFormat() end


--- Moves the cursor to the end of the buffer. "main" is the name of the main window, otherwise use the name of your user window.
--- @see moveCursor
--- @return true or false
function moveCursorEnd(windowName) end


--- Returns number of the last line in the text buffer.
function getLastLineNumber() end


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
--- @usage Set up the small system message window in the top right corner <br/>
---   -- determine the size of your screen <br/>
---   WindowWidth = 0; <br/>
---   WindowHeight = 0; <br/>
---   WindowWidth, WindowHeight = getMainWindowSize(); <br/>
---   <br/>
---   createMiniConsole("sys",WindowWidth-650,0,650,300) <br/>
---   setBackgroundColor("sys",85,55,0,255) <br/>
---   setMiniConsoleFontSize("sys", 8) <br/>
---   -- wrap lines in window "sys" at 65 characters per line <br/>
---   setWindowWrap("sys", 40) <br/>
---   -- set default font colors and font style for window "sys" <br/>
---   setTextFormat("sys",0,35,255,50,50,50,0,0,0) <br/>
---   <br/>
---   echo("sys","Hello world!")
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


--- TODO  createLabel - TLuaInterpreter::createLabel
function createLabel() end


--- This function hides a mini console label. To show it again use showWindow. 
--- @see showWindow
--- @see createMiniConsole
--- @see createLabel
function hideWindow(name) end


--- This function shows a mini console or label. To hide it use hideWindow.
--- @see hideWindow
--- @see createMiniConsole
--- @see createLabel
function showWindow(name) end


--- Creates a named buffer for formatted text, much like a user terminal window, but the buffer
--- cannot be shown on the screen; intended for temporary buffer work
function createBuffer(name) end


--- This function creates a stop watch. It is high resolution time 
--- measurement tool. Stop watches can be started, stopped, reset and asked 
--- how much time has passed since the stop watch has been started. Returns 
--- the ID of a high resolution clock with milliseconds to measure time more 
--- accurately than what is possible with std. Lua routines ? 
--- startStopWatch, stopStopWatch, resetStopWatch, getStopWatchTime example: 
--- In a global script you create all stop watches that you need in your 
--- system and store the respective stopWatch-IDs in global variables: 
--- 
--- @usage fightStopWatchID = createStopWatch(); <br/>
---   -- you store the watchID in a global variable to access it from anywhere <br/>
---   startStopWatch(fightStopWatch); <br/>
---   -- To stop the watch and measure its time in e.g. a trigger script you can write: <br/>
---   fightTime = stopStopWatch(fightStopWatchID) <br/>
---   echo("The fight lasted for " .. fightTime .. " seconds.") <br/>
---   resetStopWatch(fightStopWatchID);
--- 
--- @return stop watch ID 
---   
--- @see startStopWatch
--- @see resetStopWatch
--- @see getStopWatchTime
function createStopWatch() end


--- Returns the time without stoping stop watch (milliseconds based) in form of 0.058 
--- (= clock ran for 58 milliseconds before it was stopped).
--- @see createStopWatch
function getStopWatchTime(watchID) end


--- Stops the stop watch and returns the elapsed time in milliseconds in form of 0.001.
--- @see createStopWatch
--- @return returns time as a number
function stopStopWatch() end


--- Starts the stop watch.
--- @see createStopWatch
function startStopWatch(watchID) end


--- This function resets the time to 0:0:0.0, but does not start the stop watch. You can start it with startStopWatch.
--- @see createStopWatch
--- @see startStopWatch
function resetStopWatch(watchID) end


--- TODO  closeUserWindow - TLuaInterpreter::closeUserWindow
function closeUserWindow() end


--- Resizes a mini console or label. 
--- @see createMiniConsole
--- @see createLabel
--- @see handleWindowResizeEvent
--- @see setBorderTop
function resizeWindow(name, width, height) end


--- Returns the last measured response time between the sent command and the server reply.
--- 
--- @return number of seconds (0.058 (=58 milliseconds lag) or 0.309 (=309 milliseconds))
function getNetworkLatency() end


--- Pastes the previously copied rich text (including text formats like color etc.) into user window name.
---
--- @usage selectString(line, 1)  <br/>
---   copy()
---   appendBuffer("chat")
---
--- @see selectString
--- @see copy
--- @see echo
function appendBuffer(name) end


--- TODO  setBackgroundImage - TLuaInterpreter::setBackgroundImage
function setBackgroundImage() end


--- TODO  setBackgroundColor - TLuaInterpreter::setBackgroundColor
function setBackgroundColor() end


--- TODO  createButton - TLuaInterpreter::createButton
function createButton() end


--- TODO  setLabelClickCallback - TLuaInterpreter::setLabelClickCallback
function setLabelClickCallback() end


--- This function moves window name to the given x/y coordinate. The main screen cannot 
--- be moved. Instead you'll have to set appropriate border values → preferences to move 
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


--- TODO  setTextFormat - TLuaInterpreter::setTextFormat
function setTextFormat() end


--- TODO  getMainWindowSize - TLuaInterpreter::getMainWindowSize
function getMainWindowSize() end


--- TODO  getCurrentLine - TLuaInterpreter::getCurrentLine
function getCurrentLine() end


--- TODO  setMiniConsoleFontSize - TLuaInterpreter::setMiniConsoleFontSize
function setMiniConsoleFontSize() end


--- TODO  selectCurrentLine - TLuaInterpreter::selectCurrentLine
function selectCurrentLine() end


--- TODO  spawn - TLuaInterpreter::spawn
function spawn() end


--- TODO  getButtonState - TLuaInterpreter::getButtonState
function getButtonState() end


--- TODO  showToolBar - TLuaInterpreter::showToolBar
function showToolBar() end


--- TODO  hideToolBar - TLuaInterpreter::hideToolBar
function hideToolBar() end


--- TODO  loadRawFile - TLuaInterpreter::loadRawFile
function loadRawFile() end


--- TODO  setBold - TLuaInterpreter::setBold
function setBold() end


--- TODO  setItalics - TLuaInterpreter::setItalics
function setItalics() end


--- TODO  setUnderline - TLuaInterpreter::setUnderline
function setUnderline() end


--- Disconnect from current session.
-- @see reconnect
function disconnect() end


--- Reconnect to currect session.
-- @see disconnect
function reconnect() end


--- Returns the current home directory of the current profile. This can be used to store data, 
--- save statistical information or load resource files.
--- @usage homedir = getMudletHomeDir()
function getMudletHomeDir() end


--- TODO  setTriggerStayOpen - TLuaInterpreter::setTriggerStayOpen
function setTriggerStayOpen() end


--- Wrap line lineNumber of mini console (window) windowName. This function will interpret \n characters, 
--- apply word wrap and display the new lines on the screen. This function may be necessary if you use 
--- deleteLine() and thus erase the entire current line in the buffer, but you want to do some further 
--- echo() calls after calling deleteLine(). You will then need to re-wrap the last line of the buffer 
--- to actually see what you have echoed and get you \n interpreted as newline characters properly. <br/>
--- <br/>
--- Using this function is no good programming practice and should be avoided. There are better ways of 
--- handling situations where you would call deleteLine() and echo afterwards e.g.: <br/>
--- selectString(line,1); <br/>
--- replace(""); <br/>
--- This will effectively have the same result as a call to deleteLine() but the buffer line will not 
--- be entirely removed. Consequently, further calls to echo() etc. sort of functions are possible 
--- without using wrapLine() unnecessarily.
function wrapLine() end


--- This function returns the rgb values of the color of the first character of the current selection 
--- on mini console (window) windowName. If windowName is omitted Mudlet will use the main screen.
---
--- @usage r,g,b = getFgColor(windowName)
--- @usage local r,g,b; <br/>
---   selectString("troll",1) <br/>
---   r,g,b = getFgColor() <br/>
---   if r == 255 and g == 0 and b == 0 then <br/>
---       echo("HELP! troll is written in red letters, the monster is aggressive!\n"); <br/>
---   end <br/>
---
--- @see getBgColor
function getFgColor(windowName) end


--- Get the rgb values of the first character of the current selection 
--- @usage r,g,b = getBgColor(windowName)
--- @see getFgColor
function getBgColor(windowName) end


--- TODO  tempColorTrigger - TLuaInterpreter::tempColorTrigger
function tempColorTrigger() end


--- TODO  isAnsiFgColor - TLuaInterpreter::isAnsiFgColor
function isAnsiFgColor() end


--- This function tests if the first character of the current selection has the background color specified by ansiBgColorCode.
--- @see isAnsiFgColor
function isAnsiBgColor(ansiBgColorCode) end


--- This function plays a sound file. To make sound work on your operating system you may need to install additional packages: <br/>
--- Microsoft Windows: The underlying multimedia system is used; only WAVE format sound files are supported. (works out of the box) <br/>
--- Mac OS X: NSSound is used. All formats that NSSound supports, including QuickTime formats, are supported by Qt for Mac OS X 
--- (should work out of the box). <br/>
--- X11: The Network Audio System is used if available, otherwise all operations work silently. NAS supports WAVE and AU files. Please use
--- following workaround for Linux systems: <br/><br/>
--- if "linux" == getOS() then <br/>
---	   os.execute("aplay /usr/share/sounds/alsa/Front_Center.wav")<br/>
--- end
function playSoundFile(fileName) end


--- Sets the height of the top border to size pixel and thus effectively moves down the main console 
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
--- @see setBorderBottom
--- @see setBorderLeft
--- @see setBorderRight
function setBorderTop(size) end


--- Sets the height of the bottom border to size pixel and thus effectively moves down the main console 
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
--- @see setBorderTop
--- @see setBorderLeft
--- @see setBorderRight
function setBorderBottom(size) end


--- Sets the width of the left border and thus effectively moves down the main console 
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
--- @see setBorderTop
--- @see setBorderBottom
--- @see setBorderRight
function setBorderLeft(size) end


--- Sets the width of the right border and thus effectively moves down the main console 
--- window by size pixels to make room for e.g. mini console windows, buttons etc..
--- @see setBorderTop
--- @see setBorderBottom
--- @see setBorderLeft
function setBorderRight(size) end


--- Sets the color of the border in RGB color.
--- @usage setBorderColor( 255, 0, 0 ) <br/>
---   sets the border to red.
function setBorderColor(red, green, blue) end


--- Set the scrollback buffer size to linesLimit and determine how many lines are deleted at once in 
--- case the lines limit is reached. The lower the limit the less memory being used. On machines with 
--- low RAM you should consider limiting the size of buffers that don't need a lot of scrollback 
--- e.g. system notification windows, chat windows etc.. <br/>
--- Default values are linesLimit = 100000 lines with 10000 lines of batch deletion. <br/>
--- Minimum buffer size is 100 lines with 10 lines batch deletion.
function setConsoleBufferSize(consoleName, linesLimit, sizeOfBatchDeletion) end


--- TODO  startLogging - TLuaInterpreter::startLogging
function startLogging() end


--- This returns you the height and the length of letters for the given font size. As the primary intended 
--- usage is for calculating the needed dimensions of a miniConsole, it doesn't accept a font argument - 
--- as the miniConsoles currently only work with the default font for the sake of portability.
---
--- @usage x,y = calcFontSize(font_size)
--- @usage Create a miniConsole that is 45 letters in length and 25 letters in height for font size 7 <br/>
---   font_size = 7 <br/>
---   local x, y = calcFontSize( font_size ) <br/>
---   createMiniConsole("map",0,0,0,0) <br/>
---   setMiniConsoleFontSize("map", font_size) <br/>
---   resizeWindow( "map", x*45, y*25 ) <br/>
function calcFontSize(fontSize) end


--- TODO  permRegexTrigger - TLuaInterpreter::permRegexTrigger
function permRegexTrigger() end


--- TODO  permSubstringTrigger - TLuaInterpreter::permSubstringTrigger
function permSubstringTrigger() end


--- TODO  permTimer - TLuaInterpreter::permTimer
function permTimer() end


--- TODO  permAlias - TLuaInterpreter::permAlias
function permAlias() end


--- Tells you how many things of the given type exist. 
--- @usage echo("I have " .. exists("my trigger", "trigger") .. " triggers called 'my trigger'!")
--- @usage You can also use this alias to avoid creating duplicate things, for example: <br/>
--- 
---   -- this code doesn't check if an alias already exists and will keep creating new aliases <br/>
---   permAlias("Attack", "General", "^aa$", [[send ("kick rat")]]) <br/>
---    <br/>
---   -- while this code will make sure that such an alias doesn't exist first <br/>
---   -- we do == 0 instead of 'not exists' because 0 is considered true in Lua <br/>
---   if exists("Attack", "alias") == 0 then <br/>
---       permAlias("Attack", "General", "^aa$", [[send ("kick rat")]]) <br/>
---   end <br/>
--- 
--- @param type can be "alias", "trigger", or "timer".
--- @see isActive
--- @return number
function exists(name, type) end


--- You can use this function to check if something, or somethings, are active. Type can be either 
--- "alias", "trigger", or "timer", and name is the name of the said item you'd like to check.
---
--- @usage echo("I have " .. isActive("my trigger", "trigger") .. " currently active triggers called 'my trigger'!")
---
--- @param type could be "alias", "trigger", or "timer"
function isActive(name, type) end


--- TODO  enableAlias - TLuaInterpreter::enableAlias
function enableAlias() end


--- TODO  permBeginOfLineStringTrigger - TLuaInterpreter::permBeginOfLineStringTrigger
function permBeginOfLineStringTrigger() end


--- TODO  tempAlias - TLuaInterpreter::tempAlias
function tempAlias() end


--- TODO  disableAlias - TLuaInterpreter::disableAlias
function disableAlias() end


--- Deletes an alias with the given name. If several aliases have this name, they'll all be deleted.
---
--- @see killTimer
--- @see killTrigger
function killAlias(name) end


--- TODO  setLabelStyleSheet - TLuaInterpreter::setLabelStyleSheet
function setLabelStyleSheet() end


--- Return time information. <br/>
--- Available since Mudlet 1.0.6.
---
--- @usage getTime()
--- @usage getTime(true)
--- @usage getTime(true, "hh:mm")
---
--- @param returntype Takes a boolean value (in Lua anything but false or nil will translate to true). If true, the function will return a table in the following format: <br/>
---   { hour = #, min = #, sec = #, msec = # } <br/>
---   If false or nil, it will return the time as a string using a format passed to the second arg or the default of hh:mm:ss.zzz
--- @param format Format expressions built from following elements: <br/>
---   <br/>
---   h -              the hour without a leading zero (0 to 23 or 1 to 12 if AM/PM display) <br/>
---   hh -             the hour with a leading zero (00 to 23 or 01 to 12 if AM/PM display) <br/> 
---   H -              the hour without a leading zero (0 to 23, even with AM/PM display) <br/>
---   HH -             the hour with a leading zero (00 to 23, even with AM/PM display) <br/>
---   m -              the minute without a leading zero (0 to 59) <br/>
---   mm -             the minute with a leading zero (00 to 59) <br/>
---   s -              the second without a leading zero (0 to 59) <br/>
---   ss -             the second with a leading zero (00 to 59) <br/>
---   z -              the milliseconds without leading zeroes (0 to 999) <br/>
---   zzz -            the milliseconds with leading zeroes (000 to 999) <br/>
---   AP or A -        use AM/PM display. AP will be replaced by either "AM" or "PM". <br/>
---   ap or a -        use am/pm display. ap will be replaced by either "am" or "pm". <br/>
---   <br/>
---   d -              the day as number without a leading zero (1 to 31) <br/>
---   dd -             the day as number with a leading zero (01 to 31) <br/>
---   ddd -            the abbreviated localized day name (e.g. 'Mon' to 'Sun'). Uses QDate::shortDayName(). <br/>
---   dddd -           the long localized day name (e.g. 'Monday' to 'Qt::Sunday'). Uses QDate::longDayName(). <br/>
---   M -              the month as number without a leading zero (1-12) <br/>
---   MM -             the month as number with a leading zero (01-12) <br/>
---   MMM -            the abbreviated localized month name (e.g. 'Jan' to 'Dec'). Uses QDate::shortMonthName(). <br/>
---   MMMM -           the long localized month name (e.g. 'January' to 'December'). Uses QDate::longMonthName(). <br/>
---   yy -             the year as two digit number (00-99) <br/>
---   yyyy -           the year as four digit number <br/>
---   <br/>
---   All other input characters will be ignored. Any sequence of characters that are enclosed in singlequotes will be treated as text and not be used as an expression. Two consecutive singlequotes ("''") are replaced by a singlequote in the output.
function getTime(returntype, format) end


--- Opens a file chooser dialog, allowing the user to select a file or a folder visually. The function returns 
--- the selected path or nil if there was none chosen.
---
--- @usage local path = invokeFileDialog(false, "Find me the gfx folder, please") <br/>
---   if path then echo(path) end
---
--- @param selection true for file selection, false for folder selection.
--- @param dialogTitle name that the dialog (file chooser window) will have
function invokeFileDialog(selection, dialogTitle) end


--- Returns the timestamp string as it's seen when you enable the timestamps view (blue i button bottom right). <br/>
--- Available since 1.1.0-pre1
---
--- @usage echo( getTimestamp(getLineCount()) ) <br/>
---   Echo the timestamp of the current line in a trigger
--- 
--- @param console_name optional parameter
function getTimestamp(console_name, lineNumber) end


--- Turns the selected text into a clickable link - upon being clicked, the link will do the command code. 
--- Tooltip is a string which will be displayed when the mouse is over the selected text. <br/>
--- Available since 1.1.0-pre1.
---
--- @usage In a sewer grate substring trigger, the following code will make clicking on the words do the send("enter grate") command: <br/>
---   selectString(matches[1], 1) <br/>
---   setLink([[send("enter grate")]], "Clicky to enter grate") <br/>
---
--- @see echoLink
--- @see insertLink
function setLink(command, tooltip) end


--- Clears the current selection in the main window or miniConsole.
--- @usage deselect()
--- @usage deselect("myMiniConsole")
function deselect(windowName) end


--- Same as echoLink() but inserts the text at the current cursor position, while echoLink inserts at the end 
--- of the current line. <br/>
--- Available since 1.1.0-pre1.
--- @see echoLink
--- @see setLink
function insertLink(windowName, text, command, hint, useCurrentFormat) end


--- Echos a piece of text as a clickable link. <br/>
--- Available since 1.1.0-pre1.
---
--- @usage echoLink("hi", [[echo("hey bub!")]], "click me now")
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


--- Same as setPopup()  except it doesn’t require a selection. echoPopup creates a link from the given text that it echos. <br/>
--- Available since 1.1.0-pre1.
---
--- @usage echoPopup("activities to do", {[[send "sleep"]], [[send "sit"]], [[send "stand"]]}, {"sleep", "sit", "stand"})
---
--- @see setPopup
--- @see insertPopup
function echoPopup() end


--- Same as echoPopup(), but inserts text at the current cursor position. <br/>
--- Available since 1.1.0-pre1.
---
--- @see setPopup
--- @see echoPopup
function insertPopup(windowName, text, commands, hints, useCurrentFormat) end


--- Turns the selected text into a left-clickable link, and a right-click menu. The selected text, 
--- upon being left-clicked, will do the first command in the list. Upon being right-clicked, it’ll 
--- display a menu with all possible commands. The menu will be populated with hints, one for each line. <br/>
--- Available since 1.1.0-pre1.
--- 
--- @usage In a Raising your hand in greeting, you say "Hello!" exact match trigger, the following code will 
---   make left-clicking on Hello show you an echo, while left-clicking will show some commands you can do. <br/>
---   selectString("Hello", 1) <br/>
---   setPopup("main", {[[send("bye")]], [[echo("hi!")]]}, {"left-click or right-click and do first item to send bye", "click to echo hi"}) <br/>
---
--- @param name the name of the console to operate on. If not using this in a miniConsole, use "main" as the name.
--- @param commands a table of lua code strings to do e.g. {[[send("hello")]], [[echo("hi!"]]}.
--- @param hints a table of strings which will be shown on the popup and right-click menu e.g. {"send the hi command", "echo hi to yourself"}.
function setPopup(name, commands, hints) end


--- TODO  sendATCP - TLuaInterpreter::sendATCP
function sendATCP() end


--- TODO  hasFocus - TLuaInterpreter::hasFocus
function hasFocus() end


--- Returns true or false depending on if the current line being processed is a prompt. This infallible 
--- feature is available for MUDs that supply GA events (to check if yours is one, look to bottom-right 
--- of the main window - if it doesn't say &lt;No&nbsp;GA&gt;, then it supplies them).
--- @return true/false
function isPrompt() end


--- This function will have Mudlet parse the given text as if it came from the MUD - one great application 
--- is trigger testing. You can use \n to represent a new line. The function also accept ANSI color codes that 
--- are used in MUDs. A sample table can be found here.
---
--- @usage feedTriggers("\nYou sit yourself down.\n") <br/>	
---   Usage of new line character.
--- @usage feedTriggers("\nThis is \27[1;32mgreen\27[0;37m, \27[1;31mred\27[0;37m, " .. 
---   "\27[46mcyan background\27[0;37m,\27[32;47mwhite background and green foreground\27[0;37m.\n") <br/>
---   Demonstration of ANSI color.
function feedTriggers(text) end


--- Send telnet channel 102 commands to MUD. This is widely used on Aardwolf for setting up user tags. <br/>
--- @usage sendTelnetChannel102("\5\1") <br/>
---   Command will enable all Aardwolf channel tags. You need to have active session.
function sendTelnetChannel102() end



end

