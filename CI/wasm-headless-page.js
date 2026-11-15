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
  "[sync failed]",
];

// Where the page puts what mudlet-headless printed, ahead of the text
const outputPrefixes = /^\[(out|err)\] /;

// What the page's game connection puts either side of the bytes it hands received(), outside the view of them
const pagePadding = "the page's padding";

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

// The page that starts mudlet-headless with args, the Mudlet.ini above and the environment given. With staged,
// it first fetches stage.json, a list of absolute paths, and each of them from stage/<path>, and writes them into
// the program's file system before it starts. It prints "[exit] " and the exit status when the program ends. With
// clockJump, { line, seconds }, Date.now() and performance.now() jump that many seconds ahead once the program prints
// that line; with stdoutGone, writing to standard output fails as it does once a pipe's reader has gone. With relay,
// a CI/wasm-telnet-relay.js URL, the page makes the profile's game connection through it, as docs/headless.md has it.
// With keep, { dir, sync }, the page keeps the folder dir in IndexedDB as docs/headless.md has it, loading it before the
// program starts and, if sync, saving it each time the program says it wrote there, which it prints as "[config saved]";
// it prints "[synced]" once a save has ended with none due after it.
function page({ args, environment = {}, staged = false, clockJump = null, stdoutGone = false, relay = null, keep = null }) {
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
const clockJump = ${scriptValue(clockJump)};
let clockOffset = 0;
if (clockJump) {
  const dateNow = Date.now;
  const performanceNow = performance.now.bind(performance);
  Date.now = () => dateNow() + clockOffset;
  performance.now = () => performanceNow() + clockOffset;
}
function output(prefix) {
  return (t) => {
    console.log(prefix + t);
    if (clockJump && t === clockJump.line) {
      clockOffset += clockJump.seconds * 1000;
    }
  };
}
const relay = ${scriptValue(relay)};
const padding = new TextEncoder().encode(${scriptValue(`\r\n${pagePadding}\r\n`)});
// The first connection's write() throws, and it hands received() a DataView into the middle of a larger buffer, whose
// padding either side shows as a line of its own if Mudlet reads past the view. The second hands it the Blob a
// WebSocket gives on its default binaryType, and the rest the ArrayBuffer it gives otherwise. dial() is async, and the
// third's rejects; the fourth's write() returns a promise that rejects. Mudlet must close the first four itself, as the
// profile, told they ended, asks for no close(); the page fails the run if it is asked to dial while the last connection
// is still open.
function relayTransport() {
  let socket = null;
  let dials = 0;
  return {
    async dial(host, port, tlsRequested, connection) {
      const attempt = ++dials;
      console.log("[transport] dial " + attempt + " to " + host + ":" + port + (tlsRequested ? " with TLS asked for" : ""));
      if (socket && socket.readyState <= WebSocket.OPEN) {
        console.log("[transport] FAILED: asked to dial while the last connection was still open");
      }
      if (attempt === 3) {
        throw new Error("the page could not reach the game (a fault the smoke test makes)");
      }
      socket = new WebSocket(relay + "?host=" + encodeURIComponent(host) + "&port=" + port);
      if (attempt !== 2) {
        socket.binaryType = "arraybuffer";
      }
      socket.onopen = () => connection.connected();
      socket.onmessage = (e) => {
        if (attempt === 1) {
          const data = new Uint8Array(e.data);
          const padded = new Uint8Array(padding.length + data.length + padding.length);
          padded.set(padding);
          padded.set(data, padding.length);
          padded.set(padding, padding.length + data.length);
          connection.received(new DataView(padded.buffer, padding.length, data.length));
        } else {
          connection.received(e.data);
        }
      };
      socket.onclose = (e) => connection.closed(e.reason || "the relay closed the connection with code " + e.code);
    },
    write(bytes) {
      if (dials === 1) {
        throw new Error("the page could not send (a fault the smoke test makes)");
      }
      if (dials === 4) {
        return Promise.reject(new Error("the page's send failed later (a fault the smoke test makes)"));
      }
      socket.send(bytes);
    },
    close() {
      console.log("[transport] close");
      socket.onclose = socket.onmessage = null;
      socket.close();
    },
  };
}
const keep = ${scriptValue(keep)};
let syncing = false;
let syncAgain = false;
function syncConfig() {
  if (syncing) {
    syncAgain = true;
    return;
  }
  syncing = true;
  program.FS.syncfs(false, (error) => {
    syncing = false;
    if (error) {
      console.log("[sync failed] " + error);
    }
    if (syncAgain) {
      syncAgain = false;
      syncConfig();
    } else if (!error) {
      console.log("[synced]");
    }
  });
}
let program;
(async () => {
  const staged = ${scriptValue(Boolean(staged))} ? await Promise.all((await (await fetch("stage.json")).json()).map(async (file) => {
    const response = await fetch("stage" + file.split("/").map(encodeURIComponent).join("/"));
    if (!response.ok) {
      throw new Error("could not stage " + file + ": " + response.status);
    }
    return [file, new Uint8Array(await response.arrayBuffer())];
  })) : [];
  await qtLoad({
    arguments: ${scriptValue(args)},
    mudletTransport: relay ? relayTransport() : undefined,
    mudletConfigSaved: keep?.sync
      ? () => {
          console.log("[config saved]");
          syncConfig();
        }
      : undefined,
    print: output("[out] "),
    printErr: output("[err] "),
    preRun: [(m) => {
      program = m;
      m.FS.mkdirTree("/home/web_user/.config/mudlet");
      m.FS.writeFile("/home/web_user/.config/mudlet/Mudlet.ini", ${scriptValue(settings)});
      for (const [file, data] of staged) {
        m.FS.mkdirTree(file.slice(0, file.lastIndexOf("/")) || "/");
        m.FS.writeFile(file, data);
      }
      if (keep) {
        m.FS.mkdirTree(keep.dir);
        m.FS.mount(m.FS.filesystems.IDBFS, {}, keep.dir);
        m.addRunDependency("config folder");
        m.FS.syncfs(true, (error) => {
          if (error) {
            console.log("[abort] the config folder did not load from IndexedDB: " + error);
            return;
          }
          if (!m.FS.analyzePath(keep.dir + "/Mudlet.ini").exists) {
            m.FS.writeFile(keep.dir + "/Mudlet.ini", ${scriptValue(settings)});
          }
          m.removeRunDependency("config folder");
        });
      }
    }],
    // The standard streams are opened after preRun, and main() runs after this
    onRuntimeInitialized: () => {
      if (${scriptValue(Boolean(stdoutGone))}) {
        const stdout = program.FS.getStream(1);
        // 64 is EPIPE as Emscripten numbers it
        stdout.stream_ops = { ...stdout.stream_ops, write: () => { throw new program.FS.ErrnoError(64); } };
      }
    },
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

// The exit status the page printed, if text is that line; null otherwise
function parseExit(text) {
  const exited = /^\[exit\] (.*)$/.exec(text);
  return exited ? JSON.parse(exited[1]) : null;
}

function exitedCleanly(status) {
  return status.code === 0 && !status.crashed;
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
// response for Playwright's route.fulfill() or null, for the file of that name in dir. html may be a function giving
// the page for each load, for a test that loads it again. onConsole gets each console line; onFailure the crash of the tab or an
// error the page raised. A harness failure exits 3, naming what. With reachLoopback, the page may open WebSockets to
// 127.0.0.1, which Chromium otherwise refuses a page whose address it never looked up; Playwright does not route them.
async function openPage({ what, dir, html, serve = () => null, onConsole, onFailure, reachLoopback = false }) {
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
  const tab = await browser.newPage({ permissions: reachLoopback ? ["local-network-access"] : [] });
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
      return route.fulfill({ contentType: "text/html", body: typeof html === "function" ? html() : html });
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

module.exports = { failureMarkers, outputPrefixes, pagePadding, requireBuild, timeoutFrom, page, parseExit, exitedCleanly, openPage, closeBrowser };
