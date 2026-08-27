-- Fuzzer for Mudlet's telnet parser and trigger engine.
--
-- Two attack surfaces are driven from Lua:
--   * the telnet parser, through feedTelnet(...) which under --offline runs the
--     real cTelnet::processSocketData() state machine (IAC negotiation,
--     subnegotiation for GMCP/MSDP/MSSP/MXP/CHARSET/MCCP, ANSI/CSI/OSC decoding
--     and the UTF-8/multibyte decoders), and
--   * the trigger engine, through the temp*Trigger() creators plus feedTriggers()
--     which runs TriggerUnit::processDataStream -> TTrigger::match (PCRE2 with
--     JIT, substring, begin-of-line, exact and colour match types, capture
--     extraction, /g multimatch, multiline and filter chains).
--
-- The suite binary is built with AddressSanitizer, so a memory error surfaces as
-- an abort with a diagnostic rather than as a silent corruption.
--
-- Reproducibility: every random choice comes from a self-contained MINSTD PRNG
-- seeded from MUDLET_FUZZ_SEED, so a given seed replays byte-for-byte on any
-- Lua 5.1 build (Lua's own math.random is not used - its algorithm and seeding
-- are not portable). Generation is a pure function of (seed, iteration), so any
-- iteration can be reproduced directly. When MUDLET_FUZZ_DUMP names a file, the
-- exact bytes of each feed are written there (hex) just before the feed, so
-- after an ASan abort the file's tail is the crashing input.
--
-- This is a diagnostic spec, run on demand rather than as part of the always-on
-- CI suite: it deliberately drives persistent cTelnet state (encoding,
-- negotiated options). The teardown resets what it can, but keep it out of a
-- shared self-test run unless you have isolated TESTS_DIRECTORY to it.
--
-- Tunables (all optional, via environment):
--   MUDLET_FUZZ_SEED    integer seed                       (default 1)
--   MUDLET_FUZZ_TRIG    trigger-engine rounds              (default 400)
--   MUDLET_FUZZ_TELNET  telnet-parser iterations           (default 2000)
--   MUDLET_FUZZ_DUMP    path to write the last fed input   (default none)

local floor = math.floor
local char = string.char
local concat = table.concat
local insert = table.insert

local function envnum(name, default)
    local v = tonumber(os.getenv(name) or "")
    return v and floor(v) or default
end

local SEED = envnum("MUDLET_FUZZ_SEED", 1)
local TRIG_ROUNDS = envnum("MUDLET_FUZZ_TRIG", 400)
local TELNET_ITERS = envnum("MUDLET_FUZZ_TELNET", 2000)
local DUMP_PATH = os.getenv("MUDLET_FUZZ_DUMP")

-- MINSTD (Park-Miller): state = 16807 * state mod 2^31-1. The product stays
-- under 2^53 so it is exact in a Lua double, giving a portable, repeatable
-- stream. Returns an integer in [1, 2147483646].
local function makeRng(seed)
    local s = seed % 2147483647
    if s <= 0 then
        s = s + 2147483646
    end
    return function()
        s = (16807 * s) % 2147483647
        return s
    end
end

local rng = makeRng(SEED)

local function rnd(n)            -- integer in [0, n-1]
    return rng() % n
end
local function rrange(lo, hi)    -- integer in [lo, hi]
    return lo + rng() % (hi - lo + 1)
end
local function pick(list)
    return list[1 + rng() % #list]
end
local function chance(pct)
    return rng() % 100 < pct
end

-- Record the exact bytes of a feed before performing it, so a hard crash leaves
-- the offending input on disk. Written fresh each call: the file always holds
-- just the most recent feed.
local function dumpInput(tag, iter, raw)
    if not DUMP_PATH then
        return
    end
    local f = io.open(DUMP_PATH, "w")
    if not f then
        return
    end
    local hex = {}
    for i = 1, #raw do
        hex[i] = string.format("%02x", raw:byte(i))
    end
    f:write(string.format("seed=%d phase=%s iter=%d len=%d\n%s\n", SEED, tag, iter, #raw, concat(hex, " ")))
    f:close()
end

-- Telnet control bytes and option codes.
local IAC, DONT, DO, WONT, WILL, SB, GA, SE = 255, 254, 253, 252, 251, 250, 249, 240
local ESC, BEL, CR, LF = 27, 7, 13, 10
local OPT = {
    BINARY = 0, ECHO = 1, SGA = 3, TTYPE = 24, EOR = 25, NAWS = 31,
    NEW_ENVIRON = 39, CHARSET = 42, MSDP = 69, MSSP = 70, MCCP1 = 85,
    MCCP2 = 86, MSP = 90, MXP = 91, AARD102 = 102, ATCP = 200, GMCP = 201,
}
local OPTION_VALUES = {}
for _, v in pairs(OPT) do
    insert(OPTION_VALUES, v)
end
-- MSDP framing bytes.
local MSDP_VAR, MSDP_VAL = 1, 2
local MSDP_TABLE_OPEN, MSDP_TABLE_CLOSE, MSDP_ARRAY_OPEN, MSDP_ARRAY_CLOSE = 3, 4, 5, 6

-- Turn a list of byte values (0-255) into a string suitable for feedTelnet.
-- feedTelnet takes the argument as a C string (a raw NUL would truncate it) and
-- then runs parseTelnetCodes over it, which treats <...> as escape tags. So a
-- NUL is emitted as the <00> tag and a literal '<'/'>' is doubled; every other
-- byte is passed through verbatim. The parser therefore receives exactly the
-- bytes in the list.
local function bytesToTelnet(bytes)
    local out = {}
    for i = 1, #bytes do
        local b = bytes[i]
        if b == 0 then
            out[i] = "<00>"
        elseif b == 0x3C then
            out[i] = "<<"
        elseif b == 0x3E then
            out[i] = ">>"
        else
            out[i] = char(b)
        end
    end
    return concat(out)
end

local function appendBytes(dst, src)
    for i = 1, #src do
        insert(dst, src[i])
    end
end

-- A random run of assorted bytes, optionally biased to printable ASCII.
local function fillBytes(dst, count, printableBias)
    for _ = 1, count do
        if printableBias and chance(70) then
            insert(dst, rrange(0x20, 0x7E))
        else
            insert(dst, rrange(1, 255))       -- never 0: NUL is injected on purpose elsewhere
        end
    end
end

-- One valid or deliberately broken UTF-8 code point.
local function appendUtf8(dst)
    local kind = rnd(8)
    if kind == 0 then
        insert(dst, rrange(0x20, 0x7E))                                   -- 1-byte
    elseif kind == 1 then
        insert(dst, rrange(0xC2, 0xDF)); insert(dst, rrange(0x80, 0xBF))  -- 2-byte
    elseif kind == 2 then
        insert(dst, rrange(0xE0, 0xEF)); insert(dst, rrange(0x80, 0xBF)); insert(dst, rrange(0x80, 0xBF)) -- 3-byte
    elseif kind == 3 then
        insert(dst, rrange(0xF0, 0xF4))                                   -- 4-byte
        insert(dst, rrange(0x80, 0xBF)); insert(dst, rrange(0x80, 0xBF)); insert(dst, rrange(0x80, 0xBF))
    elseif kind == 4 then
        insert(dst, rrange(0x80, 0xBF))                                   -- lone continuation
    elseif kind == 5 then
        insert(dst, rrange(0xC0, 0xC1)); insert(dst, rrange(0x80, 0xBF))  -- overlong lead
    elseif kind == 6 then
        insert(dst, rrange(0xF5, 0xFF))                                   -- out-of-range/invalid lead
    else
        insert(dst, 0xED); insert(dst, rrange(0xA0, 0xBF)); insert(dst, rrange(0x80, 0xBF)) -- UTF-16 surrogate range
    end
end

-- Telnet chunk generators: each appends one shaped fragment to a byte list.
local telnetChunks = {
    function(b)                                              -- plain-ish text with line breaks
        fillBytes(b, rrange(1, 40), true)
        if chance(60) then insert(b, CR); insert(b, LF) end
    end,
    function(b)                                              -- UTF-8 valid and broken
        for _ = 1, rrange(1, 16) do appendUtf8(b) end
    end,
    function(b)                                              -- IAC WILL/WONT/DO/DONT <option>
        insert(b, IAC); insert(b, pick({WILL, WONT, DO, DONT})); insert(b, pick(OPTION_VALUES))
    end,
    function(b)                                              -- IAC GA / IAC EOR / IAC NOP
        insert(b, IAC); insert(b, pick({GA, 239, 241}))
    end,
    function(b)                                              -- well-formed subnegotiation
        insert(b, IAC); insert(b, SB); insert(b, pick(OPTION_VALUES))
        for _ = 1, rrange(0, 40) do
            local v = rrange(1, 255)
            if v == IAC then insert(b, IAC) end             -- escape an IAC inside SB
            insert(b, v)
        end
        insert(b, IAC); insert(b, SE)
    end,
    function(b)                                              -- unterminated subnegotiation (carry-over / oversize path)
        insert(b, IAC); insert(b, SB); insert(b, pick(OPTION_VALUES))
        fillBytes(b, rrange(0, 60), true)
    end,
    function(b)                                              -- GMCP with random/broken JSON
        insert(b, IAC); insert(b, SB); insert(b, OPT.GMCP)
        local msg = pick({"Char.Vitals", "Room.Info", "Comm.Channel", "External.Discord.Status", "A.B.C.D"})
        for i = 1, #msg do insert(b, msg:byte(i)) end
        insert(b, 0x20)
        local json = pick({"{}", "{\"hp\":", "[1,2,", "{\"x\":{\"y\":", "not json", "{\"a\":1e999}", "\"\xff\xfe\""})
        for i = 1, #json do insert(b, json:byte(i)) end
        insert(b, IAC); insert(b, SE)
    end,
    function(b)                                              -- MSDP with framing bytes
        insert(b, IAC); insert(b, SB); insert(b, OPT.MSDP)
        for _ = 1, rrange(1, 4) do
            insert(b, MSDP_VAR)
            fillBytes(b, rrange(0, 8), true)
            insert(b, MSDP_VAL)
            if chance(30) then insert(b, pick({MSDP_TABLE_OPEN, MSDP_ARRAY_OPEN})) end
            fillBytes(b, rrange(0, 8), true)
            if chance(30) then insert(b, pick({MSDP_TABLE_CLOSE, MSDP_ARRAY_CLOSE})) end
        end
        insert(b, IAC); insert(b, SE)
    end,
    function(b)                                              -- CHARSET request that switches the decoder
        insert(b, IAC); insert(b, SB); insert(b, OPT.CHARSET); insert(b, 1)   -- REQUEST
        local sep = pick({0x20, 0x3B, 0x2C})
        local enc = pick({"UTF-8", "ISO-8859-1", "GBK", "Big5", "BOGUS-charset", ""})
        insert(b, sep)
        for i = 1, #enc do insert(b, enc:byte(i)) end
        insert(b, sep)
        insert(b, IAC); insert(b, SE)
    end,
    function(b)                                              -- MCCP2: negotiate then feed non-zlib garbage (inflate error path, self-recovers)
        insert(b, IAC); insert(b, WILL); insert(b, OPT.MCCP2)
        insert(b, IAC); insert(b, SB); insert(b, OPT.MCCP2); insert(b, IAC); insert(b, SE)
        fillBytes(b, rrange(1, 30), false)
    end,
    function(b)                                              -- ANSI SGR with extreme / malformed parameters
        insert(b, ESC); insert(b, 0x5B)                     -- ESC [
        local forms = {"0", "1;31", "38;5;255", "38;2;10;20;30", "48;5;999999", "38;2;", ";;;", "999999999", "1;2;3;4;5;6;7"}
        local p = pick(forms)
        for i = 1, #p do insert(b, p:byte(i)) end
        insert(b, 0x6D)                                     -- m
    end,
    function(b)                                              -- arbitrary CSI final byte
        insert(b, ESC); insert(b, 0x5B)
        for _ = 1, rrange(0, 8) do insert(b, pick({0x30, 0x31, 0x39, 0x3B, 0x3A})) end
        insert(b, rrange(0x40, 0x7E))                       -- final byte
    end,
    function(b)                                              -- OSC 8 hyperlink and other OSC strings
        insert(b, ESC); insert(b, 0x5D)                     -- ESC ]
        local code = pick({"8", "0", "52", "1337"})
        for i = 1, #code do insert(b, code:byte(i)) end
        insert(b, 0x3B)
        fillBytes(b, rrange(0, 20), true)
        insert(b, 0x3B)
        local uri = pick({"http://x", "mxp://foo", "javascript:x", "", "\xff\xfe"})
        for i = 1, #uri do insert(b, uri:byte(i)) end
        if chance(50) then insert(b, ESC); insert(b, 0x5C) else insert(b, BEL) end  -- ST or BEL
    end,
    function(b)                                              -- MXP negotiation + mode line + inline tags
        if chance(50) then insert(b, IAC); insert(b, pick({WILL, DO})); insert(b, OPT.MXP) end
        insert(b, ESC); insert(b, 0x5B)
        local mode = pick({"0", "1", "6", "7", "99"})
        for i = 1, #mode do insert(b, mode:byte(i)) end
        insert(b, 0x7A)                                     -- z
        local tag = pick({"<b>hi</b>", "<color red>x</color>", "<send 'go'>", "<a href=", "<<broken", "<font", "</unbalanced>"})
        for i = 1, #tag do insert(b, tag:byte(i)) end
    end,
    function(b)                                              -- scattered NULs and high control bytes
        for _ = 1, rrange(1, 6) do insert(b, pick({0, 0, ESC, 0x7F, 0x9B, IAC})) end
    end,
}

-- PCRE pattern generator, bounded so the classic catastrophic-backtracking
-- shapes ((X+)+ and friends) are never emitted here: at most one quantifier per
-- atom and no quantifier is ever applied to an already-quantified group. The
-- ReDoS surface is exercised separately and safely in its own block below.
local ATOMS = {
    "a", "b", "z", "\\d", "\\w", "\\s", ".", "[a-z]", "[0-9]", "[^x]",
    "\\.", "\\*", "foo", "\\p{L}", "\\x41", "(?:ab)", "\\bword\\b", "τ", "日",
}
local QUANTS = { "*", "+", "?", "{2}", "{1,3}", "{0,5}", "*?", "+?" }

local function genAtom()
    local atom = pick(ATOMS)
    if chance(50) then
        atom = atom .. pick(QUANTS)
    end
    return atom
end

local function genPattern()
    local parts = {}
    local n = rrange(1, 6)
    for _ = 1, n do
        if chance(25) then
            insert(parts, "(" .. genAtom() .. "|" .. genAtom() .. ")")   -- capture group with alternation
        else
            insert(parts, genAtom())
        end
    end
    local pat = concat(parts)
    if chance(30) then pat = "^" .. pat end
    if chance(30) then pat = pat .. "$" end
    if chance(15) then pat = "(?i)" .. pat end
    return pat
end

-- A random subject line. Never contains a NUL (feedTriggers takes the argument
-- as a C string); high bytes and control characters are used directly.
local function genSubject()
    local bytes = {}
    local n = rrange(0, 40)
    for _ = 1, n do
        local k = rnd(5)
        if k == 0 then
            insert(bytes, rrange(0x20, 0x7E))
        elseif k == 1 then
            appendUtf8(bytes)
        elseif k == 2 then
            insert(bytes, pick({0x61, 0x62, 0x20, 0x2E, 0x2A, 0x5B, 0x5D}))  -- regex-ish metacharacters
        elseif k == 3 then
            insert(bytes, ESC); insert(bytes, 0x5B); insert(bytes, 0x33); insert(bytes, 0x31); insert(bytes, 0x6D)  -- an ANSI colour run
        else
            insert(bytes, rrange(1, 255))
        end
    end
    local out = {}
    for i = 1, #bytes do
        local b = bytes[i]
        if b ~= 0 then out[#out + 1] = char(b) end
    end
    return concat(out)
end

-- Create a trigger of a random kind for a given pattern; returns its id or nil.
-- A minority carry a capture-reading script to force the Lua match table to be
-- materialised; the engine wraps script execution in pcall, so a script error
-- is logged rather than fatal.
local CAPTURE_SCRIPT = "if matches then _G.__fuzzMatchCount = (_G.__fuzzMatchCount or 0) + #matches end"

local function makeTrigger(name, pattern)
    local script = chance(20) and CAPTURE_SCRIPT or ""
    local kind = rnd(6)
    local ok, id
    if kind == 0 then
        ok, id = pcall(tempRegexTrigger, pattern, script)
    elseif kind == 1 then
        ok, id = pcall(tempTrigger, pattern, script)               -- substring
    elseif kind == 2 then
        ok, id = pcall(tempBeginOfLineTrigger, pattern, script)
    elseif kind == 3 then
        ok, id = pcall(tempExactMatchTrigger, pattern, script)
    elseif kind == 4 then
        -- complex regex: multiline, filter and /g match-all combinations; the
        -- numeric slots (fg,bg,hlFg,hlBg,sound) are passed as 0 to disable
        -- colour, highlight and sound so only the match machinery is exercised.
        local multiline = chance(30) and 1 or 0
        local filter = chance(30) and 1 or 0
        local matchAll = chance(40) and 1 or 0
        ok, id = pcall(tempComplexRegexTrigger, name, pattern, script,
                       multiline, 0, 0, filter, matchAll, 0, 0, 0, 0, 0)
    else
        -- colour trigger over ANSI colour numbers, mixed with a regex condition
        ok, id = pcall(tempColorTrigger, rrange(0, 16), rrange(0, 16), script)
    end
    if ok and type(id) == "number" and id > 0 then
        return id
    end
    return nil
end

local function killAll(ids)
    for i = 1, #ids do
        pcall(killTrigger, ids[i])
    end
end

describe("fuzz: trigger engine", function()
    it("survives random patterns matched against random subjects", function()
        local created = 0
        for round = 1, TRIG_ROUNDS do
            local ids = {}
            local setSize = rrange(1, 8)
            for k = 1, setSize do
                local id = makeTrigger(string.format("fuzzTrig_%d_%d", round, k), genPattern())
                if id then insert(ids, id); created = created + 1 end
            end
            for _ = 1, rrange(1, 20) do
                local subject = genSubject()
                local fed = "\n" .. subject .. "\n"
                dumpInput("trigger", round, fed)
                pcall(feedTriggers, fed)
            end
            killAll(ids)
        end
        deselect()
        assert.is_true(created > 0, "no triggers were created across the run - check the temp*Trigger APIs")
    end)
end)

describe("fuzz: telnet parser", function()
    after_each(function()
        deselect()
        pcall(feedTelnet, "\r\n")
        -- Reset the negotiated MCCP state the fuzz may have set, so it does not
        -- leak past this spec if the suite is ever run un-isolated.
        pcall(feedTelnet, "<T_IAC><T_WONT><O_MCCP2><T_IAC><T_WONT><O_MCCP>")
    end)

    it("survives random IAC / subnegotiation / ANSI / UTF-8 byte streams", function()
        local fedOk = 0
        for iter = 1, TELNET_ITERS do
            local bytes = {}
            for _ = 1, rrange(1, 6) do
                pick(telnetChunks)(bytes)
            end
            -- Periodically resync so an unterminated subnegotiation from a prior
            -- iteration cannot swallow this one whole.
            if iter % 25 == 0 then
                insert(bytes, IAC); insert(bytes, SE); insert(bytes, CR); insert(bytes, LF)
            end
            local raw = bytesToTelnet(bytes)
            dumpInput("telnet", iter, raw)
            local ok = pcall(feedTelnet, raw)
            if ok then fedOk = fedOk + 1 end
        end
        deselect()
        assert.is_true(fedOk > 0, "no telnet data was accepted - is the suite running with --offline?")
    end)
end)

-- Bounded probe of PCRE2 backtracking cost. TTrigger runs pcre2_match with a
-- null match context, so the question is whether a pathological pattern can hang
-- the single main thread. Empirically it cannot: PCRE2 applies its default
-- MATCH_LIMIT (10,000,000 steps) even with no context, so the cost of an evil
-- pattern plateaus (measured ~9-15ms worst case here) rather than exploding.
-- This probe grows the subject length only until a single match crosses a small
-- time budget, then stops - so it never risks the harness timeout even if a
-- future build were to remove that limit. It is informational: it asserts only
-- that each feed returned (no crash) and writes the timing growth to the dump
-- file for the run report to pick up.
describe("fuzz: PCRE backtracking probe", function()
    local EVIL = { "(a+)+$", "(x|x)*y", "(.*a){1,20}$" }

    it("measures match time growth on known-pathological patterns", function()
        for _, pat in ipairs(EVIL) do
            local id = tempRegexTrigger(pat, "")
            assert.is_number(id)
            local lastCost, worstN, worstCost = 0, 0, 0
            for n = 4, 34, 2 do
                local subject = "\n" .. string.rep("a", n) .. "b\n"
                local t0 = os.clock()
                pcall(feedTriggers, subject)
                local cost = os.clock() - t0
                worstN, worstCost = n, cost
                if DUMP_PATH then
                    local f = io.open(DUMP_PATH .. ".redos", "a")
                    if f then
                        f:write(string.format("pat=%q n=%d cost=%.4f\n", pat, n, cost))
                        f:close()
                    end
                end
                lastCost = cost
                if cost > 0.5 then break end                -- super-linear: stop before it hurts
            end
            pcall(killTrigger, id)
            assert.is_true(lastCost >= 0, string.format("pattern %q feed did not return", pat))
            -- worstCost/worstN retained for readability of a failing run's log
            assert.is_number(worstCost)
        end
        deselect()
    end)
end)
