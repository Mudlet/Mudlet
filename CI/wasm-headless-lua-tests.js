// Runs Mudlet's Lua specs (src/mudlet-lua/tests) in a WebAssembly build of mudlet-headless in headless Chromium, as
// .github/workflows/headless-lua-tests.yml runs them in a desktop one, and writes what the run printed to a log for
// CI/check-headless-lua-failures.py:
//
//   CI/fetch-wasm-busted.sh <busted dir>
//   node CI/wasm-headless-lua-tests.js <dir holding mudlet-headless.js> <busted dir> <log file>
//
// The specs, the gui-drop package one of them installs, busted and CI/wasm-busted-shims are copied into the page's
// file system at the paths below. It exits 0 once mudlet-headless ends with status 0 and its log holds busted's
// summary, whichever specs failed; 1 if a failure marker of CI/wasm-headless-page.js comes before the specs start,
// the program ends with another status, aborts, crashes or raises a page error, or WASM_LUA_TESTS_TIMEOUT seconds
// (default 3600) pass first; 2 if its arguments or the timeout are wrong; and 3 if it could not run the program.
// With WASM_LUA_TESTS_TRACE=1 it prints each test's name as it starts, which names the one a hang is in; with
// WASM_LUA_TESTS_ONLY=<spec file> it runs only that file.

const fs = require("fs");
const path = require("path");
const { failureMarkers, outputPrefixes, missingFiles, page, openPage, closeBrowser } = require("./wasm-headless-page");

const repo = path.join(__dirname, "..");
const testsPath = "/mudlet/src/mudlet-lua/tests";
const bustedPath = "/busted";
// As in a desktop run, a summary line busted printed, which the log of a run that never got to it lacks
const summary = /^\d+ success(?:es)? \/ \d+ failures? \/ \d+ errors? \/ \d+ pending : [0-9.]+ seconds\s*$/;
// run-tests prints this once the profile has loaded; the specs print errors on purpose from here on
const specsStart = "Setup to auto-run tests - commencing.";

const [dir, bustedDir, logFile, ...extra] = process.argv.slice(2);
if (!logFile || extra.length) {
  console.error("usage: node CI/wasm-headless-lua-tests.js <dir holding mudlet-headless.js> <busted dir> <log file>");
  process.exit(2);
}
const missing = missingFiles(dir);
if (missing.length) {
  console.error(`${dir} has no ${missing.join(", ")}`);
  process.exit(2);
}
if (!fs.existsSync(path.join(bustedDir, "busted", "core.lua"))) {
  console.error(`${bustedDir} holds no busted; fill it with CI/fetch-wasm-busted.sh`);
  process.exit(2);
}
const timeoutSeconds = Number(process.env.WASM_LUA_TESTS_TIMEOUT || 3600);
// setTimeout() takes at most 2^31 - 1 ms and fires after 1 ms for anything longer
const maxTimeoutSeconds = 2147483;
if (!(timeoutSeconds > 0 && timeoutSeconds <= maxTimeoutSeconds)) {
  console.error(
    `WASM_LUA_TESTS_TIMEOUT must be a number of seconds above 0 and at most ${maxTimeoutSeconds}, ` +
      `not "${process.env.WASM_LUA_TESTS_TIMEOUT}"`,
  );
  process.exit(2);
}

// The page's path of each file it is given, to the file here
const staged = new Map();
function stage(from, to) {
  if (fs.statSync(from).isDirectory()) {
    for (const name of fs.readdirSync(from)) {
      stage(path.join(from, name), `${to}/${name}`);
    }
  } else {
    staged.set(to, from);
  }
}
stage(path.join(repo, "src", "mudlet-lua", "tests"), testsPath);
// Package_spec.lua finds it beside the specs, as in a source tree
stage(path.join(repo, "src", "packages", "gui-drop", "gui-drop.mpackage"), "/mudlet/src/packages/gui-drop/gui-drop.mpackage");
for (const name of ["busted", "luassert", "say", "pl", "mediator.lua"]) {
  stage(path.join(bustedDir, name), `${bustedPath}/${name}`);
}
stage(path.join(__dirname, "wasm-busted-shims"), bustedPath);

const args = ["-p", "Mudlet self-test", "--host", "mudlet.org", "--port", "23", "--offline", "--no-stdin"];
if (process.env.WASM_LUA_TESTS_TRACE === "1") {
  args.push("-e", 'busted.subscribe({"test", "start"}, function(element) ' +
    'local names, parent = {element.name}, busted.context.parent(element) ' +
    'while parent and parent.descriptor ~= "file" do table.insert(names, 1, tostring(parent.name)) parent = busted.context.parent(parent) end ' +
    'ioprint("[test] " .. tostring(parent and parent.name) .. ": " .. table.concat(names, " ")) return nil, true end)');
}
const environment = {
  AUTORUN_BUSTED_TESTS: "true",
  MUDLET_MATCH_THREADS: "2",
  MUDLET_MATCH_SAMPLE_EVERY: "0",
  MUDLET_TEST_MODE: "1",
  MUDLET_TEST_FAILURE_MARKER: "/tmp/busted-tests-failed",
  TESTS_DIRECTORY: process.env.WASM_LUA_TESTS_ONLY ? `${testsPath}/${process.env.WASM_LUA_TESTS_ONLY}` : testsPath,
  QUIT_MUDLET_AFTER_TESTS: "true",
  LUA_PATH: `${bustedPath}/?.lua;${bustedPath}/?/init.lua;;`,
};
// "[ ERROR ]" is translated, and Mudlet follows the browser's language once its translation is complete enough
const settings = "[General]\ninterfaceLanguage=en_US\n";

(async () => {
  const log = fs.createWriteStream(logFile);
  let finish;
  const finished = new Promise((resolve) => (finish = resolve));
  let started = false;
  let summarised = false;
  let exitStatus = null;
  const timer = setTimeout(() => {
    console.log(`[timeout] after ${timeoutSeconds}s`);
    finish(false);
  }, timeoutSeconds * 1000);
  await openPage({
    what: "Lua test runner",
    dir,
    html: page({ args, settings, environment, staged: true }),
    serve: (file) => {
      if (file === "/stage.json") {
        return { contentType: "application/json", body: JSON.stringify([...staged.keys()]) };
      }
      return file.startsWith("/stage/") ? staged.get(file.slice("/stage".length)) ?? "" : null;
    },
    onConsole: (text) => {
      console.log(text);
      const line = text.replace(outputPrefixes, "");
      log.write(line + "\n");
      const exited = /^\[exit\] (.*)$/.exec(text);
      if (exited) {
        exitStatus = JSON.parse(exited[1]);
        finish(true);
      } else if (line === specsStart) {
        started = true;
      } else if (summary.test(line)) {
        summarised = true;
      } else if (!started && failureMarkers.some((marker) => text.includes(marker))) {
        console.log("[failed] before the specs started");
        finish(false);
      }
    },
    onFailure: (text) => {
      console.log(text);
      log.write(text + "\n");
      finish(false);
    },
  });
  let passed = await finished;
  clearTimeout(timer);
  await closeBrowser();
  await new Promise((resolve) => log.end(resolve));
  if (passed && (exitStatus.crashed || exitStatus.code !== 0)) {
    console.error(`mudlet-headless exited with ${JSON.stringify(exitStatus)}`);
    passed = false;
  } else if (passed && !summarised) {
    console.error("mudlet-headless exited without busted's summary");
    passed = false;
  }
  process.exit(passed ? 0 : 1);
})();
