local http_request = require "http.request"
local lunajson = require "lunajson"

local function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local function magiclines(s)
  if s:sub(-1)~="\n" then s=s.."\n" end
  return s:gmatch("(.-)\n")
end

-- ::warning:: turns this into an annotation on the GitHub Actions run
local function warn(message)
  print("::warning::" .. message)
end

local function scrapeLuaFunctions(htmlbody)
  local funcs = {}
  local funcsHash = {}
  local count = 0
  local duplicates = 0
  local state = 0
  local name, usage
  for line in magiclines(htmlbody) do
    if state == 0 then
      -- take the heading text rather than the anchor id: MediaWiki suffixes the id
      -- of a repeated heading with _2, _3 ... which is not part of the function name
      name = string.match(line, '<h2><span class="mw%-headline" id=".-">(.-)</span></h2>')
      if name then
        state = 1
      end
    elseif state == 1 then
      usage = string.match(line, '<dl><dt>(.-)</dt>')
      if usage then
        state = 0
        local func = {}
        func.name = trim(name)
        func.usage = trim(usage)
        table.insert(funcs, func)
      end
    end
  end

  for i, v in ipairs(funcs) do
    count = count + 1
    if not string.match(v.name, "[%:%.]") then
      if funcsHash[v.name] then
        duplicates = duplicates + 1
        warn(string.format("%s is documented more than once in the manual - keeping '%s' and dropping '%s'. Merge the duplicate headings on the wiki.",
                           v.name, funcsHash[v.name], v.usage))
      else
        funcsHash[v.name] = v.usage
      end
    end
  end

  local jsonText = lunajson.encode(funcsHash)
  print(count .. " functions in the API.")
  if duplicates > 0 then
    print(duplicates .. " duplicate function heading(s) in the manual.")
  end

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
