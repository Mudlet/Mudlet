-----------------------------
-- Useful LUA functions
-----------------------------
-- Written by John Dahlstrom
-----------------------------

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

-- Echo something under your current line
function echon(what)
	moveCursor(0, 1)
	insertHTML(what)
	echo("\n")
end

-- Echo something after your line
function suffix(what)
	local length = string.len(line)
	moveCursor(length-1, 0)
	insertHTML("&nbsp;"..what)
end


-- Echo something before your line
function prefix(what)
	insertHTML(what.."&nbsp;")
end


-- Gag the whole line
function gagLine()
	selectString(line, 1)
	replace("")
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
	insertHTML(what)
end

