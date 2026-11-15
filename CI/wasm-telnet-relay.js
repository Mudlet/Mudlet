// A WebSocket to telnet relay for CI/wasm-headless-smoke.js --game-connection, standing in for the proxy a browser
// build of Mudlet reaches games through. Node's own modules only, so CI downloads nothing for it. The page asks
// ws://127.0.0.1:<port>/?host=<host>&port=<port>; the handshake is answered once the game has accepted the TCP
// connection, so the page's open is the game's, and refused with 502 if it cannot be reached. Bytes pass both ways
// unchanged, and either end closing closes the other, the game's close as WebSocket close code 1000 with a reason.
// Only the one game port it is given can be reached, so it is never an open proxy.

const crypto = require("crypto");
const http = require("http");
const net = require("net");

const handshakeGuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Unmasked, as a server's frames are
function frame(opcode, payload) {
  const length = payload.length;
  let header;
  if (length < 126) {
    header = Buffer.from([0x80 | opcode, length]);
  } else if (length < 65536) {
    header = Buffer.alloc(4);
    header[1] = 126;
    header.writeUInt16BE(length, 2);
  } else {
    header = Buffer.alloc(10);
    header[1] = 127;
    header.writeBigUInt64BE(BigInt(length), 2);
  }
  header[0] = 0x80 | opcode;
  return Buffer.concat([header, payload]);
}

function closeFrame(code, reason) {
  const payload = Buffer.alloc(2 + Buffer.byteLength(reason));
  payload.writeUInt16BE(code, 0);
  payload.write(reason, 2);
  return frame(0x8, payload);
}

// The whole frames at the start of buffer, unmasked, and what is left of it
function takeFrames(buffer) {
  const frames = [];
  let offset = 0;
  while (buffer.length - offset >= 2) {
    const masked = (buffer[offset + 1] & 0x80) !== 0;
    let length = buffer[offset + 1] & 0x7f;
    let at = offset + 2;
    if (length === 126) {
      if (buffer.length - at < 2) {
        break;
      }
      length = buffer.readUInt16BE(at);
      at += 2;
    } else if (length === 127) {
      if (buffer.length - at < 8) {
        break;
      }
      length = Number(buffer.readBigUInt64BE(at));
      at += 8;
    }
    const maskAt = at;
    if (masked) {
      at += 4;
    }
    if (buffer.length - at < length) {
      break;
    }
    const payload = Buffer.from(buffer.subarray(at, at + length));
    if (masked) {
      for (let i = 0; i < length; ++i) {
        payload[i] ^= buffer[maskAt + (i & 3)];
      }
    }
    frames.push({ opcode: buffer[offset] & 0x0f, payload });
    offset = at + length;
  }
  return [frames, buffer.subarray(offset)];
}

function relay(request, page, gamePort, log) {
  const asked = new URL(request.url, "http://relay");
  const host = asked.searchParams.get("host");
  const port = Number(asked.searchParams.get("port"));
  if (!["127.0.0.1", "localhost"].includes(host) || port !== gamePort) {
    log(`refused ${host}:${port}`);
    page.end("HTTP/1.1 403 Forbidden\r\n\r\n");
    return;
  }
  let open = false;
  let pending = Buffer.alloc(0);
  const game = net.connect(port, "127.0.0.1");
  game.on("connect", () => {
    const accept = crypto.createHash("sha1").update(request.headers["sec-websocket-key"] + handshakeGuid).digest("base64");
    page.write(`HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ${accept}\r\n\r\n`);
    open = true;
    log(`connected to ${host}:${port}`);
  });
  game.on("data", (data) => page.write(frame(0x2, data)));
  game.on("error", (error) => {
    log(`game connection failed: ${error.message}`);
    if (!open) {
      page.end("HTTP/1.1 502 Bad Gateway\r\n\r\n");
    }
  });
  game.on("close", () => {
    if (open && !page.destroyed) {
      log("the game closed the connection");
      page.end(closeFrame(1000, "the game closed the connection"));
    }
  });
  page.on("data", (data) => {
    let frames;
    [frames, pending] = takeFrames(Buffer.concat([pending, data]));
    for (const { opcode, payload } of frames) {
      if (opcode === 0x8) {
        log("the page closed the connection");
        page.end(frame(0x8, payload.subarray(0, 2)));
        game.end();
      } else if (opcode === 0x9) {
        page.write(frame(0xa, payload));
      } else if (opcode <= 0x2) {
        game.write(payload);
      }
    }
  });
  page.on("error", () => {});
  page.on("close", () => game.destroy());
}

// Resolves to { url, close() } once it listens on an ephemeral port of 127.0.0.1
function startRelay({ gamePort, log = () => {} }) {
  const server = http.createServer((request, response) => response.writeHead(426).end());
  const pages = new Set();
  server.on("upgrade", (request, page) => {
    pages.add(page);
    page.on("close", () => pages.delete(page));
    relay(request, page, gamePort, log);
  });
  return new Promise((resolve, reject) => {
    server.once("error", reject);
    server.listen(0, "127.0.0.1", () => {
      resolve({
        url: `ws://127.0.0.1:${server.address().port}/`,
        close: () => {
          for (const page of pages) {
            page.destroy();
          }
          server.close();
        },
      });
    });
  });
}

module.exports = { startRelay };
