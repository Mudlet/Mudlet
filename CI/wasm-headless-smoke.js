// Runs a WebAssembly build of mudlet-headless in headless Chromium, as a Qt for WebAssembly
// program needs a DOM, and passes once it prints a given line, runs two more seconds and still
// answers, with no failure before the answer:
//
//   node CI/wasm-headless-smoke.js <dir holding mudlet-headless.js>
//   node CI/wasm-headless-smoke.js <dir> <expected line> [mudlet-headless arguments...]
//
// Given only the directory, it runs the probe below and expects "lua says 2". It exits 0 then; 1 if
// before then an error line comes (a package with a part that does not work included), or the
// program exits, aborts, crashes or raises a page error, if the page then does not answer, or if
// WASM_SMOKE_TIMEOUT seconds (default 120) pass without the line; CI/wasm-headless-page.js lists
// what counts as an error line and how an exit shows. It exits 2 if its arguments, the directory
// or WASM_SMOKE_TIMEOUT are wrong; and 3 if it could not run the program at all: playwright does
// not load, Chromium does not start or quits, the page does not load or this script fails. It needs
// the playwright package on NODE_PATH and its Chromium installed.

const fs = require("fs");
const path = require("path");
const { failureMarkers, outputPrefixes, settings, requireBuild, timeoutFrom, page, openPage, closeBrowser } = require("./wasm-headless-page");

const fixtureDir = path.join(__dirname, "..", "src", "mudlet-lua", "tests", "fixtures");
const mapFixtureDir = path.join(fixtureDir, "maps");
// The fixtures the page serves, by name; only the default probe downloads any
function fixtureFile(name) {
  if (/^cross-platform-map(-v\d+)?\.(dat|json)$/.test(name)) {
    return path.join(mapFixtureDir, name);
  }
  // Its one entry is over the 1 MiB unzip() reads at a time
  if (name === "unzip-large.zip") {
    return path.join(fixtureDir, "archives", name);
  }
  return null;
}

// The trigger comes from the last file LuaGlobal.lua loads, and each step reaches a library the browser build links
// itself; the maps desktop Mudlet saved (src/mudlet-lua/tests/fixtures/maps/README.md) check the one 32-bit build CI
// runs. Each save removes its file, as two saves in the same second write the same file.
function defaultProbe() {
  const newerMapVersions = fs.readdirSync(mapFixtureDir)
    .map((file) => /^cross-platform-map-v(\d+)\.dat$/.exec(file)?.[1])
    .filter(Boolean)
    .map(Number)
    .sort((a, b) => a - b);
  // Shared with build-cross-platform-map.lua and Mapper_spec.lua, so fixtures remade with a change to it hold here too
  const normaliser = fs.readFileSync(path.join(mapFixtureDir, "normalise-map-json.lua"), "utf8");
  const probeLua = [
    'smokeHome = getMudletHomeDir() .. "/"',
    'smokeFile = smokeHome .. "smoke-download.js"',
    'smokeZip = smokeHome .. "unzip-large.zip"',
    'smokeMapJson = smokeHome .. "smoke-map.json"',
    'smokeMapSave = smokeHome .. "smoke-map.dat"',
    // The format each map is at, false for the default
    `smokeMaps = {${[false, ...newerMapVersions].join(", ")}}`,
    // Names how far a run that times out got
    'function smokeStage(stage) echo("smoke: " .. stage .. "\\n") end',
    'function smokeRead(file) local f = assert(io.open(file, "rb")) local text = f:read("*a") f:close() return text end',
    'function smokeSave() local ok, file = saveProfile() assert(ok, file) ' +
      'assert(smokeRead(file):find("<MudletPackage", 1, true), file .. " holds no profile") assert(os.remove(file)) end',
    `smokeNormalised = (function() ${normaliser}\nend)()`,
    'function smokeJson(map) local ok, err = loadMap(map) assert(ok, "loadMap(" .. map .. ") failed: " .. tostring(err)) ' +
      'assert(saveJsonMap(smokeMapJson)) local json = smokeRead(smokeMapJson) assert(os.remove(smokeMapJson)) ' +
      'return smokeNormalised(json) end',
    'function smokeSameJson(what, json, golden) ' +
      'local remake = "; if intended, remake the files as src/mudlet-lua/tests/fixtures/maps/README.md says" ' +
      'local nextGolden, n = (smokeRead(golden) .. "\\n"):gmatch("([^\\n]*)\\n"), 0 ' +
      'for line in (json .. "\\n"):gmatch("([^\\n]*)\\n") do n = n + 1 local want = nextGolden() if line ~= want then ' +
      'error(string.format("the JSON export of %s differs from the one desktop Mudlet made at line %d: %q where it has %q%s", ' +
      'what, n, line, tostring(want), remake)) end end ' +
      'if nextGolden() ~= nil then ' +
      'error(string.format("the JSON export of %s ends at line %d, before the one desktop Mudlet made%s", what, n, remake)) end end',
    'function smokeNextMap() smokeMapVersion = table.remove(smokeMaps, 1) ' +
      'smokeMapName = "cross-platform-map" .. (smokeMapVersion and "-v" .. smokeMapVersion or "") ' +
      'smokeMap = smokeHome .. smokeMapName .. ".dat" ' +
      'smokeGolden = smokeHome .. smokeMapName .. ".json" ' +
      'assert(downloadFile(smokeMap, "http://localhost/" .. smokeMapName .. ".dat")) end',
    'function smokeRoute(from, to, want) local ok, weight = getPath(from, to) ' +
      'local got = ok and table.concat(speedWalkDir, ",") .. " weighing " .. weight or "no route" ' +
      'if got ~= want then error(string.format("getPath(%d, %d) gave %s, not %s", from, to, got, want)) end end',
    // The routes Mapper_spec.lua checks on desktop: 3's down exit, room 5 and 5's special exit up are locked; with 3's
    // down exit and room 5 unlocked, the route crosses the weighted exit, the weighted room and 4's special exit down,
    // until that one is locked past the start room, where findPath() leaves the lock to the graph; and of two routes
    // to 3 it takes the cheaper, not the one with fewer steps
    'function smokeRoutes() smokeRoute(4, 1, "up,s,w weighing 3") smokeRoute(1, 4, "no route") ' +
      'smokeRoute(4, 5, "no route") assert(lockExit(3, "d", false)) assert(lockRoom(5, false)) ' +
      'smokeRoute(1, 5, "e,n,down,climb down the well weighing 12") ' +
      'assert(lockSpecialExit(4, 5, "climb down the well", true)) smokeRoute(1, 5, "no route") ' +
      'assert(lockSpecialExit(4, 5, "climb down the well", false)) smokeRoute(5, 4, "no route") ' +
      'assert(setExit(1, 3, "ne")) assert(setExitWeight(1, "ne", 5)) smokeRoute(1, 3, "e,n weighing 4") ' +
      'assert(setExitWeight(1, "ne", 3)) smokeRoute(1, 3, "ne weighing 3") ' +
      'smokeStage("path found") end',
    'function smokeMapCheck() smokeSameJson("the map desktop Mudlet saved as " .. smokeMap, smokeJson(smokeMap), smokeGolden) ' +
      'smokeStage("map compared") ' +
      'assert(saveMap(smokeMapSave, smokeMapVersion or 0)) ' +
      'smokeSameJson("this build\'s save of " .. smokeMap, smokeJson(smokeMapSave), smokeGolden) assert(os.remove(smokeMapSave)) ' +
      'smokeStage("reload compared") smokeRoutes() end',
    // lua-zip, through mudlet-lua's unzip()
    'function smokeUnzip() local folder = smokeHome .. "smoke-unzip" lfs.mkdir(folder) ' +
      'local ok, err = unzip(smokeZip, folder) assert(ok, "unzip(" .. smokeZip .. ") failed: " .. tostring(err)) ' +
      'local size = lfs.attributes(folder .. "/large.bin", "size") ' +
      'assert(size == 2097155, "large.bin came out at " .. tostring(size) .. " bytes, not 2097155") ' +
      // As LuaGlobal_spec.lua builds it: a pattern that never lines up with a chunk shows one lost or repeated
      'local unit = {} for i = 0, 250 do unit[#unit + 1] = string.char(i * 7 % 251) end ' +
      'local want, got = table.concat(unit):rep(math.floor(size / 251) + 1):sub(1, size), smokeRead(folder .. "/large.bin") ' +
      'if got ~= want then local at = 1 while got:sub(at, at + 4095) == want:sub(at, at + 4095) do at = at + 4096 end ' +
      'error(string.format("large.bin differs from the archived entry within bytes %d-%d", at, at + 4095)) end ' +
      'smokeStage("archive unzipped") end',
    // Hunspell through the profile's own dictionary, as the browser has no system one
    'function smokeSpellCheck() local word = "smokewibble" ' +
      'local known, err = spellCheckWord(word, true) assert(known ~= nil, err) ' +
      'assert(known == false, word .. " is in a new profile\'s dictionary") ' +
      'assert(addWordToDictionary(word)) assert(spellCheckWord(word, true), word .. " was added but is not known") ' +
      'local suggested = table.concat(assert(spellSuggestWord("smokewibbel", true)), ",") ' +
      'assert(suggested == word, "Hunspell suggested \\"" .. suggested .. "\\" for smokewibbel") ' +
      'smokeStage("spell checked") end',
    // getVerifiedInt()'s range check, which on a 32-bit build cannot lean on lua_Integer being wider than an int;
    // getRoomEnv() hands back the int setRoomEnv() stored
    'function smokeIntRange() for _, n in ipairs({2 ^ 31, -2 ^ 31 - 1}) do local ok, err = pcall(roomExists, n) ' +
      'assert(not ok and tostring(err):find("over/under-flow", 1, true), ' +
      '"roomExists(" .. n .. ") was not refused: " .. tostring(err)) end ' +
      'local room = createRoomID() assert(addRoom(room)) for _, n in ipairs({2147483647, 2147483647.5}) do ' +
      'assert(setRoomEnv(room, n)) local env = getRoomEnv(room) ' +
      'assert(env == 2147483647, "setRoomEnv(" .. n .. ") stored " .. tostring(env)) end ' +
      'assert(deleteRoom(room)) smokeStage("int range checked") end',
    'registerNamedRegexTrigger("smoke", "ping", [[^ping (\\d) \\p{Greek}+$]], function() smokeStage("trigger fired") ' +
      'local numbers = db:create("smoke", {numbers = {n = 0}}).numbers ' +
      'assert(db:add(numbers, {n = tonumber(matches[2])}, {n = yajl.to_value("[1]")[1]})) ' +
      'smokeSum = db:aggregate(numbers.n, "sum") ' +
      'assert(lpeg.match(lpeg.C(lpeg.P("a") ^ 1), "aaab") == "aaa", "lpeg did not match") smokeStage("lpeg matched") ' +
      'smokeIntRange() smokeSpellCheck() ' +
      'smokeSave() smokeStage("save 1 checked") assert(downloadFile(smokeFile, "http://localhost/qtloader.js")) end)',
    'registerAnonymousEventHandler("sysDownloadDone", function(_, file, size) if file == smokeFile then ' +
      'assert(size > 0 and #smokeRead(file) == size, file .. " did not arrive whole") smokeStage("download 1 whole") ' +
      'smokeSave() smokeStage("save 2 checked") assert(downloadFile(smokeZip, "http://localhost/unzip-large.zip")) ' +
      'elseif file == smokeZip then smokeUnzip() smokeNextMap() ' +
      'elseif file == smokeMap then smokeStage("map downloaded") ' +
      'assert(downloadFile(smokeGolden, "http://localhost/" .. smokeMapName .. ".json")) ' +
      'elseif file == smokeGolden then smokeStage("golden downloaded") smokeMapCheck() ' +
      'if #smokeMaps > 0 then smokeNextMap() else echo("lua says " .. smokeSum .. "\\n") end end end)',
    'registerAnonymousEventHandler("sysDownloadError", function(_, message, file) ' +
      'if file == smokeFile or file == smokeZip or file == smokeMap or file == smokeGolden then error(message) end end)',
    'local id, err = tempTimer(0, [[feedTriggers("ping 1 λ\\n")]]) assert(id ~= -1, err)',
  ];
  return [
    "-p", "WasmSmoke", "--host", "mudlet.org", "--port", "23", "--offline", "--no-stdin",
    ...probeLua.flatMap((code) => ["-e", code]),
  ];
}

// Failures can come after the expected line: mpkg's package-list request, refused below, can report back after it
const settleMs = 2000;
// A page stuck in a loop prints nothing, so after the settle it must still run a task within this
const answerMs = 5000;

const [dir, ...rest] = process.argv.slice(2);
if (!dir) {
  console.error(
    "usage: node CI/wasm-headless-smoke.js <dir holding mudlet-headless.js> " +
      "[<expected line> [mudlet-headless arguments...]]",
  );
  process.exit(2);
}
// A blank line would match any empty line the program prints
if (rest.length && !rest[0].trim()) {
  console.error("the expected line must not be blank");
  process.exit(2);
}
requireBuild(dir);
const timeoutSeconds = timeoutFrom("WASM_SMOKE_TIMEOUT", 120);
const [expected, ...args] = rest.length ? rest : ["lua says 2", ...defaultProbe()];

(async () => {
  let finish;
  const finished = new Promise((resolve) => (finish = resolve));
  let seen = false;
  // A failure after the settle comes once the wait is over, and must still fail the run
  let failed = false;
  function fail() {
    failed = true;
    finish(false);
  }
  const timer = setTimeout(() => {
    console.log(`[timeout] after ${timeoutSeconds}s`);
    finish(false);
  }, timeoutSeconds * 1000);
  const tab = await openPage({
    what: "smoke test",
    dir,
    html: page({ args, settings }),
    serve: (file) => fixtureFile(path.basename(file)),
    onConsole: (text) => {
      console.log(text);
      if (text.replace(outputPrefixes, "") === expected) {
        seen = true;
        clearTimeout(timer);
        setTimeout(() => finish(true), settleMs);
      } else if (failureMarkers.some((marker) => text.includes(marker))) {
        fail();
      }
    },
    onFailure: (text) => {
      console.log(text);
      fail();
    },
  });
  const settled = await finished;
  clearTimeout(timer);
  let answeredInTime = false;
  if (settled) {
    const noAnswer = new Promise((resolve) => setTimeout(() => resolve(false), answerMs));
    const answered = tab.evaluate(() => new Promise((resolve) => setTimeout(() => resolve(true), 0)));
    answeredInTime = await Promise.race([answered, noAnswer]).catch(() => false);
    if (!answeredInTime) {
      console.log(`[hung] the page ran no task within ${answerMs / 1000}s`);
    }
  }
  const passed = answeredInTime && !failed;
  await closeBrowser();
  if (!passed) {
    const what = seen ? "failed after printing" : "did not print";
    console.error(`mudlet-headless ${what} "${expected}"`);
  }
  process.exit(passed ? 0 : 1);
})();
