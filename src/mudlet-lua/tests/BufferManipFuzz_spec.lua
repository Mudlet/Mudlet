-- Fuzzer for Mudlet's console/buffer text-manipulation surface.
--
-- The telnet/trigger fuzzer (TelnetTriggerFuzz_spec.lua) drives bytes into the
-- parser. This one drives the *other* side: the TConsole / TBuffer text and
-- selection API that scripts use to rewrite what the parser produced -
-- selectString/selectSection, replace, deleteLine, moveCursor, insertText,
-- setFgColor/setBgColor over a selection, copy/paste, appendBuffer,
-- setWindowWrap, clearWindow. These carry line and column indices around, and
-- an index left stale by a deleteLine (or handed in out of range on purpose) is
-- a classic path to an out-of-bounds read/write that a sanitizer build turns
-- into an abort.
--
-- Strategy: build up a buffer from random fed lines (including OSC-8 hyperlinks
-- and multibyte, since links are tracked by line number and are known-fragile
-- under deleteLine), then hammer it with a random sequence of manipulation ops
-- whose arguments are frequently out of range - past the last line, negative,
-- wider than the line, into an empty window. Every op is wrapped so a Lua-level
-- rejection is fine; only a C++ memory error stops the run, which is the signal
-- we are hunting. Runs against both a main-window target and a miniconsole.
--
-- Reproducible via a self-contained MINSTD PRNG seeded from MUDLET_BUFFUZZ_SEED.
--   MUDLET_BUFFUZZ_SEED   integer seed          (default 1)
--   MUDLET_BUFFUZZ_ITERS  op batches per window (default 3000)
--   MUDLET_BUFFUZZ_DUMP   path; last op logged before it runs (crash repro)

-- Gated like TelnetTriggerFuzz_spec.lua: busted recurses into the whole tests
-- directory, and this spec leaves the consoles it fuzzes in arbitrary states.
if not os.getenv("MUDLET_FUZZ") then
    return
end

local floor = math.floor
local concat = table.concat
local insert = table.insert
local char = string.char

local function envnum(name, default)
    local v = tonumber(os.getenv(name) or "")
    return v and floor(v) or default
end

local SEED = envnum("MUDLET_BUFFUZZ_SEED", 1)
local ITERS = envnum("MUDLET_BUFFUZZ_ITERS", 3000)
local DUMP_PATH = os.getenv("MUDLET_BUFFUZZ_DUMP")

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

local opLog = {}
local function logOp(fmt, ...)
    if not DUMP_PATH then return end
    opLog[#opLog + 1] = string.format(fmt, ...)
    if #opLog > 40 then table.remove(opLog, 1) end       -- keep a short tail
    local f = io.open(DUMP_PATH, "w")
    if f then
        f:write(string.format("seed=%d\n", SEED))
        f:write(concat(opLog, "\n"))
        f:write("\n")
        f:close()
    end
end

-- A random, possibly out-of-range integer: mostly small, sometimes negative,
-- huge, or zero - the values that break index arithmetic.
local function wildInt()
    local k = rnd(10)
    if k < 5 then return rrange(0, 12)
    elseif k < 7 then return rrange(-5, -1)
    elseif k == 7 then return 0
    elseif k == 8 then return rrange(100, 100000)
    else return -rrange(100, 100000) end
end

-- A random short text, sometimes with multibyte, ANSI, an OSC-8 link, control
-- bytes or metacharacters - never a NUL (Lua C-string boundary).
local function wildText()
    local k = rnd(8)
    if k == 0 then return "" end
    if k == 1 then return string.rep(pick({"a", "x", "9", " "}), rrange(1, 30)) end
    if k == 2 then return pick({"τ", "日本語", "αβγ", "e\xcc\x81", "🜲🜳", "\xc3\xa9"}) end
    if k == 3 then return "\27[1;31mred\27[0m" end
    if k == 4 then return "\27]8;;http://example.com/" .. string.rep("a", rrange(0, 40)) .. "\27\\link\27]8;;\27\\" end
    if k == 5 then return pick({"[", "]", "(", ")", "|", "\\", "*", ".", "^", "$", "%1", "\t\t", "\r"}) end
    if k == 6 then
        local n = rrange(1, 8)
        local t = {}
        for i = 1, n do t[i] = char(rrange(1, 255)) end
        return concat(t)
    end
    return "word" .. rrange(0, 999)
end

-- Seed a window with a handful of random lines so there is something to select,
-- delete and rewrite.
local function seedLines(win, count)
    for _ = 1, count do
        local line = {}
        for _ = 1, rrange(1, 6) do insert(line, wildText()) end
        local text = concat(line, " ")
        if win == "main" then
            pcall(feedTriggers, "\n" .. text:gsub("[\r\n]", " ") .. "\n")
        else
            pcall(cecho, win, text .. "\n")
        end
    end
end

-- The op table. Each entry manipulates the given window with wild arguments.
-- All are pcall-guarded; the aim is to reach the C++ underneath, not to succeed.
local function callWin(fn, win, ...)
    if win == "main" then
        return pcall(fn, ...)
    else
        return pcall(fn, win, ...)
    end
end

local OPS = {
    function(win) local t, n = wildText(), wildInt(); logOp("selectString %s %q %d", win, t, n); callWin(selectString, win, t, n) end,
    function(win) local a, b = wildInt(), wildInt(); logOp("selectSection %s %d %d", win, a, b); callWin(selectSection, win, a, b) end,
    function(win) logOp("selectCurrentLine %s", win); callWin(selectCurrentLine, win) end,
    function(win) local t = wildText(); logOp("replace %s %q", win, t); callWin(replace, win, t) end,
    function(win) logOp("deleteLine %s", win); callWin(deleteLine, win) end,
    function(win) local a, b = wildInt(), wildInt(); logOp("moveCursor %s %d %d", win, a, b); callWin(moveCursor, win, a, b) end,
    function(win) logOp("moveCursorEnd %s", win); callWin(moveCursorEnd, win) end,
    function(win) local t = wildText(); logOp("insertText %s %q", win, t); callWin(insertText, win, t) end,
    function(win) logOp("setFgColor %s", win); callWin(setFgColor, win, rrange(0, 255), rrange(0, 255), rrange(0, 255)) end,
    function(win) logOp("setBgColor %s", win); callWin(setBgColor, win, rrange(0, 255), rrange(0, 255), rrange(0, 255)) end,
    function(win) logOp("resetFormat %s", win); callWin(resetFormat, win) end,
    function(win) logOp("copy %s", win); callWin(copy, win) end,
    function(win) logOp("paste %s", win); callWin(paste, win) end,
    function(win) logOp("appendBuffer %s", win); callWin(appendBuffer, win) end,
    function(win) local a, b = wildInt(), wildInt(); logOp("getLines %s %d %d", win, a, b); callWin(getLines, win, a, b) end,
    function(win) logOp("getColumnNumber %s", win); callWin(getColumnNumber, win) end,
    function(win) logOp("getLineNumber %s", win); callWin(getLineNumber, win) end,
    function(win) logOp("getLastLineNumber %s", win); callWin(getLastLineNumber, win) end,
    function(win) logOp("getCurrentLine %s", win); callWin(getCurrentLine, win) end,
    function(win) local n = wildInt(); logOp("setWindowWrap %s %d", win, n); callWin(setWindowWrap, win, n) end,
    function(win) local n = wildInt(); logOp("setWindowWrapIndent %s %d", win, n); callWin(setWindowWrapIndent, win, n) end,
    -- insertHTML takes the text alone and always writes to the main console,
    -- so there is no per-window form to drive through callWin()
    function(win) local t = "<b>" .. wildText() .. "</b>"; logOp("insertHTML main %q", t); pcall(insertHTML, t) end,
    function(win) logOp("clearWindow %s", win); callWin(clearWindow, win) end,
    function(win) logOp("feed-more %s", win); seedLines(win, rrange(1, 3)) end,
    function(win) local n = wildInt(); logOp("selectCaptureGroup %s %d", win, n); callWin(selectCaptureGroup, win, n) end,
    function(win) local t = wildText(); logOp("replaceLine %s %q", win, t); callWin(selectCurrentLine, win); callWin(replace, win, t) end,
}

local function fuzzWindow(win)
    seedLines(win, rrange(3, 10))
    for _ = 1, ITERS do
        pick(OPS)(win)
        -- occasionally re-seed so deleteLine cannot simply empty the window and
        -- leave every later op operating on nothing
        if chance(4) then seedLines(win, rrange(1, 3)) end
    end
    pcall(deselect, win == "main" and nil or win)
end

describe("fuzz: main console buffer manipulation", function()
    it("survives wild selection/edit/cursor ops on the main window", function()
        local t0 = os.clock()
        fuzzWindow("main")
        io.stderr:write(string.format("[buffuzz-timing] seed=%d main %.2fs\n", SEED, os.clock() - t0))
        assert.is_true(true)
    end)
end)

describe("fuzz: miniconsole buffer manipulation", function()
    it("survives wild selection/edit/cursor ops on a miniconsole", function()
        local t0 = os.clock()
        local win = "fuzzMiniConsole"
        pcall(createMiniConsole, win, 0, 0, 400, 300)
        fuzzWindow(win)
        pcall(clearWindow, win)
        io.stderr:write(string.format("[buffuzz-timing] seed=%d mini %.2fs\n", SEED, os.clock() - t0))
        assert.is_true(true)
    end)
end)
