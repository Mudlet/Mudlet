-- Serves a corpus file N times over TCP, bracketed by BEGIN/END markers, as fast
-- as the socket will take it. Afterwards records the first lines the client
-- writes back and counts the rest (CLIENT_LINES).
-- usage: lua5.1 logserver.lua <corpus> <port> <repeats> <log>
local socket = require("socket")
local corpusPath, port, repeats, logPath = arg[1], tonumber(arg[2]), tonumber(arg[3] or "10"), arg[4]
local logf = assert(io.open(logPath, "w"))
local function note(m)
  logf:write(os.date("%H:%M:%S ") .. m .. "\n")
  logf:flush()
end

local f = assert(io.open(corpusPath, "rb"))
local data = f:read("*a")
f:close()

local server = assert(socket.bind("127.0.0.1", port))
server:settimeout(120)
note(("listening on %d, %d bytes x %d"):format(port, #data, repeats))
local client, err = server:accept()
if not client then
  note("accept failed: " .. tostring(err))
  os.exit(1)
end
client:setoption("tcp-nodelay", true)
note("client connected")

local t0 = socket.gettime()
client:settimeout(nil)
assert(client:send("--- BEGIN MEASURING ---\r\n"))
for _ = 1, repeats do
  client:settimeout(0)
  client:receive(8192)
  client:settimeout(nil)
  local ok, serr = client:send(data)
  if not ok then
    note("send failed: " .. tostring(serr))
    os.exit(1)
  end
end
assert(client:send("--- END MEASURING ---\r\n"))
local elapsed = socket.gettime() - t0
local total = #data * repeats
note(("sent %d bytes in %.3fs (%.1f MB/s)"):format(total, elapsed, total / elapsed / 1048576))

client:settimeout(1)
local clientLines, logged, idle = 0, 0, 0
while idle < 240 do
  local line, rerr = client:receive("*l")
  if line then
    idle = 0
    line = line:gsub("[^\32-\126]", "")
    if #line > 0 then
      clientLines = clientLines + 1
      if logged < 60 then
        logged = logged + 1
        note("CLIENT> " .. line)
      end
    end
  elseif rerr == "closed" then
    note("client closed")
    break
  else
    idle = idle + 1
  end
end
note("CLIENT_LINES " .. clientLines)
client:close()
server:close()
note("done")
