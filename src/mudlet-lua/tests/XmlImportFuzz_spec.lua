-- Fuzzer for the XML importer: map files read by loadMap(".xml") and
-- packages installed from a bare .xml.
--
-- Takes the map and package fixtures as a corpus and mutates them with XML in
-- mind, so most results still parse and reach XMLimport's readers rather than
-- stopping at the first well-formedness error: attribute values and element
-- text swapped for extreme numbers, empty, long or odd strings and character
-- references; elements deleted, repeated or renamed; start or end tags dropped;
-- and now and then plain byte damage. A map that loads is read back through the
-- API and path-found over; a package that installs is uninstalled again.
--
-- Run it against a sanitizer build: a damaged file should load or be refused,
-- never read or write out of bounds or overflow.
--
-- Reproducible via a self-contained MINSTD PRNG seeded from MUDLET_XMLFUZZ_SEED.
--   MUDLET_XMLFUZZ_SEED   integer seed                         (default 1)
--   MUDLET_XMLFUZZ_ITERS  number of mutated files imported     (default 1000)
--   MUDLET_XMLFUZZ_MODE   "map", "package" or "both"           (default both)
--   MUDLET_XMLFUZZ_DUMP   path; each mutation is logged before it is imported,
--                         and the file itself is copied to <path>.input, so
--                         after an abort that copy IS the crashing input (the
--                         test runner deletes the profile's copy on exit)

-- Gated like the other fuzz specs: busted recurses into the whole tests
-- directory, and this spec replaces the profile's map and installs packages.
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

local SEED = envnum("MUDLET_XMLFUZZ_SEED", 1)
local ITERS = envnum("MUDLET_XMLFUZZ_ITERS", 1000)
local MODE = os.getenv("MUDLET_XMLFUZZ_MODE") or "both"
local DUMP_PATH = os.getenv("MUDLET_XMLFUZZ_DUMP")

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

local specDirectory = debug.getinfo(1, "S").source:match("^@(.*)[/\\]")
local fixtures = specDirectory .. "/fixtures"
local home = getMudletHomeDir()
local mapPath = home .. "/xmlimportfuzz_map.xml"
local packageName = "xmlimportfuzzpkg"
local packagePath = home .. "/" .. packageName .. ".xml"
local mapBackupPath = home .. "/map/xmlimportfuzz_backup.dat"

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

-- Every direction, door and attribute readRoom() knows, so the corpus reaches
-- each of its branches before any mutation.
local RICH_MAP = [[<?xml version="1.0" encoding="UTF-8"?>
<map>
  <areas>
    <area id="7001" name="Xml Fuzz A"/>
    <area id="7002" name="Xml Fuzz B"/>
  </areas>
  <rooms>
    <room id="7001" area="7001" title="One" environment="5">
      <coord x="0" y="0" z="0"/>
      <exit direction="north" target="7002"/>
      <exit direction="northeast" target="7003" door="1"/>
      <exit direction="east" target="7004" door="2"/>
      <exit direction="southeast" target="7005" door="3"/>
      <exit direction="south" target="7002" hidden="1"/>
      <exit direction="southwest" target="7003"/>
      <exit direction="west" target="7004"/>
      <exit direction="northwest" target="7005"/>
      <exit direction="up" target="7002"/>
      <exit direction="down" target="7003"/>
      <exit direction="in" target="7004"/>
      <exit direction="out" target="7005"/>
      <exit special="1" command="climb tree" target="7002"/>
      <features>
        <feature type="shop"/>
        <feature type="bank"/>
      </features>
    </room>
    <room id="7002" area="7001" title="Two" environment="6">
      <coord x="1" y="-1" z="0"/>
      <exit direction="south" target="7001"/>
    </room>
    <room id="7003" area="7002" title="Three" environment="7">
      <coord x="-3" y="2" z="1"/>
      <exit direction="up" target="7001"/>
    </room>
    <room id="7004" area="7002" title="Four">
      <coord x="2" y="2" z="-1"/>
    </room>
    <room id="7005" area="7001" title="Five" environment="300">
      <coord x="" y="" z=""/>
    </room>
  </rooms>
  <environments>
    <environment id="5" color="1"/>
    <environment id="6" color="15"/>
    <environment id="300" color="200"/>
  </environments>
</map>
]]

local PACKAGE_FIXTURES = {
    "mudlet-spec-minimal", "mudlet-spec-nested", "mudlet-spec-varsonly", "mudlet-spec-xmlonly",
    "mudlet-spec-brokenscripts", "mudlet-spec-colorfilter", "mudlet-spec-treeindex", "mudlet-spec-triggerkinds",
}

local VALUES = {
    "", "0", "1", "-1", "2", "3", "4", "12", "2147483647", "2147483648", "-2147483648", "-2147483649",
    "4294967296", "99999999999999999999", "1e9", "-0", "0x10", " 7 ", "abc", "yes", "true",
    string.rep("z", 4000), "&amp;", "&lt;&gt;", "&#0;", "&#xD800;", "&#x1F600;", "\0", "äö€😀",
    "^(.*)$", "(", "[", "*", "%s",
}

-- Start offset, end offset (inclusive) of every attribute value, every run of
-- text between tags, and every start tag (through its ">") with its name.
local function scan(data)
    local attrs, texts, tags = {}, {}, {}
    for s, e in data:gmatch('="()[^"]*()"') do
        insert(attrs, { s, e - 1 })
    end
    for s, text, e in data:gmatch(">()([^<]+)()<") do
        if text:find("%S") then
            insert(texts, { s, e - 1 })
        end
    end
    for s, name, e in data:gmatch("()<([%a_][%w_%-%.]*)[^>]*()>") do
        insert(tags, { s, e, name })
    end
    return attrs, texts, tags
end

-- The element a start tag opens, through its matching end tag, or the tag
-- alone if it closes itself or no end tag is found.
local function elementRange(data, tag)
    if data:sub(tag[2] - 1, tag[2] - 1) == "/" then
        return tag[1], tag[2]
    end
    local name = tag[3]
    local depth, pos = 1, tag[2] + 1
    while depth > 0 do
        local s, e, closing, found = data:find("<(/?)([%a_][%w_%-%.]*)[^>]*>", pos)
        if not s then
            return tag[1], tag[2]
        end
        if found == name then
            if closing == "/" then
                depth = depth - 1
            elseif data:sub(e - 1, e - 1) ~= "/" then
                depth = depth + 1
            end
        end
        if depth == 0 then
            return tag[1], e
        end
        pos = e + 1
    end
end

local function replaceRange(data, s, e, with)
    return data:sub(1, s - 1) .. with .. data:sub(e + 1)
end

local function mutate(data)
    local ops = {}
    for _ = 1, rrange(1, 3) do
        local attrs, texts, tags = scan(data)
        local kind = rnd(100)
        if kind < 30 and #attrs > 0 then
            local a = pick(attrs)
            local v = pick(VALUES)
            data = replaceRange(data, a[1], a[2], (v:gsub('"', "&quot;")))
            insert(ops, ("attr@%d=%q"):format(a[1] - 1, v:sub(1, 16)))
        elseif kind < 50 and #texts > 0 then
            local t = pick(texts)
            local v = pick(VALUES)
            data = replaceRange(data, t[1], t[2], (v:gsub("<", "&lt;")))
            insert(ops, ("text@%d=%q"):format(t[1] - 1, v:sub(1, 16)))
        elseif kind < 62 and #tags > 1 then
            local s, e = elementRange(data, pick(tags))
            data = replaceRange(data, s, e, "")
            insert(ops, ("delete@%d+%d"):format(s - 1, e - s + 1))
        elseif kind < 74 and #tags > 1 then
            local s, e = elementRange(data, pick(tags))
            data = data:sub(1, e) .. data:sub(s, e) .. data:sub(e + 1)
            insert(ops, ("repeat@%d+%d"):format(s - 1, e - s + 1))
        elseif kind < 82 and #tags > 1 then
            -- another element's name, so a reader meets a child it does not expect there
            local t = pick(tags)
            local other = pick(tags)[3]
            data = replaceRange(data, t[1] + 1, t[1] + #t[3], other)
            insert(ops, ("rename@%d=%s"):format(t[1] - 1, other))
        elseif kind < 90 and #tags > 1 then
            local t = pick(tags)
            data = replaceRange(data, t[1], t[2], "")
            insert(ops, ("droptag@%d"):format(t[1] - 1))
        else
            local len = #data
            if len > 0 then
                local at = rrange(1, len)
                if chance(50) then
                    data = data:sub(1, at - 1)
                    insert(ops, ("truncate@%d"):format(at - 1))
                else
                    local b = pick({ 0, 0x3c, 0x3e, 0x22, 0x26, 0xff, 0xc3 })
                    data = data:sub(1, at - 1) .. char(b) .. data:sub(at + 1)
                    insert(ops, ("byte@%d=%d"):format(at - 1, b))
                end
            end
        end
    end
    return data, concat(ops, " ")
end

local function exerciseMap()
    local ids = {}
    for id in pairs(getRooms() or {}) do
        insert(ids, id)
        if #ids >= 20 then break end
    end
    for _, areaId in pairs(getAreaTable() or {}) do
        pcall(getAreaRooms, areaId)
        pcall(getAreaExits, areaId, true)
    end
    for _, id in ipairs(ids) do
        pcall(getRoomExits, id)
        pcall(getSpecialExits, id)
        pcall(getDoors, id)
        pcall(getAllRoomUserData, id)
        pcall(getRoomCoordinates, id)
        pcall(getAllRoomEntrances, id)
    end
    if #ids >= 2 then
        pcall(getPath, ids[1], ids[#ids])
    end
    pcall(auditAreas)
end

-- An install or uninstall queues a profile save, and the package functions
-- refuse while one runs; installPackage("") waits one out.
local function waitForProfileSave()
    for _ = 1, 50 do
        local _, err = installPackage("")
        if not (type(err) == "string" and err:find("still in progress", 1, true)) then
            return
        end
        pumpEvents(20)
    end
end

local function removePackage()
    for _ = 1, 50 do
        if not table.contains(getPackages(), packageName) then
            return true
        end
        waitForProfileSave()
        uninstallPackage(packageName)
        pumpEvents(10)
    end
    return false
end

describe("XML import fuzzer", function()
    it("survives importing mutated map and package XML", function()
        if DUMP_PATH then
            dump = io.open(DUMP_PATH, "w")
        end
        print(("XmlImportFuzz: seed=%d iters=%d mode=%s"):format(SEED, ITERS, MODE))
        record(("-- seed=%d mode=%s"):format(SEED, MODE))

        local corpus = {}
        if MODE ~= "package" then
            insert(corpus, { kind = "map", name = "rich", data = RICH_MAP })
            insert(corpus, { kind = "map", name = "minimal", data = readFile(fixtures .. "/maps/minimal-map.xml") })
        end
        if MODE ~= "map" then
            for _, name in ipairs(PACKAGE_FIXTURES) do
                insert(corpus, { kind = "package", name = name, data = readFile(("%s/packages/sources/%s/%s.xml"):format(fixtures, name, name)) })
            end
        end
        if MODE ~= "package" then
            assert.is_true(saveMap(mapBackupPath))
        end
        -- busted keeps only the last finally() a test registers, so this is the one
        finally(function()
            if dump then
                dump:close()
            end
            if MODE ~= "package" then
                loadMap(mapBackupPath)
            end
            removePackage()
        end)

        local imported = 0
        for i = 1, ITERS do
            local base = pick(corpus)
            local data, ops = mutate(base.data)
            record(("%d %s %s %s"):format(i, base.kind, base.name, ops))
            keepInput(data)
            if base.kind == "map" then
                writeFile(mapPath, data)
                local ok = loadMap(mapPath)
                -- paint now, so a report from drawing this map follows its dump record;
                -- a failed XML import leaves what it read so far, so pump either way
                pumpEvents(0)
                if ok then
                    imported = imported + 1
                    exerciseMap()
                    -- and what reading it queued, before the next record
                    pumpEvents(0)
                end
            else
                writeFile(packagePath, data)
                waitForProfileSave()
                if installPackage(packagePath) then
                    imported = imported + 1
                end
                assert.is_true(removePackage(), "the fuzzed package could not be uninstalled")
            end
        end
        print(("XmlImportFuzz: %d of %d mutated files imported"):format(imported, ITERS))
        record("-- done")
    end)
end)
