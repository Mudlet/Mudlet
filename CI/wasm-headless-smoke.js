// Runs a WebAssembly build of mudlet-headless in headless Chromium, as a Qt for WebAssembly
// program needs a DOM, and passes once it prints a given line, runs two more seconds and still
// answers, with no failure before the answer:
//
//   node CI/wasm-headless-smoke.js <dir holding mudlet-headless.js>
//   node CI/wasm-headless-smoke.js <dir> <expected line> [mudlet-headless arguments...]
//   node CI/wasm-headless-smoke.js <dir> --quit-while-loading
//   node CI/wasm-headless-smoke.js <dir> --quit-after-lua-error
//   node CI/wasm-headless-smoke.js <dir> --game-connection
//   node CI/wasm-headless-smoke.js <dir> --persistence
//
// Given only the directory, it runs the probe below and expects "lua says 2". It exits 0 then; 1 if
// before then an error line comes (a package with a part that does not work included), or the
// program exits, aborts, crashes or raises a page error, if the page then does not answer, or if
// WASM_SMOKE_TIMEOUT seconds (default 120) pass without the line; CI/wasm-headless-page.js lists
// what counts as an error line and how an exit shows. With --quit-while-loading, an -e step feeds
// the game a line while standard output has gone, as a pipe's has once its reader exits, which asks
// for the quit before the event loop runs; it exits 0 once the program ends with status 0, and 1 on
// another status, an error line first or the timeout. --quit-after-lua-error does the same after an
// -e step that raises an error, so it wants status 1 and takes that error's line as no failure.
// With --game-connection, the page makes the profile's connection, as docs/headless.md has a page do,
// over a WebSocket to CI/wasm-telnet-relay.js and on to CI/telnet-fixture-server.py; that game asks
// for the terminal type, offers GMCP, sends vitals and a line a trigger answers, and hangs up once
// the answer arrives. The page's first four connections fail as CI/wasm-headless-page.js describes,
// and the profile reconnects after each. It passes as the default does, once the disconnect handler
// finds the next connection played the game, and then only if Mudlet logged all four failures to the
// browser's console and gave each as the reason on its own, logged no other, never showed the page's
// padding, the page was never asked to dial while a connection was open, and the game received
// Mudlet's terminal type, NUL byte and all, its agreement to GMCP and the trigger's command.
// With --persistence, the page keeps the profiles' folder in IndexedDB, as docs/headless.md has a page do, and is
// loaded four times in one tab, which keeps IndexedDB between loads. The first run saves an alias, a map room and a
// setting with the page not syncing; the second finds none of them, saves them again and is reloaded once the page has
// synced on Mudlet's signal; the third finds them, makes another alias and room it never saves and quits, so only the
// shutdown's own save and signal can keep them, and its page's handler for that signal, the first after the line its
// exit handler prints, throws once it has started the sync, which Mudlet must log and carry on from to a clean exit;
// and the fourth finds all of them. It passes as the default does, once the fourth says so.
// It exits 2 if its arguments, the directory or WASM_SMOKE_TIMEOUT are wrong; and 3 if it could not
// run the program at all: playwright does not load, Chromium does not start or quits, the page does
// not load, the game does not start or this script fails. It needs the playwright package on
// NODE_PATH and its Chromium installed, and python3 for --game-connection.

const { spawn } = require("child_process");
const fs = require("fs");
const os = require("os");
const path = require("path");
const { failureMarkers, outputPrefixes, pagePadding, requireBuild, timeoutFrom, page, parseExit, exitedCleanly, openPage, closeBrowser } = require("./wasm-headless-page");
const { startRelay } = require("./wasm-telnet-relay");

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

// Emscripten's clock() wraps 2147 seconds after its first call, so the page moves its clocks on past that when the
// probe prints this stage, and os.clock() must move on with them
const clockStage = "moving the clocks on";
const clockJump = { line: `smoke: ${clockStage}`, seconds: 2200 };

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
    // Waits for the jump through the epoch, as the page sees the stage line once it is printed
    'function smokeClock(done) local before, epoch = os.clock(), getEpoch() ' +
      `smokeStage("${clockStage}") local function check() ` +
      `if getEpoch() - epoch < ${clockJump.seconds} then tempTimer(0.05, check) return end local after = os.clock() ` +
      `if after < before + ${clockJump.seconds} then error(string.format("os.clock() went from %.2f to %.2f as the clocks ` +
      `moved on ${clockJump.seconds} seconds", before, after)) end smokeStage("clock moved on") done() end check() end`,
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
      'if #smokeMaps > 0 then smokeNextMap() else smokeClock(function() echo("lua says " .. smokeSum .. "\\n") end) end end end)',
    'registerAnonymousEventHandler("sysDownloadError", function(_, message, file) ' +
      'if file == smokeFile or file == smokeZip or file == smokeMap or file == smokeGolden then error(message) end end)',
    'local id, err = tempTimer(0, [[feedTriggers("ping 1 λ\\n")]]) assert(id ~= -1, err)',
  ];
  return [
    "-p", "WasmSmoke", "--host", "mudlet.org", "--port", "23", "--offline", "--no-stdin",
    ...probeLua.flatMap((code) => ["-e", code]),
  ];
}

const hex = (text) => Buffer.from(text).toString("hex");
const terminalTypeSent = "fffa1800" + hex("MUDLET") + "fff0";
const gmcpAgreed = "fffdc9";
const gameCommand = "kill dragon\r\n";
const gameGreeting = "fffd18" + "fffa1801fff0" + "fffbc9" + "fffac9" + hex('Char.Vitals {"hp": 42}') + "fff0" + hex("A dragon arrives.\r\n");
const gameOver = "the game hung up";

// Resolves to the fixture's process and port once it accepts connections
async function startGame(directory) {
  const game = spawn("python3", [path.join(__dirname, "telnet-fixture-server.py")], {
    env: { ...process.env, MUDLET_TEST_TELNET_DIR: directory, MUDLET_TEST_TELNET_GREETING_HEX: gameGreeting, MUDLET_TEST_TELNET_HANG_UP_HEX: hex(gameCommand) },
    stdio: ["ignore", "inherit", "inherit"],
  });
  let exited = null;
  game.on("exit", (code, signal) => (exited = signal ?? code));
  const portFile = path.join(directory, "port");
  for (let waited = 0; waited < 10000; waited += 50) {
    if (exited !== null) {
      throw new Error(`the telnet fixture ended with ${exited} before it listened`);
    }
    if (fs.existsSync(portFile)) {
      return { game, port: Number(fs.readFileSync(portFile, "utf8")) };
    }
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
  game.kill();
  throw new Error("the telnet fixture did not listen within 10s");
}

const faultLogged = "mudletTransport: ";
// The faults the page makes, as Mudlet logs them to the browser's console after this prefix and gives them as the
// reason the connection ended on its own console
const pageFaults = [
  "mudletTransport.write() failed: Error: the page could not send",
  "mudletTransport.dial() failed: ",
  "mudletTransport.write() failed: Error: the page's send failed later",
  "connection.received() takes an ArrayBuffer or a view of one such as a Uint8Array, not Blob",
];
const pageFailed = "[transport] FAILED";

function gameArgs(port) {
  const gameLua = [
    'function gameStage(stage) echo("game: " .. stage .. "\\n") end',
    "gameConnections = 0",
    'registerAnonymousEventHandler("sysConnectionEvent", function() gameConnections = gameConnections + 1 gameHp = nil gameTriggered = nil ' +
      'gameStage("connection " .. gameConnections .. " made") end)',
    'registerAnonymousEventHandler("gmcp.Char.Vitals", function() gameHp = gmcp.Char.Vitals.hp gameStage("vitals received") end)',
    'tempRegexTrigger("^A dragon arrives", function() gameTriggered = true gameStage("trigger fired") send("kill dragon") end)',
    'registerAnonymousEventHandler("sysDisconnectionEvent", function() if gameConnections < 4 then gameStage("connection " .. gameConnections .. " ended") reconnect() ' +
      `elseif gameTriggered and gameHp == 42 then echo("${gameOver}\\n") else gameStage("disconnected before the game was played") end end)`,
  ];
  return ["-p", "WasmGame", "--host", "127.0.0.1", "--port", String(port), "--no-tls", "--no-stdin", ...gameLua.flatMap((code) => ["-e", code])];
}

// What the game received that it should have, or the first thing missing
function gameReceived(directory) {
  const received = JSON.parse(fs.readFileSync(path.join(directory, "capture.json"), "utf8")).received;
  if (!received.includes(terminalTypeSent)) {
    return "IAC SB TTYPE IS MUDLET IAC SE";
  }
  if (!received.includes(gmcpAgreed)) {
    return "IAC DO GMCP";
  }
  if (!received.includes(hex(gameCommand))) {
    return JSON.stringify(gameCommand);
  }
  return null;
}

// Failures can come after the expected line: mpkg's package-list request, refused below, can report back after it
const settleMs = 2000;
// A page stuck in a loop prints nothing, so after the settle it must still run a task within this
const answerMs = 5000;

async function pageAnswers(tab) {
  const noAnswer = new Promise((resolve) => setTimeout(() => resolve(false), answerMs));
  const answered = tab.evaluate(() => new Promise((resolve) => setTimeout(() => resolve(true), 0)));
  const answeredInTime = await Promise.race([answered, noAnswer]).catch(() => false);
  if (!answeredInTime) {
    console.log(`[hung] the page ran no task within ${answerMs / 1000}s`);
  }
  return answeredInTime;
}

const [dir, ...rest] = process.argv.slice(2);
const failingStep = rest[0] === "--quit-after-lua-error";
const quitting = failingStep || rest[0] === "--quit-while-loading";
const playing = rest[0] === "--game-connection";
const keeping = rest[0] === "--persistence";
// 1 is exitFailed in src/headless/main.cpp
const quitStatus = failingStep ? 1 : 0;
if (!dir || ((quitting || playing || keeping) && rest.length > 1)) {
  console.error(
    "usage: node CI/wasm-headless-smoke.js <dir holding mudlet-headless.js> " +
      "[<expected line> [mudlet-headless arguments...] | --quit-while-loading | --quit-after-lua-error | --game-connection | --persistence]",
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
// The last -e step feeds the game a line, which goes to standard output, so its failure asks for the quit before exec()
const quitArgs = [
  "-p", "WasmSmokeQuit", "--host", "mudlet.org", "--port", "23", "--offline", "--no-stdin",
  ...(failingStep ? ["-e", 'error("smoke")'] : []),
  "-e", 'feedTriggers("standard output has gone\\n")',
];
// The failing step's error, on the error console, is no failure
const markers = failingStep ? failureMarkers.filter((marker) => marker !== "[ERROR:]") : failureMarkers;
const probing = !rest.length;
let expected = null;
let args = quitArgs;
if (playing) {
  expected = gameOver;
} else if (!quitting) {
  [expected, ...args] = probing ? ["lua says 2", ...defaultProbe()] : rest;
}

const keepArgs = ["-p", "WasmKeep", "--host", "mudlet.org", "--port", "23", "--offline", "--no-stdin", "--config-dir", "/mudlet"];
const keepFirst = 'exists("keepAlias", "alias") > 0, roomExists(4242), getConfig("showTabConnectionIndicators")';
const keepNothing = (why) => `local a, r, s = ${keepFirst} assert(not (a or r or s), "${why}")`;
const keepFound = `local a, r, s = ${keepFirst} assert(a and r and s, "a reload lost what the page synced on Mudlet's signal")`;
const keepChange =
  'permAlias("keepAlias", "", "^keep$", "") addRoom(4242) setRoomArea(4242, addAreaName("keep")) ' +
  'assert(setConfig("showTabConnectionIndicators", true)) assert(saveProfile()) assert(saveMap())';
const keepAll = "keep: all kept";
const keepSaved = "keep: saved";
// Printed once the shutdown has begun, after which the only signal is its own
const keepClosing = "keep: closing";
const keepSaveRun = (sync, why) => ({ sync, lua: [keepNothing(why), keepChange, `echo("${keepSaved}\\n")`], done: keepSaved });
// Each run's steps, whether the page syncs, the line its last step prints, and what the page's handler throws and when
const keepRuns = [
  keepSaveRun(false, "a new browser context already holds the profile"),
  keepSaveRun(true, "a reload kept what the page never synced"),
  {
    sync: true,
    quits: true,
    fault: { after: keepClosing, message: "the page's handler failed, as the smoke test has it do" },
    lua: [
      keepFound,
      'permAlias("keepAlias2", "", "^keep2$", "") addRoom(4243) setRoomArea(4243, getRoomArea(4242))',
      `registerAnonymousEventHandler("sysExitEvent", function() echo("${keepClosing}\\n") end)`,
      'feedTriggers("standard output has gone\\n")',
    ],
  },
  {
    sync: true,
    lua: [
      keepFound,
      'assert(exists("keepAlias2", "alias") > 0 and roomExists(4243), "a reload lost what only the shutdown saved")',
      `echo("${keepAll}\\n")`,
    ],
  },
];

async function keepProfiles() {
  let run = 0;
  let finish;
  const finished = new Promise((resolve) => (finish = resolve));
  // Per run: its done line, Mudlet's signal after it, its exit, whether a sync is due or running, and its handler's fault
  // logged
  let done = false;
  let signalled = false;
  let exited = false;
  let syncDue = false;
  let faultLogged = false;
  let tab = null;
  let reloading = false;
  const timer = setTimeout(() => {
    console.log(`[timeout] after ${timeoutSeconds}s`);
    finish(false);
  }, timeoutSeconds * 1000);
  function runOver() {
    const current = keepRuns[run];
    if (current.quits) {
      return exited && !syncDue && (!current.fault || faultLogged);
    }
    return done && (!current.sync || (signalled && !syncDue));
  }
  function check() {
    if (!tab || reloading || !runOver()) {
      return;
    }
    reloading = true;
    run += 1;
    done = false;
    signalled = false;
    exited = false;
    syncDue = false;
    faultLogged = false;
    console.log(`[reload] run ${run + 1} of ${keepRuns.length}`);
    tab.goto("http://localhost/index.html").then(() => {
      reloading = false;
      check();
    });
  }
  tab = await openPage({
    what: "smoke test",
    dir,
    html: () => {
      const current = keepRuns[run];
      const args = [...keepArgs, ...current.lua.flatMap((code) => ["-e", code])];
      const keep = { dir: "/mudlet", sync: current.sync, fault: current.fault };
      return page({ args, keep, stdoutGone: Boolean(current.quits) });
    },
    onConsole: (text) => {
      console.log(text);
      const status = parseExit(text);
      const line = text.replace(outputPrefixes, "");
      const current = keepRuns[run];
      if (status && current.quits) {
        exited = true;
        if (!exitedCleanly(status)) {
          finish(false);
        }
      } else if (current.fault && !faultLogged && text === `mudletConfigSaved() failed: Error: ${current.fault.message}`) {
        faultLogged = true;
      } else if (failureMarkers.some((marker) => text.includes(marker))) {
        finish(false);
      } else if (text === "[config saved]") {
        syncDue = true;
        signalled = done;
      } else if (text === "[synced]") {
        syncDue = false;
      } else if (line === keepAll) {
        clearTimeout(timer);
        setTimeout(() => finish(true), settleMs);
      } else if (line === current.done) {
        // The signal comes from the event loop, after the step. With no sync, the reload waits as long again as that
        // takes, so it comes after the signal the page ignores
        if (current.sync) {
          done = true;
        } else {
          setTimeout(() => {
            done = true;
            check();
          }, 1000);
        }
      }
      check();
    },
    onFailure: (text) => {
      console.log(text);
      finish(false);
    },
  });
  check();
  const settled = await finished;
  clearTimeout(timer);
  const answeredInTime = settled && (await pageAnswers(tab));
  await closeBrowser();
  if (!answeredInTime) {
    console.error(`mudlet-headless did not keep the profile across reloads (run ${run + 1} of ${keepRuns.length})`);
  }
  process.exit(answeredInTime ? 0 : 1);
}

(async () => {
  if (keeping) {
    return keepProfiles();
  }
  let relay = null;
  let game = null;
  let gameDir = null;
  if (playing) {
    gameDir = fs.mkdtempSync(path.join(os.tmpdir(), "mudlet-wasm-game-"));
    // For a harness failure, which exits at once; a finished run waits for the fixture to end first, as one still
    // writing into the folder can leave it behind
    process.on("exit", () => {
      game?.kill();
      try {
        fs.rmSync(gameDir, { recursive: true, force: true });
      } catch {}
    });
    try {
      let port;
      ({ game, port } = await startGame(gameDir));
      relay = await startRelay({ gamePort: port, log: (text) => console.log("[relay] " + text) });
      args = gameArgs(port);
    } catch (error) {
      console.error(`the smoke test itself failed: ${error?.stack ?? error}`);
      process.exit(3);
    }
  }
  let finish;
  const finished = new Promise((resolve) => (finish = resolve));
  let seen = false;
  const faultsLogged = new Set();
  const faultsShown = new Set();
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
    html: page({ args, clockJump: probing ? clockJump : null, stdoutGone: quitting, relay: relay?.url }),
    serve: (file) => fixtureFile(path.basename(file)),
    reachLoopback: playing,
    onConsole: (text) => {
      console.log(text);
      const status = quitting ? parseExit(text) : null;
      const shown = playing && outputPrefixes.test(text) ? pageFaults.filter((fault) => text.includes(fault)) : [];
      if (status) {
        finish(status.code === quitStatus && !status.crashed);
      } else if (!quitting && text.replace(outputPrefixes, "") === expected) {
        seen = true;
        clearTimeout(timer);
        setTimeout(() => finish(true), settleMs);
      } else if (shown.length) {
        // Even on the "[ ERROR ]" line that gives a failed dial's reason
        shown.forEach((fault) => faultsShown.add(fault));
      } else if (markers.some((marker) => text.includes(marker)) || (playing && (text.startsWith(pageFailed) || text.includes(pagePadding)))) {
        fail();
      } else if (playing && text.startsWith(faultLogged)) {
        // Any other is a fault of Mudlet's, such as received() refusing what the page gave it
        const fault = pageFaults.find((known) => text.startsWith(faultLogged + known));
        if (fault) {
          faultsLogged.add(fault);
        } else {
          fail();
        }
      }
    },
    onFailure: (text) => {
      console.log(text);
      fail();
    },
  });
  const settled = await finished;
  clearTimeout(timer);
  const answeredInTime = settled && !quitting && (await pageAnswers(tab));
  const gameChecked = playing && answeredInTime && !failed;
  const unreported = [];
  if (gameChecked) {
    for (const fault of pageFaults) {
      if (!faultsLogged.has(fault)) {
        unreported.push(`Mudlet never logged "${faultLogged}${fault}"`);
      }
      if (!faultsShown.has(fault)) {
        unreported.push(`Mudlet never gave "${fault}" as the reason the connection ended`);
      }
    }
  }
  unreported.forEach((what) => console.log(`[transport] ${what}`));
  const missing = gameChecked && !unreported.length ? gameReceived(gameDir) : null;
  if (missing) {
    console.log(`[game] never received ${missing}`);
  }
  const passed = (quitting ? settled : answeredInTime) && !failed && !unreported.length && !missing;
  await closeBrowser();
  relay?.close();
  if (game && game.exitCode === null && game.signalCode === null) {
    const ended = new Promise((resolve) => game.once("exit", resolve));
    game.kill();
    await ended;
  }
  if (gameDir) {
    fs.rmSync(gameDir, { recursive: true, force: true });
  }
  if (!passed) {
    if (quitting) {
      console.error(`mudlet-headless did not end with status ${quitStatus} once standard output had gone`);
    } else {
      const what = seen ? "failed after printing" : "did not print";
      console.error(`mudlet-headless ${what} "${expected}"`);
    }
  }
  process.exit(passed ? 0 : 1);
})();
