// The page and headless Chromium that CI/wasm-headless-smoke.js and CI/wasm-headless-lua-tests.js run a
// WebAssembly build of mudlet-headless in, as a Qt for WebAssembly program needs a DOM. It needs the playwright
// package on NODE_PATH and its Chromium installed.

const fs = require("fs");
const path = require("path");

// While a profile loads, a package's broken scripts and patterns are only logged, not put on the console.
// "[exit]" covers an abort, a failed start and the program's end; an exit() the runtime outlives, as Qt keeps it alive,
// only shows in a build linked with -sASSERTIONS, as CI's is, which says "program exited (".
const failureMarkers = [
  "[ ERROR ]",
  "[ERROR:]",
  "[  LUA  ] - ",
  "[ WARN ]  - Failed to load ",
  "Host::installPackage() WARNING - parts of ",
  "[exit]",
  "[abort]",
  "program exited (",
];

// Where the page puts what mudlet-headless printed, ahead of the text
const outputPrefixes = /^\[(out|err)\] /;

// "[ ERROR ]" is translated, and Mudlet follows the browser's language once its translation is complete enough
const settings = "[General]\ninterfaceLanguage=en_US\n";

// Exits 2 unless dir holds a build
function requireBuild(dir) {
  const missing = ["mudlet-headless.js", "mudlet-headless.wasm", "qtloader.js"].filter((file) => !fs.existsSync(path.join(dir, file)));
  if (missing.length) {
    console.error(`${dir} has no ${missing.join(", ")}`);
    process.exit(2);
  }
}

// The seconds the environment variable gives, or fallback when it is unset; exits 2 when it is not a timeout
function timeoutFrom(variable, fallback) {
  const seconds = Number(process.env[variable] || fallback);
  // setTimeout() takes at most 2^31 - 1 ms and fires after 1 ms for anything longer
  const maxSeconds = 2147483;
  if (!(seconds > 0 && seconds <= maxSeconds)) {
    console.error(`${variable} must be a number of seconds above 0 and at most ${maxSeconds}, not "${process.env[variable]}"`);
    process.exit(2);
  }
  return seconds;
}

// Script text for the page: JSON, escaped, as an argument holding "</script>" would end the page's script element
function scriptValue(value) {
  return JSON.stringify(value).replace(/</g, "\\u003c");
}

// The page that starts mudlet-headless with args, a Mudlet.ini of settings and the environment given. With staged,
// it first fetches stage.json, a list of absolute paths, and each of them from stage/<path>, and writes them into
// the program's file system before it starts. It prints "[exit] " and the exit status when the program ends.
function page({ args, settings, environment = {}, staged = false }) {
  return `<!doctype html>
<html><head><meta charset="utf-8"><title>mudlet-headless</title></head>
<body><div id="screen" style="width:800px;height:600px"></div>
<script src="qtloader.js"></script>
<script src="mudlet-headless.js"></script>
<script>
// Playwright reports a promise rejected with a DOMException as a page error with no text. Qt's media player never
// handles the promise an audio element's play() returns, which a stop soon after the start rejects; that is no failure.
window.addEventListener("unhandledrejection", (e) => {
  console.log("[rejection] " + e.reason);
  if (e.reason instanceof DOMException && e.reason.name === "AbortError") {
    e.preventDefault();
  }
});
(async () => {
  const staged = ${staged} ? await Promise.all((await (await fetch("stage.json")).json()).map(async (file) => {
    const response = await fetch("stage" + file.split("/").map(encodeURIComponent).join("/"));
    if (!response.ok) {
      throw new Error("could not stage " + file + ": " + response.status);
    }
    return [file, new Uint8Array(await response.arrayBuffer())];
  })) : [];
  await qtLoad({
    arguments: ${scriptValue(args)},
    print: (t) => console.log("[out] " + t),
    printErr: (t) => console.log("[err] " + t),
    preRun: [(m) => {
      m.FS.mkdirTree("/home/web_user/.config/mudlet");
      m.FS.writeFile("/home/web_user/.config/mudlet/Mudlet.ini", ${scriptValue(settings)});
      for (const [file, data] of staged) {
        m.FS.mkdirTree(file.slice(0, file.lastIndexOf("/")) || "/");
        m.FS.writeFile(file, data);
      }
    }],
    qt: {
      entryFunction: window.mudlet_headless_executable_entry,
      containerElements: [document.getElementById("screen")],
      environment: ${scriptValue(environment)},
      onExit: (e) => console.log("[exit] " + JSON.stringify(e)),
    },
  });
})().catch((e) => console.log("[abort] " + e));
</script>
</body></html>
`;
}

let browser;
// Set before the browser is closed on purpose, after which its going away is no failure
let closing = false;
// Bounded, as a browser that broke may not close either
async function closeBrowser() {
  closing = true;
  await Promise.race([browser?.close(), new Promise((resolve) => setTimeout(resolve, 5000))]).catch(() => {});
}
// For a failure of the harness or what it runs on, as opposed to one of mudlet-headless
async function harnessFailed(what, error) {
  console.error(`the ${what} itself failed: ${error?.stack ?? error}`);
  await closeBrowser();
  process.exit(3);
}

// Opens html at http://localhost/index.html, answering the page's requests with serve(path): a file's path, a
// response for Playwright's route.fulfill() or null, for the file of that name in dir. onConsole gets each console line; onFailure the crash of the tab or an
// error the page raised. A harness failure exits 3, naming what.
async function openPage({ what, dir, html, serve = () => null, onConsole, onFailure }) {
  process.on("uncaughtException", (error) => harnessFailed(what, error));
  process.on("unhandledRejection", (error) => harnessFailed(what, error));
  const { chromium } = require("playwright");
  // As a page opened by a person who clicked in it: the sound specs play sounds
  browser = await chromium.launch({ args: ["--autoplay-policy=no-user-gesture-required"] });
  // A tab that crashes is mudlet-headless failing; the browser process itself going is not
  function gone(reason) {
    if (!closing) {
      harnessFailed(what, reason);
    }
  }
  browser.on("disconnected", () => gone("Chromium quit"));
  const tab = await browser.newPage();
  tab.on("close", () => gone("the page was closed"));
  tab.on("console", (message) => onConsole(message.text()));
  tab.on("pageerror", (error) => onFailure("[pageerror] " + (error.stack ?? error.message)));
  tab.on("crash", () => onFailure("[crash]"));
  // localhost, so the page is a secure context. Nothing listens there, as its requests are answered here;
  // any other origin's are refused, so the run never depends on the network (mpkg fetches from GitHub)
  const origin = "http://localhost";
  await tab.route("**/*", (route) => {
    const url = new URL(route.request().url());
    if (url.origin !== origin) {
      return route.abort();
    }
    if (url.pathname === "/index.html") {
      return route.fulfill({ contentType: "text/html", body: html });
    }
    const served = serve(decodeURIComponent(url.pathname));
    if (typeof served === "object" && served !== null) {
      return route.fulfill(served);
    }
    const file = served ?? path.join(dir, path.basename(url.pathname));
    if (!fs.existsSync(file)) {
      return route.fulfill({ status: 404 });
    }
    // Typed by extension, so the module is application/wasm, which the browser compiles as it downloads
    return route.fulfill({ path: file });
  });
  await tab.goto(`${origin}/index.html`);
  return tab;
}

module.exports = { failureMarkers, outputPrefixes, settings, requireBuild, timeoutFrom, page, openPage, closeBrowser };
