-- Everything here drives the real API: no mocking, and each function gets both
-- what it answers (including when it is misused) and, where it can be reached
-- offline, what it actually did - the file it wrote, the event it raised, the
-- line it put on screen.
--
-- Console readback goes through textFrom()/wrapped(): the main console wraps
-- long lines, and a wrap swallows the space it broke at, so the text is
-- compared with all whitespace removed rather than line by line.

local function contains(haystack, needle)
  return type(haystack) == "string" and haystack:find(needle, 1, true) ~= nil
end

-- Asserts that calling fn raises a Lua error whose message contains needle.
-- Matching a message substring (rather than merely "did it error?") ensures the
-- function is actually registered and reached its own argument validation: an
-- unregistered/nil function would raise a different "attempt to call" error.
local function assertArgError(fn, needle)
  local ok, err = pcall(fn)
  assert.is_false(ok)
  assert.is_true(contains(err, needle), tostring(err))
end

-- Everything the main console gained since it was at line `mark`, joined up.
local function textFrom(mark)
  return table.concat(getLines("main", mark, getLastLineNumber("main") + 1), "")
end

local function wrapped(text)
  return (tostring(text):gsub("%s+", ""))
end

local function containsWrapped(haystack, needle)
  return contains(wrapped(haystack), wrapped(needle))
end

local function fileExists(path)
  return lfs.attributes(path, "mode") ~= nil
end

local function readFile(path)
  local handle = io.open(path, "rb")
  if not handle then
    return nil
  end
  local contents = handle:read("*a")
  handle:close()
  return contents
end

local function writeFile(path, contents)
  local handle = io.open(path, "wb")
  assert.is_not_nil(handle, "could not write to " .. path)
  handle:write(contents)
  handle:close()
end

-- Takes a copy of the encoding in use, to be put back once a spec has changed
-- it. setServerEncoding() also writes the profile's "encoding" file, and a
-- profile that never had one must not be left with one.
local function restoreServerEncoding()
  local encodingFile = getMudletHomeDir() .. "/encoding"
  local hadFile = fileExists(encodingFile)
  local original = getServerEncoding()
  return function()
    setServerEncoding(original)
    if not hadFile then
      os.remove(encodingFile)
    end
  end
end

local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
assert(specDirectory, "Miscallaneous_spec.lua has to be run from a file so that it can find its fixtures")
local fixtureDirectory = specDirectory .. "/fixtures/packages"

-- waitForEvent() and pumpEvents() answer nil and a message outside test mode,
-- so the specs that need an event to arrive say so instead of failing on a
-- developer's interactive run.
local testMode = os.getenv("MUDLET_TEST_MODE")

describe("Tests C++ functions in the Miscallaneous category", function()
    describe("Tests the functionality of sendMSDP", function()
      it("should return nil and an error message when MSDP cannot be sent", function()
        local ok, err = sendMSDP("CLIENT_NAME", "Mudlet")
        if ok ~= nil then
          -- connected to a server which negotiated MSDP, so the send succeeded
          assert.is_true(ok)
          return
        end
        assert.is_nil(ok)
        assert.is_string(err)
        assert.is_true(err:find("MSDP") ~= nil)
      end)
    end)

    describe("Tests the functionality of getOS", function()
      it("should return the correct number of values for the current OS", function()
        local results = {getOS()}
        -- Linux returns 4 values, all others return 3
        if results[1] == "linux" then
          assert.equals(4, #results)
        else
          assert.equals(3, #results)
        end
      end)

      it("should return string values", function()
        local osName, osVersion, osType = getOS()
        assert.is_string(osName)
        assert.is_string(osVersion)
        if osType then
          assert.is_string(osType)
        end
      end)

      it("should return a valid OS name as first value", function()
        local validOSNames = {
          "windows", "mac", "linux", "cygwin", "hurd",
          "freebsd", "kfreebsd", "openbsd", "netbsd",
          "bsd4", "unix", "unknown"
        }
        local osName = getOS()
        assert.is_true(table.contains(validOSNames, osName))
      end)

      it("should not return empty strings", function()
        local osName, osVersion, osType = getOS()
        assert.is_true(osName ~= "")
        assert.is_true(osVersion ~= "")
        if osType then
          assert.is_true(osType ~= "")
        end
      end)
    end)

    describe("Tests the functionality of getTimestamp", function()
      it("should return a string for a valid line number", function()
        echo("getTimestamp test line\n")
        assert.is_string(getTimestamp(1))
      end)

      it("should return nil+msg for an out-of-range line number", function()
        local timestamp, err = getTimestamp(getLineCount() + 1000)
        assert.is_nil(timestamp)
        assert.is_string(err)
        assert.is_true(err:find("beyond the last line", 1, true) ~= nil)
      end)

      it("should return nil+msg for an out-of-range line number in a miniconsole", function()
        createMiniConsole("getTimestampTestConsole", 0, 0, 100, 100)
        local timestamp, err = getTimestamp("getTimestampTestConsole", 1000000)
        assert.is_nil(timestamp)
        assert.is_string(err)
        assert.is_true(err:find("beyond the last line", 1, true) ~= nil)
        deleteMiniConsole("getTimestampTestConsole")
      end)
    end)

    describe("Tests the functionality of getModulePath", function()
      it("should return nil+msg for a module that does not exist", function()
        local path, err = getModulePath("busted-nonexistent-module")
        assert.is_nil(path)
        assert.is_string(err)
        assert.is_true(err:find("module doesn't exist", 1, true) ~= nil)
      end)
    end)

    describe("Tests the functionality of getModulePriority", function()
      it("should return nil+msg for a module that does not exist", function()
        local priority, err = getModulePriority("busted-nonexistent-module")
        assert.is_nil(priority)
        assert.is_string(err)
        assert.is_true(err:find("module doesn't exist", 1, true) ~= nil)
      end)
    end)

    describe("Tests the functionality of getCommandSeparator", function()
      it("returns the separator the profile splits commands on", function()
        assert.equals(";;", getCommandSeparator())
      end)

      it("returns the separator that actually splits a command", function()
        local fired = {}
        local first = tempAlias("^mudletSpecSeparatorA$", function() fired[#fired + 1] = "A" end)
        local second = tempAlias("^mudletSpecSeparatorB$", function() fired[#fired + 1] = "B" end)
        finally(function()
          killAlias(tostring(first))
          killAlias(tostring(second))
        end)

        expandAlias("mudletSpecSeparatorA" .. getCommandSeparator() .. "mudletSpecSeparatorB", false)

        assert.same({"A", "B"}, fired)
      end)
    end)

    describe("Tests the functionality of getTime", function()
      it("raises a Lua error when the first argument is not a boolean", function()
        assertArgError(function() getTime("yes") end, "getTime: bad argument #1 type")
      end)

      it("raises a Lua error when the format is not a string", function()
        assertArgError(function() getTime(true, {}) end, "getTime: bad argument #2 type")
      end)

      it("returns the time as a string in the documented default format", function()
        local time = getTime(true)
        assert.is_string(time)
        assert.is_truthy(time:match("^%d%d%d%d%.%d%d%.%d%d %d%d:%d%d:%d%d%.%d%d%d$"), time)
      end)

      it("honours a custom format", function()
        assert.equals(getTime(true, "yyyy"), tostring(getTime().year))
        assert.is_truthy(getTime(true, "hh:mm"):match("^%d%d:%d%d$"))
      end)

      it("returns a table of the parts when not asked for a string", function()
        local time = getTime()
        assert.is_table(time)
        for _, field in ipairs({"year", "month", "day", "hour", "min", "sec", "msec"}) do
          assert.is_number(time[field], field .. " is missing")
        end
        assert.is_true(time.month >= 1 and time.month <= 12)
        assert.is_true(time.day >= 1 and time.day <= 31)
        assert.is_true(time.hour >= 0 and time.hour <= 23)
        assert.is_true(time.min >= 0 and time.min <= 59)
        assert.is_true(time.sec >= 0 and time.sec <= 60)
        assert.is_true(time.msec >= 0 and time.msec <= 999)
      end)

      it("returns the same date in both forms", function()
        -- each call reads the clock afresh, so a run that steps over midnight
        -- between the two would see different dates: the table form is read
        -- between two string forms and has to match one of them
        local before = getTime(true, "yyyy-MM-dd")
        local asTable = getTime()
        local after = getTime(true, "yyyy-MM-dd")

        local asDate = string.format("%04d-%02d-%02d", asTable.year, asTable.month, asTable.day)
        assert.is_true(asDate == before or asDate == after, asDate .. " is neither " .. before .. " nor " .. after)
      end)
    end)

    describe("Tests the functionality of getProcessID", function()
      it("returns this process's own id", function()
        local pid = getProcessID()
        assert.is_number(pid)
        assert.equals(math.floor(pid), pid)
        assert.equals(pid, getProcessID())
        if getOS() == "linux" then
          -- the number is only worth anything if the operating system agrees
          -- that it is this process
          assert.equals("directory", lfs.attributes("/proc/" .. pid, "mode"))
        else
          assert.is_true(pid > 0)
        end
      end)
    end)

    describe("Tests the functionality of getServerEncodingsList", function()
      it("lists ASCII first and then every encoding Mudlet can be switched to", function()
        local encodings = getServerEncodingsList()
        assert.is_table(encodings)
        assert.equals("ASCII", encodings[1])
        assert.is_true(#encodings > 1)

        local seen = {}
        for _, encoding in ipairs(encodings) do
          assert.is_string(encoding)
          -- the "M_" prefix marks Mudlet's own codecs and is not part of the
          -- name the rest of the API uses
          assert.is_nil(encoding:find("^M_"), encoding .. " leaked its internal prefix")
          assert.is_nil(seen[encoding], encoding .. " is listed twice")
          seen[encoding] = true
        end
        assert.is_true(seen[getServerEncoding()], "the encoding in use is not in the list")
      end)

      it("names every encoding in the form setServerEncoding accepts", function()
        finally(restoreServerEncoding())

        for _, encoding in ipairs(getServerEncodingsList()) do
          assert.is_true(setServerEncoding(encoding), "the list offered " .. encoding .. " but setServerEncoding refused it")
          assert.equals(encoding, getServerEncoding())
        end
      end)
    end)

    describe("Tests the functionality of getMudletInfo", function()
      it("returns nothing and reports the encodings on the main console", function()
        local mark = getLastLineNumber("main")

        assert.equals(0, select('#', getMudletInfo()))

        local text = textFrom(mark)
        -- a profile that has not been switched to a real encoding reports the
        -- ASCII it falls back to in quotes
        local encoding = getServerEncoding()
        local reported = containsWrapped(text, "Current encoding: " .. encoding) or containsWrapped(text, 'Current encoding: "' .. encoding .. '"')
        assert.is_true(reported, text)
        assert.is_true(containsWrapped(text, "Available encodings:"), text)
        for _, encoding in ipairs(getServerEncodingsList()) do
          assert.is_true(containsWrapped(text, encoding), encoding .. " was not reported")
        end
      end)
    end)

    describe("Tests the functionality of getWindowsCodepage", function()
      it("only answers on Windows", function()
        local codepage, err = getWindowsCodepage()
        if getOS() == "windows" then
          assert.is_string(codepage)
          assert.is_nil(err)
          return
        end
        assert.is_nil(codepage)
        assert.is_true(contains(err, "only needed on Windows"), tostring(err))
      end)
    end)

    describe("Tests the functionality of getCharacterName", function()
      it("returns nil+msg while no character name is set", function()
        local name, err = getCharacterName()
        if name ~= nil then
          -- a profile that has a login set answers with it instead
          assert.is_string(name)
          assert.is_true(#name > 0)
          return
        end
        assert.equals("no character name set", err)
      end)
    end)

    describe("Tests the profile description accessors", function()
      -- The description is profile data on disk, so every spec here puts back
      -- what it found: the self-test profile is reused between runs.
      local descriptionFile = getMudletHomeDir() .. "/description"

      -- A game Mudlet lists in the connection dialog that has no folder here, or
      -- nil if they all have one. Such a name resolves for a profile lookup
      -- without being a profile, which is the case worth testing. getProfiles()
      -- lists folders, so a bundled game missing from it has none; several are
      -- offered so that a run against a config where some have been opened still
      -- finds one.
      local function unopenedBundledGame()
        local profiles = getProfiles()
        for _, game in ipairs({"Achaea", "Aetolia", "Lusternia", "Imperian", "StickMUD", "Materia Magica"}) do
          if not profiles[game] then
            return game
          end
        end
      end

      local function restoreDescription()
        local original = getProfileInformation()
        -- a profile that has never had a description has no file for one, and
        -- writing the empty string back would leave one behind
        local hadFile = fileExists(descriptionFile)
        return function()
          setProfileInformation(original)
          if not hadFile then
            os.remove(descriptionFile)
          end
        end
      end

      describe("Tests the functionality of getProfileInformation", function()
        it("raises a Lua error when the profile name is not a string", function()
          assertArgError(function() getProfileInformation({}) end, "getProfileInformation: bad argument #1 type")
        end)

        it("returns nil+msg for an empty profile name", function()
          local info, err = getProfileInformation("")
          assert.is_nil(info)
          assert.equals("getProfileInformation: profile name cannot be empty", err)
        end)

        it("returns nil+msg for a profile that does not exist", function()
          local info, err = getProfileInformation("mudlet-spec-never-a-profile")
          assert.is_nil(info)
          assert.is_true(contains(err, "does not exist"), tostring(err))
        end)

        it("returns a string for this profile", function()
          assert.is_string(getProfileInformation())
          assert.equals(getProfileInformation(), getProfileInformation(getProfileName()))
        end)
      end)

      describe("Tests the functionality of setProfileInformation", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() setProfileInformation() end, "setProfileInformation: bad argument #1 type")
        end)

        it("raises a Lua error when the two argument form has no text", function()
          assertArgError(function() setProfileInformation(getProfileName(), {}) end, "setProfileInformation: bad argument #2 type")
        end)

        it("round-trips a description through getProfileInformation", function()
          finally(restoreDescription())

          assert.is_true(setProfileInformation("set by the Miscallaneous specs"))
          assert.equals("set by the Miscallaneous specs", getProfileInformation())
          assert.equals("set by the Miscallaneous specs", getProfileInformation(getProfileName()))
        end)

        it("round-trips a description named by profile", function()
          finally(restoreDescription())

          assert.is_true(setProfileInformation(getProfileName(), "named form"))
          assert.equals("named form", getProfileInformation())
        end)

        it("matches the profile whatever case it is named in", function()
          finally(restoreDescription())

          assert.is_true(setProfileInformation(getProfileName():upper(), "shouted form"))
          assert.equals("shouted form", getProfileInformation())
          -- naming the profile in the wrong case must find the folder it has,
          -- not make a second one beside it
          assert.is_nil(getProfiles()[getProfileName():upper()])
        end)

        it("is what getProfiles reports as the description", function()
          finally(restoreDescription())
          setProfileInformation("as seen by getProfiles")

          assert.equals("as seen by getProfiles", getProfiles()[getProfileName()].description)
        end)

        it("refuses a profile that does not exist", function()
          local ok, err = setProfileInformation("mudlet-spec-never-a-profile", "text")
          assert.is_nil(ok)
          assert.equals("profile 'mudlet-spec-never-a-profile' does not exist", err)
          -- refusing is not enough on its own: the write goes through
          -- writeProfileData(), which creates whatever folder it is handed, and
          -- a folder here is a profile to getProfiles() and the connection dialog
          local profiles = getProfiles()
          assert.is_table(profiles[getProfileName()], "getProfiles() answered nothing at all")
          assert.is_nil(profiles["mudlet-spec-never-a-profile"])
        end)

        it("refuses a game Mudlet ships with that has no profile of its own", function()
          local game = unopenedBundledGame()
          if not game then
            pending("every bundled game this spec knows of has a profile here")
            return
          end

          -- the getter answers for this name, which is what makes it the
          -- bundled-game case rather than a second unknown-name spec
          assert.is_string(getProfileInformation(game))

          local ok, err = setProfileInformation(game, "text")
          assert.is_nil(ok)
          assert.equals(("profile '%s' does not exist"):format(game), err)
          assert.is_nil(getProfiles()[game])
        end)
      end)

      describe("Tests the functionality of clearProfileInformation", function()
        it("raises a Lua error when the profile name is not a string", function()
          assertArgError(function() clearProfileInformation({}) end, "clearProfileInformation: bad argument #1 type")
        end)

        it("refuses a profile that does not exist", function()
          local ok, err = clearProfileInformation("mudlet-spec-never-a-profile")
          assert.is_nil(ok)
          assert.equals("profile 'mudlet-spec-never-a-profile' does not exist", err)
          assert.is_nil(getProfiles()["mudlet-spec-never-a-profile"])
        end)

        it("refuses a game Mudlet ships with that has no profile of its own", function()
          local game = unopenedBundledGame()
          if not game then
            pending("every bundled game this spec knows of has a profile here")
            return
          end

          assert.is_string(getProfileInformation(game))

          local ok, err = clearProfileInformation(game)
          assert.is_nil(ok)
          assert.equals(("profile '%s' does not exist"):format(game), err)
          -- clearing writes the description the game ships with, so a folder
          -- made here would not merely exist, it would read as a set up profile
          assert.is_nil(getProfiles()[game])
        end)

        it("puts back the description a bundled game ships with", function()
          finally(restoreDescription())
          setProfileInformation("something else entirely")

          -- both forms have to restore the blurb, so the named one clears first
          -- and the description is dirtied again for the no-argument one
          assert.is_true(clearProfileInformation(getProfileName()))
          setProfileInformation("something else entirely")
          assert.is_true(clearProfileInformation())

          -- the self-test profile is one of Mudlet's own games, so clearing
          -- restores its built-in blurb rather than emptying the description
          local restored = getProfileInformation()
          assert.is_string(restored)
          assert.are_not.equals("something else entirely", restored)
          assert.is_true(contains(restored, "Busted"), restored)
        end)
      end)
    end)

    describe("Tests the command history saving accessors", function()
      describe("Tests the functionality of getSaveCommandHistory", function()
        it("returns nil+msg for a command line that does not exist", function()
          local saving, err = getSaveCommandHistory("mudlet-spec-no-such-command-line")
          assert.is_nil(saving)
          assert.is_true(contains(err, "not found"), tostring(err))
        end)

        it("answers for the main command line when not told which one", function()
          local saving, message = getSaveCommandHistory()
          assert.is_boolean(saving)
          assert.is_string(message)
          -- the name it defaults to is what makes the two forms the same call,
          -- so the state is set through the named form and read back through both
          finally(function() setSaveCommandHistory("main", saving) end)
          assert.is_true(setSaveCommandHistory("main", not saving))
          assert.equals(not saving, (getSaveCommandHistory()))
          assert.equals(not saving, (getSaveCommandHistory("main")))
        end)
      end)

      describe("Tests the functionality of setSaveCommandHistory", function()
        it("raises a Lua error when the argument is neither a name nor a boolean", function()
          assertArgError(function() setSaveCommandHistory(5) end, "setSaveCommandHistory: bad argument #1 type")
        end)

        it("turns saving on when told which command line, or none at all, but not whether to", function()
          local original = getSaveCommandHistory()
          finally(function() setSaveCommandHistory(original) end)
          -- turning it off first is what makes turning it on observable, so the
          -- off state is asserted rather than assumed
          assert.is_true(setSaveCommandHistory(false))
          assert.is_false((getSaveCommandHistory()))

          assert.is_true(setSaveCommandHistory())
          assert.is_true((getSaveCommandHistory()))

          assert.is_true(setSaveCommandHistory(false))
          assert.is_false((getSaveCommandHistory()))
          assert.is_true(setSaveCommandHistory("main"))
          assert.is_true((getSaveCommandHistory()))
        end)

        it("turns saving on for the command line it is named, and no other", function()
          -- "main" is also the name the no-argument form falls back to, so only
          -- a second command line can tell "the name was read" from "the name
          -- was dropped and main was used"
          local commandLine = "mudlet-spec-save-history"
          createCommandLine(commandLine, 10, 10, 120, 30)
          local original = getSaveCommandHistory()
          finally(function()
            setSaveCommandHistory("main", original)
            deleteCommandLine(commandLine)
          end)

          assert.is_true(setSaveCommandHistory(commandLine, false))
          assert.is_true(setSaveCommandHistory("main", false))

          assert.is_true(setSaveCommandHistory(commandLine))

          assert.is_true((getSaveCommandHistory(commandLine)))
          assert.is_false((getSaveCommandHistory("main")))
        end)

        it("returns nil+msg for a command line that does not exist", function()
          local ok, err = setSaveCommandHistory("mudlet-spec-no-such-command-line")
          assert.is_nil(ok)
          assert.is_true(contains(err, "not found"), tostring(err))
        end)

        it("round-trips through getSaveCommandHistory", function()
          local original = getSaveCommandHistory()
          finally(function() setSaveCommandHistory(original) end)

          assert.is_true(setSaveCommandHistory(false))
          assert.is_false((getSaveCommandHistory()))
          assert.equals("disabled", (select(2, getSaveCommandHistory())))

          assert.is_true(setSaveCommandHistory("main", true))
          local saving, message = getSaveCommandHistory("main")
          assert.is_true(saving)
          assert.equals("enabled (" .. getConfig("commandLineHistorySaveSize") .. " lines will be saved)", message)
        end)

        it("is refused, and the getter reports off, while the profile has history saving turned off", function()
          local savedLines = getConfig("commandLineHistorySaveSize")
          finally(function() setConfig("commandLineHistorySaveSize", savedLines) end)

          setConfig("commandLineHistorySaveSize", 0)

          local saving, getterMessage = getSaveCommandHistory()
          assert.is_false(saving)
          assert.equals("disabled by profile global preference", getterMessage)

          local ok, setterMessage = setSaveCommandHistory(true)
          assert.is_nil(ok)
          assert.equals("disabled by profile global preference", setterMessage)
        end)
      end)
    end)

    describe("Tests the logging functions", function()
      describe("Tests the functionality of startLogging", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() startLogging() end, "startLogging: bad argument #1 type")
        end)

        it("starts and stops logging, reporting the file it uses", function()
          local logPath
          finally(function()
            startLogging(false)
            if logPath then
              os.remove(logPath)
            end
          end)

          local started, startMessage, startPath, startState = startLogging(true)
          logPath = startPath
          assert.is_true(started)
          assert.is_string(startPath)
          assert.equals(1, startState)
          assert.is_true(contains(startMessage, startPath), startMessage)
          assert.is_true(fileExists(startPath), "no log file was created")

          echo("mudlet-spec-logged-line\n")

          local stopped, stopMessage, stopPath, stopState = startLogging(false)
          assert.is_true(stopped)
          assert.equals(startPath, stopPath)
          assert.equals(0, stopState)
          assert.is_true(contains(stopMessage, "stopped being logged"), stopMessage)
          -- the line logged most recently is held back for duplicate detection
          -- and only written out when logging stops, so read the file after
          assert.is_true(contains(readFile(startPath), "mudlet-spec-logged-line"), "the console output did not reach the log")
        end)

        it("reports, rather than repeats, a state it is already in", function()
          local logPath
          finally(function()
            startLogging(false)
            if logPath then
              os.remove(logPath)
            end
          end)

          local alreadyOff, offMessage, offPath, offState = startLogging(false)
          assert.is_nil(alreadyOff)
          assert.equals("Main console output was already not being logged to a file.", offMessage)
          assert.is_nil(offPath)
          assert.equals(-2, offState)

          logPath = select(3, startLogging(true))
          local alreadyOn, onMessage, onPath, onState = startLogging(true)
          assert.is_nil(alreadyOn)
          assert.equals(logPath, onPath)
          assert.equals(-1, onState)
          assert.is_true(contains(onMessage, "already being logged"), onMessage)
        end)
      end)

      describe("Tests the functionality of appendLog", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() appendLog() end, "appendLog: bad argument #1 type")
        end)

        it("writes the text into the log file", function()
          local logPath
          finally(function()
            startLogging(false)
            if logPath then
              os.remove(logPath)
            end
          end)
          logPath = select(3, startLogging(true))

          appendLog("mudlet-spec-appended-line")

          -- read only once logging has stopped: the log stream is buffered
          startLogging(false)
          local contents = readFile(logPath)
          assert.is_string(contents)
          assert.is_true(contains(contents, "mudlet-spec-appended-line"), "the appended text is not in the log")
        end)

        it("writes nothing while logging is off", function()
          local logPath
          finally(function()
            startLogging(false)
            if logPath then
              os.remove(logPath)
            end
          end)
          logPath = select(3, startLogging(true))
          startLogging(false)

          assert.equals(0, select('#', appendLog("mudlet-spec-never-logged")))

          local contents = readFile(logPath)
          assert.is_string(contents, "the log file that was closed is not readable")
          assert.is_false(contains(contents, "mudlet-spec-never-logged"), "the text was appended to a log that was closed")
        end)
      end)

      -- A received line is held back from the log until the next one commits.
      -- That deferral is what duplicate detection needs, and it is also the
      -- window in which a trigger can still gag the line with deleteLine().
      -- These pin down what it must, and must not, swallow.
      describe("Tests what the deferred logging of received lines writes out", function()
        local htmlLogging

        setup(function()
          -- plain text, so the assertions read the lines rather than the markup
          -- an HTML log would wrap them in
          htmlLogging = getConfig("logInHTML")
          setConfig("logInHTML", false)
        end)

        teardown(function()
          setConfig("logInHTML", htmlLogging)
        end)

        -- counts plain-text occurrences, so a needle carrying a Lua pattern
        -- character still counts what it looks like
        local function occurrences(haystack, needle)
          local seen, from = 0, 1
          while true do
            local start, stop = haystack:find(needle, from, true)
            if not start then
              return seen
            end
            seen, from = seen + 1, stop + 1
          end
        end

        it("keeps a line the window was cleared after", function()
          local logPath
          finally(function()
            startLogging(false)
            if logPath then
              os.remove(logPath)
            end
          end)
          logPath = select(3, startLogging(true))

          local mark = getLastLineNumber("main")
          feedTelnet("You are dead.\n")
          assert.is_true(contains(textFrom(mark), "You are dead."), "the fed line did not reach the console buffer - start the suite with --offline, see the tests README")

          -- the main console is shared with every other spec file, so this is
          -- the one place in the suite that empties it; nothing else reads
          -- console content it did not put there itself
          clearWindow()
          feedTelnet("You emerge unscathed.\n")
          startLogging(false)

          local log = readFile(logPath)
          assert.is_string(log, "the log file that was closed is not readable")
          assert.is_true(contains(log, "You are dead."), "clearing the window dropped the line that was pending for logging")
          assert.is_true(contains(log, "You emerge unscathed."), "the line received after the window was cleared is missing from the log")
        end)

        it("keeps a line whose own trigger cleared the window", function()
          local logPath, triggerId
          finally(function()
            startLogging(false)
            if triggerId then
              killTrigger(triggerId)
            end
            if logPath then
              os.remove(logPath)
            end
          end)
          logPath = select(3, startLogging(true))
          triggerId = tempRegexTrigger("^You perish$", [[clearWindow()]])

          feedTelnet("You perish\n")
          feedTelnet("A new dawn\n")
          startLogging(false)

          local log = readFile(logPath)
          assert.is_string(log, "the log file that was closed is not readable")
          assert.is_true(contains(log, "You perish"), "a line whose own trigger cleared the window was dropped from the log")
          assert.is_true(contains(log, "A new dawn"), "the line received after the window was cleared is missing from the log")
        end)

        it("leaves out a line its trigger gagged with deleteLine", function()
          local logPath, triggerId
          finally(function()
            startLogging(false)
            if triggerId then
              killTrigger(triggerId)
            end
            if logPath then
              os.remove(logPath)
            end
          end)
          logPath = select(3, startLogging(true))
          triggerId = tempRegexTrigger("^Top secret plans$", [[deleteLine()]])

          feedTelnet("Before the gag.\n")
          feedTelnet("Top secret plans\n")
          feedTelnet("After the gag.\n")
          startLogging(false)

          local log = readFile(logPath)
          assert.is_string(log, "the log file that was closed is not readable")
          assert.is_true(contains(log, "Before the gag."), "the line before the gagged one is missing from the log")
          assert.is_false(contains(log, "Top secret plans"), "the gagged line leaked into the log")
          assert.is_true(contains(log, "After the gag."), "the line after the gagged one is missing from the log")
        end)

        it("does not replay the last line of one session into the next", function()
          local firstPath, secondPath
          finally(function()
            startLogging(false)
            if firstPath then
              os.remove(firstPath)
            end
            if secondPath and secondPath ~= firstPath then
              os.remove(secondPath)
            end
          end)

          firstPath = select(3, startLogging(true))
          local mark = getLastLineNumber("main")
          feedTelnet("Session one final line.\n")
          assert.is_true(contains(textFrom(mark), "Session one final line."), "the fed line did not reach the console buffer - start the suite with --offline, see the tests README")

          -- stopping flushes the line that was still pending
          local stoppedState = select(4, startLogging(false))
          assert.equals(0, stoppedState)
          local restartedPath, restartedState = select(3, startLogging(true))
          secondPath = restartedPath
          assert.equals(1, restartedState)

          feedTelnet("Session two line.\n")
          startLogging(false)

          local log = readFile(secondPath)
          assert.is_string(log, "the log file that was closed is not readable")
          -- the log file is named after the second it was opened in and there is
          -- no Lua setter for that name, so the restart either appends to the
          -- same file or opens a new one. Either way the first session's last
          -- line is written exactly once, and only into the log that was open
          -- when it arrived.
          if secondPath == firstPath then
            assert.equals(1, occurrences(log, "Session one final line."))
          else
            assert.equals(0, occurrences(log, "Session one final line."))
            local firstLog = readFile(firstPath)
            assert.is_string(firstLog, "the first session's log file is not readable")
            assert.equals(1, occurrences(firstLog, "Session one final line."))
          end
          assert.is_true(contains(log, "Session two line."), "the second session's own line is missing from its log")
        end)
      end)
    end)

    describe("Tests the file watching functions", function()
      local watchedFile = getMudletHomeDir() .. "/mudlet-spec-watched.txt"

      describe("Tests the functionality of addFileWatch", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() addFileWatch() end, "addFileWatch: bad argument #1 type")
        end)

        it("returns nil+msg for a path that is not there", function()
          local ok, err = addFileWatch(getMudletHomeDir() .. "/mudlet-spec-no-such-path")
          assert.is_nil(ok)
          assert.is_true(contains(err, "does not exist"), tostring(err))
        end)
      end)

      describe("Tests the functionality of removeFileWatch", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() removeFileWatch() end, "removeFileWatch: bad argument #1 type")
        end)

        it("returns false for a path nobody is watching", function()
          assert.is_false(removeFileWatch(getMudletHomeDir() .. "/mudlet-spec-no-such-path"))
        end)
      end)

      describe("Tests watching a file for changes", function()
        it("watches a file that is there, and stops when told to", function()
          -- adding and removing a watch needs no events, so unlike the spec
          -- below this one runs outside test mode too
          finally(function() os.remove(watchedFile) end)
          writeFile(watchedFile, "first\n")

          assert.is_true(addFileWatch(watchedFile))

          assert.is_true(removeFileWatch(watchedFile))
          assert.is_false(removeFileWatch(watchedFile), "the watch was removed twice")
        end)

        it("raises sysPathChanged until the watch is removed", function()
          if not testMode then
            pending("waiting for sysPathChanged needs MUDLET_TEST_MODE")
            return
          end
          finally(function()
            removeFileWatch(watchedFile)
            os.remove(watchedFile)
          end)
          writeFile(watchedFile, "first\n")

          assert.is_true(addFileWatch(watchedFile))
          -- Windows works out that a file changed by comparing its modification
          -- time against the one noted when the watch was added - the contents
          -- and the size are not looked at - and that stamp only moves as fast
          -- as the system clock ticks, about every 16ms. Rewriting the file in
          -- the same tick would be invisible there, so leave the stamp room to
          -- move before writing again.
          pumpEvents(250)
          writeFile(watchedFile, "second\n")
          local event, path = waitForEvent("sysPathChanged", 5000)
          assert.equals("sysPathChanged", event)
          -- the watcher can report the path with the platform's own separators
          assert.equals(watchedFile, (tostring(path):gsub("\\", "/")))

          -- one write can produce more than one notification, so let the rest of
          -- them arrive before the watch goes away: a straggler would otherwise
          -- look like the removed watch still reporting
          pumpEvents(250)
          assert.is_true(removeFileWatch(watchedFile))
          writeFile(watchedFile, "third\n")
          -- a watch that has been taken away must not report anything further,
          -- so this one is meant to time out
          assert.is_nil((waitForEvent("sysPathChanged", 750)))
        end)
      end)
    end)

    describe("Tests the dictionary functions", function()
      -- The words go into the profile's own dictionary file, which outlives the
      -- run, so every spec takes back out what it put in.
      local function withWords(...)
        local words = {...}
        finally(function()
          for _, word in ipairs(words) do
            removeWordFromDictionary(word)
          end
        end)
        for _, word in ipairs(words) do
          assert.is_true(addWordToDictionary(word), "could not add " .. word)
        end
      end

      local function indexOf(list, word)
        for index, entry in ipairs(list) do
          if entry == word then
            return index
          end
        end
      end

      describe("Tests the functionality of addWordToDictionary", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() addWordToDictionary() end, "addWordToDictionary: bad argument #1 type")
        end)

        it("adds a word that getDictionaryWordList then lists", function()
          withWords("mudletspecwibble")

          assert.is_not_nil(indexOf(getDictionaryWordList(), "mudletspecwibble"))
        end)

        it("returns nil+msg for a word that is already there", function()
          withWords("mudletspecwibble")

          local ok, err = addWordToDictionary("mudletspecwibble")
          assert.is_nil(ok)
          assert.is_true(contains(err, "already seems to be in the user dictionary"), tostring(err))
        end)
      end)

      describe("Tests the functionality of removeWordFromDictionary", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() removeWordFromDictionary() end, "removeWordFromDictionary: bad argument #1 type")
        end)

        it("takes the word back out of the list", function()
          withWords("mudletspecwibble")
          assert.is_not_nil(indexOf(getDictionaryWordList(), "mudletspecwibble"))

          assert.is_true(removeWordFromDictionary("mudletspecwibble"))

          assert.is_nil(indexOf(getDictionaryWordList(), "mudletspecwibble"))
        end)

        it("returns nil+msg for a word that was never added", function()
          local ok, err = removeWordFromDictionary("mudletspecnosuchword")
          assert.is_nil(ok)
          assert.is_true(contains(err, "does not seem to be in the user dictionary"), tostring(err))
        end)
      end)

      describe("Tests the functionality of getDictionaryWordList", function()
        it("returns the words sorted", function()
          withWords("mudletspeczebra", "mudletspecapple")

          local words = getDictionaryWordList()
          assert.is_table(words)
          local apple = indexOf(words, "mudletspecapple")
          local zebra = indexOf(words, "mudletspeczebra")
          assert.is_not_nil(apple)
          assert.is_not_nil(zebra)
          assert.is_true(apple < zebra, "the word list came back unsorted")
        end)
      end)

      describe("Tests the functionality of spellCheckWord", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() spellCheckWord() end, "spellCheckWord: bad argument #1 type")
        end)

        it("raises a Lua error when the dictionary choice is not a boolean", function()
          assertArgError(function() spellCheckWord("word", "user") end, "spellCheckWord: bad argument #2 type")
        end)

        it("knows a word that was added to the profile dictionary, and not one that was not", function()
          withWords("mudletspecwibble")

          assert.is_true(spellCheckWord("mudletspecwibble", true))
          assert.is_false(spellCheckWord("mudletspecwobble", true))
        end)

        it("answers from the system dictionary, or says it has none", function()
          local known, err = spellCheckWord("hello")
          if known == nil then
            assert.is_true(contains(err, "no main dictionaries found"), tostring(err))
            return
          end
          assert.is_boolean(known)
          -- which words a system dictionary knows depends on the language it is
          -- for, so the only answer worth asserting is for something no language
          -- spells that way
          assert.is_false(spellCheckWord("mudletspecqqzzxxvv"))
        end)
      end)

      describe("Tests the functionality of spellSuggestWord", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() spellSuggestWord() end, "spellSuggestWord: bad argument #1 type")
        end)

        it("raises a Lua error when the dictionary choice is not a boolean", function()
          assertArgError(function() spellSuggestWord("word", "user") end, "spellSuggestWord: bad argument #2 type")
        end)

        it("suggests a word the profile dictionary knows", function()
          withWords("mudletspecwibble")

          local suggestions = spellSuggestWord("mudletspecwobble", true)
          assert.is_table(suggestions)
          assert.is_not_nil(indexOf(suggestions, "mudletspecwibble"), "the added word was not suggested")
        end)

        it("returns a table from the system dictionary, or says it has none", function()
          local suggestions, err = spellSuggestWord("helo")
          if suggestions == nil then
            assert.is_true(contains(err, "no main dictionaries found"), tostring(err))
            return
          end
          assert.is_table(suggestions)
          for index, suggestion in ipairs(suggestions) do
            assert.is_string(suggestion, "suggestion " .. index .. " is not a word")
            assert.is_true(#suggestion > 0, "suggestion " .. index .. " is empty")
          end
        end)
      end)
    end)

    describe("Tests the functionality of unzipAsync", function()
      local extractDirectory = getMudletHomeDir() .. "/mudlet-spec-unzipped"

      local function removeExtractDirectory()
        if fileExists(extractDirectory .. "/readme.txt") then
          os.remove(extractDirectory .. "/readme.txt")
        end
        lfs.rmdir(extractDirectory)
      end

      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() unzipAsync() end, "unzipAsync: bad argument #1 type")
      end)

      it("raises a Lua error when given no place to extract to", function()
        assertArgError(function() unzipAsync("archive.zip") end, "unzipAsync: bad argument #2 type")
      end)

      it("extracts the archive and raises sysUnzipDone", function()
        if not testMode then
          pending("waiting for sysUnzipDone needs MUDLET_TEST_MODE")
          return
        end
        finally(removeExtractDirectory)
        local archive = fixtureDirectory .. "/mudlet-spec-emptyarchive.mpackage"

        assert.is_true(unzipAsync(archive, extractDirectory))

        local event, zipLocation, extractLocation = waitForEvent("sysUnzipDone", 10000)
        assert.equals("sysUnzipDone", event)
        assert.equals(archive, zipLocation)
        -- the extract location comes back with the trailing separator the
        -- function adds, whether or not the caller gave one
        assert.equals(extractDirectory .. "/", extractLocation)
        assert.is_true(fileExists(extractDirectory .. "/readme.txt"), "the archive was not unpacked")
      end)

      it("raises sysUnzipError for a file that is not an archive", function()
        if not testMode then
          pending("waiting for sysUnzipError needs MUDLET_TEST_MODE")
          return
        end
        finally(removeExtractDirectory)
        local notAnArchive = fixtureDirectory .. "/mudlet-spec-notazip.mpackage"

        -- the call itself cannot tell: unzipping happens on another thread, so
        -- it answers true and reports the failure through the event
        assert.is_true(unzipAsync(notAnArchive, extractDirectory))

        local event, zipLocation = waitForEvent("sysUnzipError", 10000)
        assert.equals("sysUnzipError", event)
        assert.equals(notAnArchive, zipLocation)
        assert.is_false(fileExists(extractDirectory .. "/readme.txt"))
      end)
    end)

    describe("Tests the functionality of loadReplay", function()
      -- A replay file is a run of (offset, length, bytes) records written by
      -- QDataStream, which is big-endian, so one can be built here rather than
      -- committed as a binary fixture.
      local function bigEndian32(value)
        return string.char(math.floor(value / 16777216) % 256, math.floor(value / 65536) % 256, math.floor(value / 256) % 256, value % 256)
      end

      local function writeReplay(path, payload)
        writeFile(path, bigEndian32(0) .. bigEndian32(#payload) .. payload)
      end

      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() loadReplay() end, "loadReplay: bad argument #1 type")
      end)

      it("returns nil+msg for a blank file name", function()
        local ok, err = loadReplay("")
        assert.is_nil(ok)
        assert.equals("a blank string is not a valid replay file name", err)
      end)

      it("returns nil+msg for a file that is not there", function()
        local ok, err = loadReplay(getMudletHomeDir() .. "/mudlet-spec-no-such-replay.dat")
        assert.is_nil(ok)
        assert.is_true(contains(err, "Cannot read file"), tostring(err))
      end)

      it("returns nil+msg for a file that is not a replay", function()
        local corrupt = getMudletHomeDir() .. "/mudlet-spec-corrupt-replay.dat"
        finally(function() os.remove(corrupt) end)
        writeFile(corrupt, "this is not a replay")

        local ok, err = loadReplay(corrupt)
        assert.is_nil(ok)
        assert.is_true(contains(err, "replay file seems to be corrupt"), tostring(err))
      end)

      it("plays the recorded bytes back into the main console", function()
        if not testMode then
          pending("letting the replay timer run needs MUDLET_TEST_MODE")
          return
        end
        local replay = getMudletHomeDir() .. "/mudlet-spec-replay.dat"
        finally(function() os.remove(replay) end)
        writeReplay(replay, "mudlet-spec-replayed-line\r\n")
        local mark = getLastLineNumber("main")

        assert.is_true(loadReplay(replay))

        local arrived = false
        for _ = 1, 40 do
          pumpEvents(50)
          arrived = contains(textFrom(mark), "mudlet-spec-replayed-line")
          if arrived then
            break
          end
        end
        assert.is_true(arrived, "the replay did not reach the console")
        -- whether a replay is running is application-wide, so let this one run
        -- out before the next spec asks for one
        pumpEvents(200)
      end)
    end)

    describe("Tests the functionality of findItems", function()
      -- Named items can only be made permanently, and a permanent item cannot
      -- be removed again from Lua, so the name matching below is checked
      -- against the nested triggers the run-tests package (the one running
      -- these specs) ships: "Test selectCaptureGroup with nested hierarchy" >
      -- "Filter" > "Not Filter" > "Trigger".
      local function ids(...)
        local found = findItems(...)
        assert.is_table(found)
        table.sort(found)
        return found
      end

      local function joined(list)
        return table.concat(list, ",")
      end

      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() findItems() end, "findItems: bad argument #1 type")
      end)

      it("raises a Lua error when given no item type", function()
        assertArgError(function() findItems("name") end, "findItems: bad argument #2 type")
      end)

      it("returns nil+msg for an item type it does not know", function()
        local ok, err = findItems("name", "sandwich")
        assert.is_nil(ok)
        assert.is_true(contains(err, "invalid item type 'sandwich' given"), tostring(err))
      end)

      it("returns an empty table when nothing matches", function()
        assert.same({}, findItems("mudletSpecNeverAnItem", "alias"))
        assert.same({}, findItems("mudletSpecNeverAnItem", "trigger"))
      end)

      it("returns the id of a temporary item, which is named after that id", function()
        local aliasId = tempAlias("^mudletSpecFindAlias$", function() end)
        local triggerId = tempTrigger("mudletSpecFindTrigger", function() end)
        finally(function()
          killAlias(tostring(aliasId))
          killTrigger(tostring(triggerId))
        end)

        assert.same({aliasId}, findItems(tostring(aliasId), "alias"))
        assert.same({triggerId}, findItems(tostring(triggerId), "trigger"))
      end)

      it("finds items of every kind it accepts", function()
        for _, itemType in ipairs({"timer", "trigger", "alias", "keybind", "button", "script"}) do
          assert.is_table(findItems("mudletSpecNeverAnItem", itemType), itemType .. " was not accepted")
        end
        -- the harness's own scripts are the ones that are always there
        assert.is_true(#findItems("test scripts", "script") > 0, "the run-tests package's scripts are not installed")
      end)

      it("matches by exactly the name it was given", function()
        local exact = ids("Filter", "trigger")
        assert.is_true(#exact > 0, "the run-tests package's nested triggers are not installed")
        assert.same({}, findItems("ilte", "trigger"))
      end)

      it("matches part of a name when not asked for an exact match", function()
        local exact = ids("Filter", "trigger")
        local notFilter = ids("Not Filter", "trigger")
        assert.is_true(#notFilter > 0)

        local partial = ids("Filter", "trigger", false)

        -- "Not Filter" only shows up once an exact match is no longer required
        assert.is_true(#partial > #exact, joined(partial))
        for _, id in ipairs(notFilter) do
          assert.is_truthy(table.contains(partial, id), "the partial match missed " .. id)
        end
      end)

      it("ignores case when asked to", function()
        local exact = ids("Filter", "trigger")

        assert.same({}, findItems("FILTER", "trigger"))
        assert.same(exact, ids("FILTER", "trigger", true, false))
      end)
    end)

    describe("Tests the functionality of insertHTML", function()
      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() insertHTML() end, "insertHTML: bad argument #1 type")
      end)

      it("inserts the text at the cursor", function()
        finally(function() moveCursorEnd() end)
        echo("mudlet-spec-insert-target\n")
        moveCursor(0, getLastLineNumber("main") - 1)

        assert.equals(0, select('#', insertHTML("mudlet-spec-inserted")))

        assert.equals("mudlet-spec-insertedmudlet-spec-insert-target", getCurrentLine())
      end)

      it("renders the markup it is given", function()
        -- BUG: insertHTML() hands the text straight to insertText(), so the
        -- markup its name and the wiki both promise is put on the line as
        -- literal characters. Left pending rather than pinning that as the
        -- contract.
        pending("insertHTML() does not interpret HTML, it is an alias for insertText")
        finally(function() moveCursorEnd() end)
        echo("mudlet-spec-html-target\n")
        moveCursor(0, getLastLineNumber("main") - 1)

        insertHTML("<b>mudlet-spec-bold</b>")

        assert.equals("mudlet-spec-boldmudlet-spec-html-target", getCurrentLine())
      end)
    end)

    describe("Tests the functionality of setMergeTables", function()
      it("raises a Lua error when a module is not a string", function()
        assertArgError(function() setMergeTables({}) end, "setMergeTables: bad argument #1 type")
      end)

      it("raises a Lua error naming the argument that is wrong", function()
        assertArgError(function() setMergeTables("MudletSpec.NeverAModule", {}) end, "setMergeTables: bad argument #2 type")
      end)

      it("returns nothing for any number of modules, including none", function()
        -- keys can be registered but never taken off again, so these are names
        -- no game will ever send rather than the real Char.* ones
        assert.equals(0, select('#', setMergeTables()))
        assert.equals(0, select('#', setMergeTables("MudletSpec.NeverAModule")))
        assert.equals(0, select('#', setMergeTables("MudletSpec.NeverAModule", "MudletSpec.NeverAnother")))
      end)

      it("merges the keys it was given into an incoming GMCP table", function()
        -- The merge only happens as GMCP or MSDP arrives from a server. Now
        -- that specs run offline, feedTelnet() can deliver a GMCP
        -- subnegotiation itself, so this is writable - just not written yet.
        pending("feeding the profile a GMCP subnegotiation is not written yet")
      end)
    end)

    describe("Tests the functionality of send", function()
      -- send() is registered from the C++ sendRaw(), which is the name its own
      -- error messages use.
      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() send() end, "sendRaw: bad argument #1 type")
      end)

      it("raises a Lua error when whether to show the command is not a boolean", function()
        assertArgError(function() send("mudletSpecSend", "yes") end, "sendRaw: bad argument #2 type")
      end)

      it("shows the command on the main console, unless told not to", function()
        -- whether the argument is listened to at all is the profile's to decide:
        -- the other two modes show every command, or none
        local originalMode = getConfig("showSentText", true)
        finally(function() setConfig("showSentText", originalMode) end)
        assert.is_true(setConfig("showSentText", "script"))
        local mark = getLastLineNumber("main")

        assert.is_true(send("mudletSpecShownCommand", true))

        assert.is_true(contains(textFrom(mark), "mudletSpecShownCommand"), textFrom(mark))

        mark = getLastLineNumber("main")
        assert.is_true(send("mudletSpecHiddenCommand", false))
        assert.is_false(contains(textFrom(mark), "mudletSpecHiddenCommand"), textFrom(mark))

        mark = getLastLineNumber("main")
        assert.is_true(send("mudletSpecDefaultCommand"))
        assert.is_true(contains(textFrom(mark), "mudletSpecDefaultCommand"), textFrom(mark))
      end)
    end)

    describe("Tests the functionality of denyCurrentSend", function()
      it("returns nothing", function()
        finally(function()
          -- the flag is consumed by the next send, so hand it one rather than
          -- leaving the following specs' sends blocked
          send("", false)
        end)

        assert.equals(0, select('#', denyCurrentSend()))
      end)

      it("stops the command that follows it from being sent", function()
        -- Nothing goes on the wire offline, so what makes a send observable is
        -- the warning Mudlet posts when a command cannot be encoded for the
        -- game: it is only reached once the send has been allowed. Switching
        -- the encoding first clears the once-per-encoding warning flag.
        local probeEncoding = (getServerEncoding() == "ISO 8859-1") and "ISO 8859-2" or "ISO 8859-1"
        finally(restoreServerEncoding())
        assert.is_true(setServerEncoding(probeEncoding))
        -- U+4E00, which no ISO 8859 encoding can represent
        local unencodable = "\228\184\128"

        denyCurrentSend()
        local mark = getLastLineNumber("main")
        send("mudletSpecDenied" .. unencodable, false)
        local afterDeny = textFrom(mark)

        mark = getLastLineNumber("main")
        send("mudletSpecAllowed" .. unencodable, false)
        local afterAllow = textFrom(mark)

        -- checked first: without it a silent deny spec would pass even if the
        -- warning had stopped being posted at all
        assert.is_true(contains(afterAllow, "mudletSpecAllowed"), "the allowed send was not reported, so this spec cannot tell the two apart")
        assert.is_false(contains(afterDeny, "mudletSpecDenied"), "the denied command was sent anyway")
      end)
    end)

    describe("Tests the functionality of isAncestorsActive", function()
      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() isAncestorsActive() end, "isAncestorsActive: bad argument #1 type")
      end)

      it("raises a Lua error when given no item type", function()
        assertArgError(function() isAncestorsActive(1) end, "isAncestorsActive: bad argument #2 type")
      end)

      it("returns nil+msg for a negative item ID", function()
        local ok, err = isAncestorsActive(-1, "alias")
        assert.is_nil(ok)
        assert.is_true(contains(err, "does not seem to be parseable as a positive integer"), tostring(err))
      end)

      it("returns nil+msg for an item that does not exist", function()
        local ok, err = isAncestorsActive(9999999, "alias")
        assert.is_nil(ok)
        assert.is_true(contains(err, "does not exist"), tostring(err))
      end)

      it("returns nil+msg for an item type it does not know", function()
        local ok, err = isAncestorsActive(1, "sandwich")
        assert.is_nil(ok)
        assert.is_true(contains(err, "invalid item type 'sandwich' given"), tostring(err))
      end)

      it("is true for a temporary item, which has no ancestors at all", function()
        local triggerId = tempTrigger("mudletSpecAncestorTrigger", function() end)
        local timerId = tempTimer(60, function() end)
        finally(function()
          killTrigger(tostring(triggerId))
          killTimer(timerId)
        end)

        assert.is_true(isAncestorsActive(triggerId, "trigger"))
        assert.is_true(isAncestorsActive(timerId, "timer"))
      end)

      it("follows the state of a nested item's parent group", function()
        -- Nesting needs permanent items, which cannot be removed again from
        -- Lua, so this uses the hierarchy the run-tests package (the one
        -- running these specs) already ships and puts its state back.
        local parentGroup = "Not Filter"
        local nested = findItems("Trigger", "trigger")
        -- both names have to be the harness's own, or this would be toggling
        -- some other package's trigger and asking about an unrelated item
        assert.equals(1, #nested, "expected exactly the run-tests package's nested 'Trigger'")
        assert.equals(1, #findItems(parentGroup, "trigger"), "expected exactly the run-tests package's '" .. parentGroup .. "' group")
        local childId = nested[1]
        finally(function() enableTrigger(parentGroup) end)

        assert.is_true(isAncestorsActive(childId, "trigger"))

        assert.is_true(disableTrigger(parentGroup))
        assert.is_false(isAncestorsActive(childId, "trigger"))

        assert.is_true(enableTrigger(parentGroup))
        assert.is_true(isAncestorsActive(childId, "trigger"))
      end)
    end)

    describe("Tests the functionality of getProfiles", function()
      it("lists this profile as loaded, with what it was set up with", function()
        local profiles = getProfiles()
        assert.is_table(profiles)

        local own = profiles[getProfileName()]
        assert.is_table(own, "the running profile is not in the list")
        assert.is_true(own.loaded)
        assert.is_boolean(own.connected)
        assert.equals(getProfileInformation(), own.description)
        if own.host then
          assert.is_string(own.host)
          assert.is_string(own.port)
        end

        assert.is_nil(profiles["mudlet-spec-never-a-profile"])
      end)

      it("lists a profile that is not loaded", function()
        local profilesDirectory = getMudletHomeDir():match("^(.*)[/\\]")
        assert.is_string(profilesDirectory, "could not work out the profiles folder from " .. getMudletHomeDir())
        local unloaded = profilesDirectory .. "/mudlet-spec-unloaded"
        -- a folder left behind would be listed as a profile by every later run,
        -- and by the connection dialog
        finally(function() assert.is_true(lfs.rmdir(unloaded), "could not remove " .. unloaded) end)
        assert.is_true(lfs.mkdir(unloaded))

        local entry = getProfiles()["mudlet-spec-unloaded"]
        assert.is_table(entry, "a profile folder that is not open was not listed")
        assert.is_false(entry.loaded)
        -- only a loaded profile has a connection to report on
        assert.is_nil(entry.connected)
        assert.equals("", entry.description)
      end)
    end)

    describe("Tests the functionality of getProfileStats", function()
      it("reports a count for every kind of item", function()
        local stats = getProfileStats()
        assert.is_table(stats)
        for _, kind in ipairs({"triggers", "aliases", "timers", "keys", "scripts"}) do
          assert.is_number(stats[kind].total, kind .. " has no total")
          assert.is_number(stats[kind].temp, kind .. " has no temp count")
          assert.is_number(stats[kind].active, kind .. " has no active count")
        end
        assert.is_number(stats.triggers.patterns.total)
        assert.is_number(stats.triggers.patterns.active)
        assert.is_number(stats.gifs.total)
      end)

      it("counts a temporary item that has just been created", function()
        local before = getProfileStats()
        local timerId = tempTimer(60, function() end)
        local triggerId = tempTrigger("mudletSpecStatsTrigger", function() end)
        finally(function()
          killTimer(timerId)
          killTrigger(triggerId)
        end)

        local after = getProfileStats()
        assert.equals(before.timers.total + 1, after.timers.total)
        assert.equals(before.timers.temp + 1, after.timers.temp)
        assert.equals(before.triggers.total + 1, after.triggers.total)
        assert.equals(before.triggers.temp + 1, after.triggers.temp)
      end)
    end)

    describe("Tests the profile icon functions", function()
      local iconSource = getMudletHomeDir() .. "/mudlet-spec-icon.png"
      local profileIcon = getMudletHomeDir() .. "/profileicon"

      -- The icon a player chose is theirs, and the profile outlives the run, so
      -- the specs below take a copy of it, work from a profile with no icon, and
      -- put the copy back.
      local function withNoProfileIcon()
        local original = readFile(profileIcon)
        finally(function()
          os.remove(iconSource)
          resetProfileIcon()
          if original then
            writeFile(profileIcon, original)
          end
        end)
        if original then
          assert.is_true(resetProfileIcon())
        end
        assert.is_false(fileExists(profileIcon), "the profile still has an icon")
      end

      describe("Tests the functionality of setProfileIcon", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() setProfileIcon() end, "setProfileIcon: bad argument #1 type")
        end)

        it("returns nil+msg for a blank path", function()
          local ok, err = setProfileIcon("")
          assert.is_nil(ok)
          assert.equals("a blank string is not a valid icon file path", err)
        end)

        it("returns nil+msg for a file that is not there", function()
          local ok, err = setProfileIcon(getMudletHomeDir() .. "/mudlet-spec-no-such-icon.png")
          assert.is_nil(ok)
          assert.is_true(contains(err, "doesn't exist"), tostring(err))
        end)

        it("copies the icon into the profile", function()
          withNoProfileIcon()
          writeFile(iconSource, "mudlet-spec-icon-bytes")

          assert.is_true(setProfileIcon(iconSource))

          assert.is_true(fileExists(profileIcon), "no icon was copied into the profile")
          assert.equals("mudlet-spec-icon-bytes", readFile(profileIcon))
        end)
      end)

      describe("Tests the functionality of resetProfileIcon", function()
        it("takes the icon back out of the profile", function()
          withNoProfileIcon()
          writeFile(iconSource, "mudlet-spec-icon-bytes")
          assert.is_true(setProfileIcon(iconSource))
          assert.is_true(fileExists(profileIcon))

          assert.is_true(resetProfileIcon())

          assert.is_false(fileExists(profileIcon), "the icon was left in the profile")
        end)

        it("is happy to be asked when there is no icon to remove", function()
          withNoProfileIcon()

          assert.is_true(resetProfileIcon())

          assert.is_false(fileExists(profileIcon))
        end)
      end)
    end)

    describe("Tests the functionality of raiseGlobalEvent", function()
      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() raiseGlobalEvent() end, "raiseGlobalEvent: missing argument #1")
      end)

      it("raises a Lua error for a first argument it cannot carry", function()
        assertArgError(function() raiseGlobalEvent({}) end, "raiseGlobalEvent: bad argument type #1")
      end)

      it("raises a Lua error for a later argument it cannot carry", function()
        -- the arguments are all vetted before the TEvent is built, so this raise
        -- has nothing to strand; the leak-checking CI job is what would notice
        -- if that changed
        assertArgError(function() raiseGlobalEvent("mudletSpecGlobalEvent", {}) end, "raiseGlobalEvent: bad argument type #2")
      end)

      it("does not deliver the event back to the profile that sent it", function()
        -- Only the half that one profile can see: that the sender is left out.
        -- Whether the other profiles receive it needs a second profile, so it
        -- belongs to the functional tests rather than here.
        local received = 0
        local handler = registerAnonymousEventHandler("mudletSpecGlobalEvent", function() received = received + 1 end)
        finally(function() killAnonymousEventHandler(handler) end)

        assert.is_true(raiseGlobalEvent("mudletSpecGlobalEvent", 1, "two", true, nil))

        if testMode then
          pumpEvents(200)
        end
        assert.equals(0, received)
        -- raiseEvent() is what a profile uses to talk to itself, and it proves
        -- the handler the count above is being read from does work
        raiseEvent("mudletSpecGlobalEvent")
        assert.equals(1, received)
      end)
    end)

    describe("Tests the functionality of wait", function()
      it("raises a Lua error when called with no arguments", function()
        assertArgError(function() wait() end, "Wait: wrong number of arguments")
      end)

      it("raises a Lua error when the delay is not a number", function()
        assertArgError(function() wait("soon") end, "Wait: bad argument #1 type")
      end)

      it("returns nothing and blocks for at least as long as it was asked to", function()
        local before = getEpoch()

        assert.equals(0, select('#', wait(10)))

        -- getEpoch() is in seconds; wait() blocks the whole thread, which is
        -- why nothing here waits any longer than it has to
        assert.is_true(getEpoch() - before >= 0.009)
      end)
    end)

    describe("Tests the functions whose effect needs a desktop or a person", function()
      -- These reach a browser, the system tray, a modal dialog or the physical
      -- keyboard, so only the refusals can be driven from here: every spec below
      -- gets the call turned away before it can do anything. That the call is
      -- reached at all is the point - it proves the function is registered and
      -- validates what it was handed.

      describe("Tests the functionality of openWebPage", function()
        it("raises a Lua error, rather than opening anything, when given no URL", function()
          assertArgError(function() openWebPage() end, "openWebPage: bad argument #1 type")
        end)
      end)

      describe("Tests the functionality of showNotification", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() showNotification() end, "showNotification: bad argument #1 type")
        end)

        it("raises a Lua error when the expiry time is not a number", function()
          assertArgError(function() showNotification("title", "message", "soon") end, "showNotification: bad argument #3 type")
        end)
      end)

      describe("Tests the functionality of invokeFileDialog", function()
        it("raises a Lua error, rather than opening a dialog, when not told what to ask for", function()
          assertArgError(function() invokeFileDialog() end, "invokeFileDialog: bad argument #1 type")
        end)

        it("raises a Lua error when given no title", function()
          assertArgError(function() invokeFileDialog(true) end, "invokeFileDialog: bad argument #2 type")
        end)
      end)

      describe("Tests the functionality of holdingModifiers", function()
        it("raises a Lua error when the modifier is not a number", function()
          -- what it answers depends on which keys are held down as the specs
          -- run, so only the refusal can be asserted on
          assertArgError(function() holdingModifiers("ctrl") end, "holdingModifiers: bad argument #1 type")
        end)
      end)

      describe("Tests the functionality of showHandlerError", function()
        it("raises a Lua error when called with no arguments", function()
          assertArgError(function() showHandlerError() end, "showHandlerError: bad argument #1 type")
        end)

        it("raises a Lua error when given no error message", function()
          -- where the message goes is the editor's error console and, only for a
          -- profile that opted into echoing Lua errors, the main console;
          -- neither can be turned on from Lua
          assertArgError(function() showHandlerError("mudletSpecEvent") end, "showHandlerError: bad argument #2 type")
        end)
      end)

      describe("Tests the functionality of clearCmdLineBlacklist", function()
        it("returns nil+msg for a command line that does not exist", function()
          local ok, err = clearCmdLineBlacklist("mudlet-spec-no-such-command-line")
          assert.is_nil(ok)
          assert.is_true(contains(err, "not found"), tostring(err))
        end)

        it("returns nothing for the main command line", function()
          -- what it cleared cannot be read back: there is no getter for a
          -- command line's blacklist
          assert.equals(0, select('#', clearCmdLineBlacklist()))
          assert.equals(0, select('#', clearCmdLineBlacklist("main")))
        end)

        it("still reads the command line name when something trails it", function()
          local ok, err = clearCmdLineBlacklist("mudlet-spec-no-such-command-line", "trailing")
          assert.is_nil(ok)
          assert.is_true(contains(err, "not found"), tostring(err))
        end)
      end)

      describe("Tests the functionality of showUnzipProgress", function()
        it("says it does nothing, having been removed", function()
          local ok, err = showUnzipProgress()
          assert.is_nil(ok)
          assert.equals("removed command, this function is now inactive and does nothing", err)
        end)
      end)
    end)

    describe("The Miscallaneous specs clean up after themselves", function()
      it("leaves no file or folder of its own behind", function()
        -- the specs above write into the profile, and one of them into the
        -- folder profiles live in, which no other spec file watches: anything
        -- left there would be listed as a profile from the next run onwards
        for entry in lfs.dir(getMudletHomeDir()) do
          assert.is_nil(entry:find("mudlet%-spec%-"), "left " .. entry .. " in the profile")
        end
        local profilesDirectory = getMudletHomeDir():match("^(.*)[/\\]")
        for entry in lfs.dir(profilesDirectory) do
          assert.is_nil(entry:find("mudlet%-spec%-"), "left " .. entry .. " among the profiles")
        end
      end)
    end)

  end)
