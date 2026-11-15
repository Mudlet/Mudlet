-- Fuzzer for the C++ Lua API's argument handling.
--
-- The other fuzzers aim at one parser each. This one sweeps the functions
-- TLuaInterpreter registers, calling them with hostile arguments: wrong types,
-- missing and surplus arguments, out-of-range and non-finite numbers (which
-- truncate to arbitrary ints on the C++ side), empty and very long strings, and
-- the names of windows that do and do not exist. Each call is wrapped in pcall,
-- so a Lua error is fine; only a C++ memory error or undefined behaviour stops
-- the run, which a sanitizer build turns into an abort.
--
-- Functions that leave the process, touch the network or the disk, block, or
-- change the profile the busted run relies on are skipped (see SKIP below).
--
-- Reproducible via a self-contained MINSTD PRNG seeded from MUDLET_APIFUZZ_SEED.
--   MUDLET_APIFUZZ_SEED   integer seed                        (default 1)
--   MUDLET_APIFUZZ_ITERS  number of calls                     (default 20000)
--   MUDLET_APIFUZZ_ONLY   comma-separated function names to restrict the run to
--   MUDLET_APIFUZZ_DUMP   path; each call is logged before it runs (crash repro)

-- Gated like the other fuzz specs: busted recurses into the whole tests
-- directory, and this spec leaves windows, triggers and the map in arbitrary states.
if not os.getenv("MUDLET_FUZZ") then
    return
end

local floor = math.floor
local concat = table.concat
local insert = table.insert

local function envnum(name, default)
    local v = tonumber(os.getenv(name) or "")
    return v and floor(v) or default
end

local SEED = envnum("MUDLET_APIFUZZ_SEED", 1)
local ITERS = envnum("MUDLET_APIFUZZ_ITERS", 20000)
local ONLY = os.getenv("MUDLET_APIFUZZ_ONLY")
local DUMP_PATH = os.getenv("MUDLET_APIFUZZ_DUMP")

local function makeRng(seed)
    local s = seed % 2147483647
    if s <= 0 then s = s + 2147483646 end
    return function()
        s = (16807 * s) % 2147483647
        return s
    end
end
local rng = makeRng(SEED)
local function rnd(n) return rng() % n end
local function rrange(lo, hi) return lo + rng() % (hi - lo + 1) end
local function pick(list) return list[1 + rng() % #list] end
local function chance(pct) return rng() % 100 < pct end

local SKIP = {}
for name in ([[
  closeMudlet closeProfile resetProfile loadProfile setActiveProfile saveProfile
  connectToServer reconnect disconnect send sendSocket sendATCP sendGMCP sendMSDP
  sendTelnetChannel102 sendIrc openIRC restartIrc setIrcServer setIrcNick setIrcChannels
  spawn openUrl openWebPage installPackage installModule uninstallPackage uninstallModule
  reloadModule unzipAsync showUnzipProgress downloadFile getHTTP postHTTP putHTTP
  deleteHTTP customHTTP invokeFileDialog loadMap saveMap loadJsonMap saveJsonMap
  deleteMap loadReplay startLogging appendLog loadRawFile playMusicFile playSoundFile
  playVideoFile loadMusicFile loadSoundFile loadVideoFile purgeMediaCache receiveMSP
  wait waitForEvent pumpEvents setAppStyleSheet setProfileStyleSheet showNotification
  alert announce raiseEvent raiseGlobalEvent addFileWatch removeFileWatch setProfileIcon
  resetProfileIcon saveWindowLayout loadWindowLayout setConfig exportAreaImage
  addWordToDictionary removeWordFromDictionary setScript permScript setModuleInfo
  setModulePriority setModuleSync enableModuleSync disableModuleSync setPackageInfo
  setProfileInformation clearProfileInformation killTimer killTrigger killAlias killKey
  disableTimer disableTrigger disableAlias disableKey disableScript enableScript
  setDiscordApplicationID setDiscordGame setDiscordGameUrl setDiscordDetail
  setDiscordState setDiscordParty setDiscordLargeIcon setDiscordLargeIconText
  setDiscordSmallIcon setDiscordSmallIconText setDiscordElapsedStartTime
  setDiscordRemainingEndTime resetDiscordData setBackgroundImage setMovie
  setLabelCustomCursor setServerEncoding feedTelnet getLines setConsoleBufferSize
  setSaveCommandHistory expandAlias feedTriggers registerAnonymousEventHandler
  setClipboardText rearmLazyGlobals showHandlerError mudletOlderThan
  ttsSpeak ttsQueue ttsSkip ttsPause ttsResume ttsClearQueue ttsSetVoiceByName
  ttsSetVoiceByIndex ttsSetRate ttsSetPitch ttsSetVolume setMainWindowSize
  assert collectgarbage dofile error gcinfo getfenv getmetatable ipairs load loadfile
  loadstring module next pairs pcall print rawequal rawget rawset require select
  setfenv setmetatable tonumber tostring type unpack xpcall newproxy
]]):gmatch("%S+") do
    SKIP[name] = true
end

-- The registered C functions, whatever the Lua side has wrapped them in, are
-- still the ones defined in C; the Lua wrappers are covered by ordinary specs.
local function collectTargets()
    local only
    if ONLY and ONLY ~= "" then
        only = {}
        for name in ONLY:gmatch("[^,%s]+") do only[name] = true end
    end
    local names = {}
    for name, fn in pairs(_G) do
        if type(fn) == "function" and not SKIP[name] and (not only or only[name]) then
            local info = debug.getinfo(fn, "S")
            if info and info.what == "C" and name:match("^[a-z]") and not _G.string[name] then
                insert(names, name)
            end
        end
    end
    table.sort(names)
    return names
end

local WINDOWS = { "main", "fzMini", "fzLabel", "fzBuf", "fzUser", "fzCmd", "fzEdit", "fzScroll", "fzNope", "" }

local NUMBERS = {
    0, 1, -1, 2, 3, 7, 80, -80, 255, 256, 1000,
    2147483647, 2147483648, -2147483648, -2147483649, 4294967296,
    9007199254740992, -9007199254740992, 1e308, -1e308,
    math.huge, -math.huge, 0 / 0, 0.5, -0.5, 1e-300,
}

local LONG = string.rep("x", 5000)
local STRINGS = {
    "", "a", " ", "abc def", LONG, "\0", "a\0b", "\n\n", "%s%d%n", "#ff0000", "#zzzzzz",
    "<255,0,0>", "<red>text", "äöü€😀", "\255\254", "\xe2\x82", "^.*$", "(", "[", "*",
    "red", "blue", "0", "-1", "1e9", "nan", "true", "Monospace", "default", "fzStop",
}

local TABLES = {
    function() return {} end,
    function() return { 1, 2, 3 } end,
    function() return { -1, 2147483648, 0 / 0 } end,
    function() return { a = 1, b = "x" } end,
    function() return { "main", "", LONG } end,
    function() return { 255, 0, 0, 255 } end,
}

local function randomArg()
    local r = rnd(100)
    if r < 30 then
        if chance(30) then return rrange(-5, 120) end
        return pick(NUMBERS)
    elseif r < 50 then
        return pick(WINDOWS)
    elseif r < 75 then
        return pick(STRINGS)
    elseif r < 85 then
        return pick(TABLES)()
    elseif r < 92 then
        return chance(50)
    elseif r < 97 then
        return nil
    else
        return function() end
    end
end

local function typedArg(kind, argName)
    if kind == "number" then
        if chance(40) then return rrange(-5, 120) end
        return pick(NUMBERS)
    elseif kind == "string" then
        if argName and argName:find("name") and chance(70) then return pick(WINDOWS) end
        return chance(30) and pick(WINDOWS) or pick(STRINGS)
    elseif kind == "boolean" then
        return chance(50)
    elseif kind == "table" then
        return pick(TABLES)()
    elseif kind == "function" then
        return function() end
    end
    return randomArg()
end

-- "bad argument #2 type (x as number expected, got string!)" and its
-- "is optional" twin name the type the C++ side wanted at that position.
local KINDS = { "number", "string", "boolean", "table", "function" }
local function wantedType(msg)
    if type(msg) ~= "string" then return nil end
    local pos, argName, want = msg:match("bad argument #(%d+) type %((.-) as (.-) [ei][xs][pt]")
    if not pos then
        pos, want = msg:match("bad argument #(%d+) .-%((%a+) expected")
    end
    if not pos then return nil end
    local options = {}
    for _, k in ipairs(KINDS) do
        if want:find(k, 1, true) then insert(options, k) end
    end
    if #options == 0 then return nil end
    return tonumber(pos), pick(options), argName
end

local function show(v)
    local t = type(v)
    if t == "string" then
        if #v > 40 then
            return ("string.rep(%q, %d)"):format(v:sub(1, 1), #v)
        end
        return ("%q"):format(v):gsub("\n", "n")
    elseif t == "number" then
        if v ~= v then return "0/0" end
        if v == math.huge then return "math.huge" end
        if v == -math.huge then return "-math.huge" end
        return ("%.17g"):format(v)
    elseif t == "table" then
        local parts = {}
        for k, x in pairs(v) do
            insert(parts, (type(k) == "number" and "" or (k .. "=")) .. show(x))
        end
        return "{" .. concat(parts, ", ") .. "}"
    elseif t == "function" then
        return "function() end"
    end
    return tostring(v)
end

local dump
local function record(line)
    if dump then
        dump:write(line, "\n")
        dump:flush()
    end
end

local function setupCall(name, ...)
    local shown = {}
    for a = 1, select("#", ...) do
        shown[a] = show((select(a, ...)))
    end
    record(("%s(%s)"):format(name, concat(shown, ", ")))
    pcall(_G[name], ...)
end

local function setupWindows()
    setupCall("createMiniConsole", "fzMini", 0, 0, 200, 100)
    setupCall("createLabel", "fzLabel", 0, 0, 100, 50, 1)
    setupCall("createBuffer", "fzBuf")
    setupCall("openUserWindow", "fzUser")
    setupCall("createCommandLine", "fzCmd", 0, 0, 200, 20)
    setupCall("createTextEdit", "fzEdit", 0, 0, 200, 100)
    setupCall("createScrollBox", "fzScroll", 0, 0, 200, 100)
    setupCall("createStopWatch", "fzStop")
    for _, w in ipairs({ "fzMini", "fzBuf", "fzUser" }) do
        for i = 1, 5 do
            setupCall("echo", w, ("line %d äö€ some text\n"):format(i))
        end
    end
end

describe("Lua API argument fuzzer", function()
    it("survives hostile arguments to the registered C functions", function()
        if DUMP_PATH then
            dump = io.open(DUMP_PATH, "w")
        end
        local targets = collectTargets()
        print(("ApiArgFuzz: seed=%d iters=%d targets=%d"):format(SEED, ITERS, #targets))
        record(("-- seed=%d"):format(SEED))
        setupWindows()

        for i = 1, ITERS do
            if i % 2000 == 0 then
                -- Windows get deleted along the way; bring them back so later
                -- calls keep reaching the real code paths rather than "not found".
                setupWindows()
            end
            local name = pick(targets)
            local argc = rnd(7)
            local args = {}
            local shown = {}
            for a = 1, argc do
                args[a] = randomArg()
                shown[a] = show(args[a])
            end
            -- Retry with the argument type each rejection asks for, so calls
            -- get past the type checks into the code that uses the values.
            for _ = 1, 8 do
                record(("%s(%s)"):format(name, concat(shown, ", ")))
                local ok, r1, r2 = pcall(_G[name], unpack(args, 1, argc))
                local pos, kind, argName = wantedType(ok and r2 or r1)
                if not pos or pos > 12 or (args[pos] ~= nil and type(args[pos]) == kind) then
                    break
                end
                for a = argc + 1, pos - 1 do
                    shown[a] = "nil"
                end
                argc = math.max(argc, pos)
                args[pos] = typedArg(kind, argName)
                shown[pos] = show(args[pos])
            end
        end
        record("-- done")
        if dump then
            dump:close()
        end
        assert.is_true(true)
    end)
end)
