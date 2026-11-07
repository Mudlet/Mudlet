-- Fuzzer for the binary and JSON map file loaders.
--
-- Builds a map that uses most of what the format can carry (areas, exits,
-- special exits, doors, weights, locks, symbols, custom lines, labels, user
-- data, custom environment colors), saves it in each format version the save
-- side offers, then loads byte-level mutations of those files: bit flips,
-- interesting bytes and 32-bit values written over the data (QDataStream
-- prefixes every container and string with a 32-bit count), truncation, and
-- chunks deleted or repeated. The JSON export is mutated with its structure
-- in mind instead - numbers swapped for extreme ones, values for other JSON
-- types, strings for empty, long or lone-surrogate ones - so the result usually
-- still parses and reaches the import code behind the parser. A map that loads
-- is then read back through the
-- API, audited, path-found over and saved again, since a loader that accepts
-- inconsistent data hands the problem on to whatever reads it next.
--
-- Run it against a sanitizer build: a corrupted file should load or be refused,
-- never read or write out of bounds, overflow, or try to allocate the moon.
--
-- Reproducible via a self-contained MINSTD PRNG seeded from MUDLET_MAPFUZZ_SEED.
--   MUDLET_MAPFUZZ_SEED   integer seed                          (default 1)
--   MUDLET_MAPFUZZ_ITERS  number of mutated files loaded        (default 2000)
--   MUDLET_MAPFUZZ_FORMAT "binary", "json" or "both"            (default both)
--   MUDLET_MAPFUZZ_DUMP   path; each mutation is logged before it loads, and
--                         the file itself is copied to <path>.input, so after
--                         an abort that copy IS the crashing input (the test
--                         runner deletes the profile's copy on exit)
-- The base files embed the labels' rendered images, which differ with the
-- fonts installed, so a seed only replays on the machine that ran it - keep
-- the .input file rather than the seed.

-- Gated like the other fuzz specs: busted recurses into the whole tests
-- directory, and this spec replaces the profile's map many times over.
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

local SEED = envnum("MUDLET_MAPFUZZ_SEED", 1)
local ITERS = envnum("MUDLET_MAPFUZZ_ITERS", 2000)
local DUMP_PATH = os.getenv("MUDLET_MAPFUZZ_DUMP")
local FORMAT = os.getenv("MUDLET_MAPFUZZ_FORMAT") or "both"

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

local mapDirectory = getMudletHomeDir() .. "/map"
local currentPath = mapDirectory .. "/mapfilefuzz_spec_current.dat"
local currentJsonPath = mapDirectory .. "/mapfilefuzz_spec_current.json"
local backupPath = mapDirectory .. "/mapfilefuzz_spec_backup.dat"

local function readFile(path)
    local file = assert(io.open(path, "rb"))
    local data = file:read("*a")
    file:close()
    return data
end

local function writeFile(path, data)
    local file = assert(io.open(path, "wb"))
    file:write(data)
    file:close()
end

-- Best effort: a dump directory that does not exist just goes without
local function keepInput(data)
    local file = DUMP_PATH and io.open(DUMP_PATH .. ".input", "wb")
    if file then
        file:write(data)
        file:close()
    end
end

local dump
local function record(line)
    if dump then
        dump:write(line, "\n")
        dump:flush()
    end
end

-- QDataStream is big-endian
local function int32(value)
    if value < 0 then
        value = value + 2 ^ 32
    end
    local bytes = {}
    for i = 4, 1, -1 do
        bytes[i] = char(value % 256)
        value = floor(value / 256)
    end
    return concat(bytes)
end

local INTERESTING_BYTES = { 0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff }
local INTERESTING_INT32 = {
    0, 1, 2, 3, 16, 255, 256, 65535, 65536, 0x10000000, 0x3fffffff, 0x40000000,
    0x7ffffffe, 0x7fffffff, -2147483648, -2, -1,
}

local function mutate(data)
    local ops = {}
    for _ = 1, rrange(1, 4) do
        local len = #data
        if len == 0 then break end
        local kind = rnd(100)
        if kind < 25 then
            local at = rrange(1, len)
            local bit = 2 ^ rnd(8)
            local b = data:byte(at)
            local flipped = (floor(b / bit) % 2 == 1) and (b - bit) or (b + bit)
            data = data:sub(1, at - 1) .. char(flipped) .. data:sub(at + 1)
            insert(ops, ("flip@%d^%d"):format(at - 1, bit))
        elseif kind < 40 then
            local at = rrange(1, len)
            local b = pick(INTERESTING_BYTES)
            data = data:sub(1, at - 1) .. char(b) .. data:sub(at + 1)
            insert(ops, ("byte@%d=%d"):format(at - 1, b))
        elseif kind < 70 and len >= 4 then
            local at = rrange(1, len - 3)
            local v = pick(INTERESTING_INT32)
            data = data:sub(1, at - 1) .. int32(v) .. data:sub(at + 4)
            insert(ops, ("int32@%d=%d"):format(at - 1, v))
        elseif kind < 80 then
            local cut = rrange(0, len - 1)
            data = data:sub(1, cut)
            insert(ops, ("truncate@%d"):format(cut))
        elseif kind < 90 then
            local at = rrange(1, len)
            local n = rrange(1, math.min(64, len - at + 1))
            data = data:sub(1, at - 1) .. data:sub(at + n)
            insert(ops, ("delete@%d+%d"):format(at - 1, n))
        else
            local at = rrange(1, len)
            local n = rrange(1, math.min(64, len - at + 1))
            data = data:sub(1, at + n - 1) .. data:sub(at, at + n - 1) .. data:sub(at + n)
            insert(ops, ("repeat@%d+%d"):format(at - 1, n))
        end
    end
    return data, concat(ops, " ")
end

local JSON_NUMBERS = {
    "0", "-1", "1", "2147483647", "2147483648", "-2147483648", "-2147483649", "4294967296",
    "9007199254740993", "1e308", "-1e308", "1e-320", "0.5", "-0.5", "99999999999999999999999",
}
local JSON_VALUES = { "null", "true", "false", "\"\"", "\"x\"", "[]", "{}", "[[]]", "{\"a\":1}", "[1,2,3]", "[-1]" }
local JSON_STRINGS = { "", "\\ud800", "\\udc00x", string.rep("y", 3000), "\\u0000", "\\n\\t" }

-- start/end offsets of every number literal, and of every string literal's contents
local function jsonTokens(data)
    local numbers, strings = {}, {}
    local i, len = 1, #data
    while i <= len do
        local c = data:sub(i, i)
        if c == '"' then
            local j = i + 1
            while j <= len do
                local d = data:sub(j, j)
                if d == "\\" then
                    j = j + 2
                elseif d == '"' then
                    break
                else
                    j = j + 1
                end
            end
            insert(strings, { i + 1, j - 1 })
            i = j + 1
        elseif c:match("[%-%d]") then
            local s, e = data:find("^%-?%d+%.?%d*[eE]?[%+%-]?%d*", i)
            if s then
                insert(numbers, { s, e })
                i = e + 1
            else
                i = i + 1
            end
        else
            i = i + 1
        end
    end
    return numbers, strings
end

local function mutateJson(data)
    local ops = {}
    for _ = 1, rrange(1, 4) do
        local numbers, strings = jsonTokens(data)
        local kind = rnd(100)
        if kind < 45 and #numbers > 0 then
            local t = pick(numbers)
            local v = pick(JSON_NUMBERS)
            data = data:sub(1, t[1] - 1) .. v .. data:sub(t[2] + 1)
            insert(ops, ("num@%d=%s"):format(t[1] - 1, v))
        elseif kind < 70 and #numbers > 0 then
            local t = pick(numbers)
            local v = pick(JSON_VALUES)
            data = data:sub(1, t[1] - 1) .. v .. data:sub(t[2] + 1)
            insert(ops, ("type@%d=%s"):format(t[1] - 1, v))
        elseif kind < 80 and #strings > 0 then
            -- a key as often as a value, which renames or blanks the field
            local t = pick(strings)
            local v = pick(JSON_STRINGS)
            data = data:sub(1, t[1] - 1) .. v .. data:sub(t[2] + 1)
            insert(ops, ("str@%d=%s"):format(t[1] - 1, v:sub(1, 12)))
        elseif kind < 90 and #strings > 0 then
            -- the string and whatever value follows it become another type
            local t = pick(strings)
            local _, colonEnd = data:find("^%s*:%s*", t[2] + 2)
            if colonEnd then
                local first = data:sub(colonEnd + 1, colonEnd + 1)
                local closer = ({ ["["] = "]", ["{"] = "}" })[first]
                if closer then
                    local depth, j = 0, colonEnd + 1
                    repeat
                        local d = data:sub(j, j)
                        if d == first then depth = depth + 1 elseif d == closer then depth = depth - 1 end
                        j = j + 1
                    until depth == 0 or j > #data
                    local v = pick(JSON_VALUES)
                    data = data:sub(1, colonEnd) .. v .. data:sub(j)
                    insert(ops, ("container@%d=%s"):format(colonEnd, v))
                end
            end
        else
            local m
            data, m = mutate(data)
            insert(ops, m)
        end
    end
    return data, concat(ops, " ")
end

local function buildMap()
    local areas = {}
    for a = 1, 3 do
        areas[a] = addAreaName("MapFileFuzz area " .. a)
        setAreaUserData(areas[a], "key" .. a, "value äö€ " .. a)
    end
    local rooms = {}
    for i = 1, 24 do
        local id = createRoomID()
        addRoom(id)
        rooms[i] = id
        setRoomArea(id, areas[(i % 3) + 1])
        setRoomCoordinates(id, i % 5 - 2, floor(i / 5) - 2, i % 2)
        setRoomName(id, "room " .. i .. " ☃")
        setRoomEnv(id, (i % 4) + 1)
        setRoomUserData(id, "note", string.rep("x", i))
        setRoomChar(id, i % 2 == 0 and "★" or "a")
        setRoomWeight(id, i)
    end
    -- setExit()'s numbering, 1 north to 12 out, and each direction's opposite
    local DIRECTIONS = { "n", "ne", "nw", "e", "w", "s", "se", "sw", "up", "down", "in", "out" }
    local OPPOSITE = { 6, 8, 7, 5, 4, 1, 3, 2, 10, 9, 12, 11 }
    for i = 1, #rooms - 1 do
        local dir = ((i - 1) % 12) + 1
        local name = DIRECTIONS[dir]
        assert(setExit(rooms[i], rooms[i + 1], dir))
        assert(setExit(rooms[i + 1], rooms[i], OPPOSITE[dir]))
        assert(addSpecialExit(rooms[i], rooms[(i * 7) % #rooms + 1], "go hole " .. i))
        if i % 3 == 0 then
            assert(setDoor(rooms[i], name, (i % 3) + 1))
            assert(setExitWeight(rooms[i], name, i))
            lockExit(rooms[i], name, true)
            assert(hasExitLock(rooms[i], name))
        end
        if i % 5 == 0 then
            assert(addCustomLine(rooms[i], { { i, i, 0 }, { i + 1, i, 0 } }, name, "dot line", { 255, 0, 0 }, true))
            -- a stub needs a direction with no exit: not this one, nor the way back from the previous room
            local taken = { [dir] = true, [OPPOSITE[((i - 2) % 12) + 1]] = true }
            local stub = 1
            while taken[stub] do
                stub = stub + 1
            end
            setExitStub(rooms[i], DIRECTIONS[stub], true)
            assert(next(getExitStubs1(rooms[i]) or {}), "the exit stub was refused")
            lockRoom(rooms[i], true)
        end
    end
    setCustomEnvColor(260, 10, 20, 30, 255)
    setCustomEnvColor(261, 200, 100, 0, 128)
    setMapUserData("fuzz", "map user data")
    for a = 1, 3 do
        createMapLabel(areas[a], "label " .. a, a, a, 0, 255, 255, 0, 0, 0, 0, 1, 10, true, true, "Monospace", 255, 0)
    end
    return rooms
end

local function exerciseLoadedMap()
    local areaTable = getAreaTable() or {}
    local ids = {}
    for id in pairs(getRooms() or {}) do
        insert(ids, id)
        if #ids >= 40 then break end
    end
    for _, areaId in pairs(areaTable) do
        pcall(getAreaRooms, areaId)
        pcall(getMapLabels, areaId)
        pcall(getAreaExits, areaId, true)
        pcall(getAllAreaUserData, areaId)
    end
    for _, id in ipairs(ids) do
        pcall(getRoomExits, id)
        pcall(getSpecialExits, id)
        pcall(getDoors, id)
        pcall(getExitWeights, id)
        pcall(getExitStubs1, id)
        pcall(getCustomLines, id)
        pcall(getAllRoomUserData, id)
        pcall(getRoomCoordinates, id)
        pcall(getRoomChar, id)
        pcall(getRoomName, id)
        pcall(getAllRoomEntrances, id)
    end
    if #ids >= 2 then
        pcall(getPath, ids[1], ids[#ids])
    end
    pcall(getAllMapUserData)
    pcall(getCustomEnvColorTable)
    pcall(auditAreas)
    pcall(saveMap, mapDirectory .. "/mapfilefuzz_spec_resaved.dat")
end

describe("Map file loader fuzzer", function()
    it("survives loading mutated binary map files", function()
        if DUMP_PATH then
            dump = io.open(DUMP_PATH, "w")
        end
        print(("MapFileFuzz: seed=%d iters=%d"):format(SEED, ITERS))
        record(("-- seed=%d"):format(SEED))

        assert.is_true(saveMap(backupPath))
        -- busted keeps only the last finally() a test registers, so this is the one
        finally(function()
            if dump then
                dump:close()
            end
            loadMap(backupPath)
        end)
        deleteMap()
        buildMap()

        local bases = {}
        if FORMAT ~= "json" then
            for _, version in ipairs({ 17, 18, 19, 20 }) do
                local path = ("%s/mapfilefuzz_spec_v%d.dat"):format(mapDirectory, version)
                if saveMap(path, version) then
                    insert(bases, { version = "v" .. version, data = readFile(path) })
                end
            end
        end
        if FORMAT ~= "binary" then
            -- the JSON functions refuse until a mapper has been shown
            openMapWidget()
            local path = mapDirectory .. "/mapfilefuzz_spec_base.json"
            assert.is_true(saveJsonMap(path))
            insert(bases, { version = "json", data = readFile(path) })
        end
        assert.is_true(#bases > 0, "no base map file could be saved")

        local loaded = 0
        for i = 1, ITERS do
            local base = pick(bases)
            local isJson = base.version == "json"
            local data, ops
            if isJson then
                data, ops = mutateJson(base.data)
            else
                data, ops = mutate(base.data)
            end
            record(("%d %s %s"):format(i, base.version, ops))
            keepInput(data)
            local path = isJson and currentJsonPath or currentPath
            writeFile(path, data)
            local ok
            if isJson then
                ok = loadJsonMap(path)
            else
                ok = loadMap(path)
            end
            -- paint now, so a report from drawing this map follows its dump record
            pumpEvents(0)
            if ok then
                loaded = loaded + 1
                exerciseLoadedMap()
                -- and what reading it queued, before the next record
                pumpEvents(0)
            end
        end
        print(("MapFileFuzz: %d of %d mutated files loaded"):format(loaded, ITERS))
        record("-- done")
    end)
end)
