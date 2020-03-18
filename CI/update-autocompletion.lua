local http_request = require "http.request"
local lunajson = require "lunajson"

local function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local function magiclines(s)
  if s:sub(-1)~="\n" then s=s.."\n" end
  return s:gmatch("(.-)\n")
end

local function scrapeLuaFunctions(htmlbody)
  local funcs = {}
  local funcsHash = {}
  local count = 0
  local state = 0
  local line = nil
  local match = nil
  local name, usage, definition
  for line in magiclines(htmlbody) do
    if state == 0 then
      --print("testing match on " .. line)
      name = string.match(line, '<h2><span class="mw%-headline" id="(.-)">.-</span></h2>')
      if name then
        state = 1
        --print("Name: " .. name)
      end
    elseif state == 1 then
      usage = string.match(line, '<dl><dt>(.-)</dt>')
      if usage then
        state = 0
        --print("Usage: " .. usage)
        func = {}
        func.name = trim(name)
        func.usage = trim(usage)
        table.insert(funcs, func)
      end
    end
  end

  funcsHash = {}
  local count = 0
  for i, v in ipairs(funcs) do
    count = count + 1
    --print(v.name .. " - " .. v.usage)
    if not string.match(v.name, "[%:%.]") then
      funcsHash[v.name] = v.usage
    end
  end

  jsonText = lunajson.encode(funcsHash)
  print(count .. " functions in the API.")

  return jsonText
end

local headers, stream = assert(http_request.new_from_uri("https://wiki.mudlet.org/w/Manual:Lua_Functions"):go())
local body = assert(stream:get_body_as_string())
if headers:get ":status" ~= "200" then
    error(body)
end

local data = scrapeLuaFunctions(body)

local f = io.open(arg[1], "w")
io.output(f)
io.write(data)
io.close(f)
