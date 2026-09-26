local http_request = require "http.request"
local lunajson = require "lunajson"

-- Keeping under keepRatio of the old list means a markup change, not wiki edits; don't ship it.
-- Up to unusableAllowance unreadable headings are manual quirks and only warn; more means the markup moved.
local keepRatio = 0.95
local unusableAllowance = 5

local function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local function magiclines(s)
  if s:sub(-1)~="\n" then s=s.."\n" end
  return s:gmatch("(.-)\n")
end

-- ::warning:: makes an Actions annotation; the workflow sorts them by the kind prefix.
local function warn(kind, message)
  print("::warning::" .. kind .. ": " .. message)
end

local function headingName(line)
  -- take the heading text rather than the anchor id: MediaWiki suffixes the id
  -- of a repeated heading with _2, _3 ... which is not part of the function name
  local name = string.match(line, '<h2><span class="mw%-headline" id=".-">(.-)</span></h2>')
  if not name then
    return nil
  end
  -- The capture runs to </h2>, so it holds the section-edit link and the headline's closing tags:
  -- drop the link first, or its text stays in the name ("addAreaName[edit | edit source]").
  name = name:gsub('<span class="mw%-editsection">.*', "")
  return trim((name:gsub("<[^>]*>", "")))
end

-- Namespaced functions (db:add, table.insert) are silently left out: the editor lists plain names.
-- Matched positively so leaked edit-link text with a colon ("[edit | Manual:edit source]") still warns.
local function namespacedName(name)
  -- every segment must be an identifier: "db:add:" or "table.insert." is a mangled heading
  local rest = string.match(name, "^[%a_][%w_]*[%.:](.*)$")
  if not rest then
    return false
  end
  return string.match(rest, "^[%a_][%w_]*$") ~= nil or namespacedName(rest)
end

local function scrapeLuaFunctions(htmlbody)
  local funcs = {}
  local funcsHash = {}
  local kept = 0
  local namespaced = 0
  local unusable = 0
  local duplicates = 0
  local state = 0
  local name, usage
  for line in magiclines(htmlbody) do
    local heading = headingName(line)
    if heading then
      -- re-bind even while awaiting a signature, or a section with a prose signature (no <dl><dt>)
      -- would take the next section's signature
      name = heading
      state = 1
    elseif state == 1 then
      usage = string.match(line, '<dl><dt>(.-)</dt>')
      if usage then
        state = 0
        local func = {}
        func.name = name
        -- the editor shows the signature as text, so strip any tags a wiki edit added
        func.usage = trim((usage:gsub("<[^>]*>", "")))
        table.insert(funcs, func)
      end
    end
  end

  for _, v in ipairs(funcs) do
    if string.match(v.name, "^[%a_][%w_]*$") then
      if funcsHash[v.name] then
        duplicates = duplicates + 1
        warn("duplicate heading", string.format("%s is documented more than once in the manual - keeping '%s' and dropping '%s'. Merge the duplicate headings on the wiki.",
                                                v.name, funcsHash[v.name], v.usage))
      else
        funcsHash[v.name] = v.usage
        kept = kept + 1
      end
    elseif namespacedName(v.name) then
      namespaced = namespaced + 1
    else
      unusable = unusable + 1
      warn("unusable name", string.format("'%s' is not a function name - leaving it out of the list. Check the heading for it in the wiki manual.",
                                          v.name))
    end
  end

  print(string.format("%d of %d headings kept as functions (%d namespaced, %d unreadable).", kept, #funcs, namespaced, unusable))
  if duplicates > 0 then
    print(duplicates .. " duplicate function heading(s) in the manual.")
  end

  -- A wiki that lost functions and a broken scraper both open a green PR, so fail here.
  if kept == 0 then
    error("no function names could be read from the manual - the wiki's heading markup has changed and the patterns in this script no longer match it")
  end
  -- An absolute count, not a share of the page: on a page this long a share lets lost functions through.
  if unusable > unusableAllowance then
    error(string.format("%d headings could not be read as a function name, more than the %d a week of wiki edits explains - the wiki's heading markup has changed",
                        unusable, unusableAllowance))
  end

  return lunajson.encode(funcsHash), kept
end

-- nil when there is no previous list to compare with
local function entriesIn(path)
  local file = io.open(path, "r")
  if not file then
    return nil
  end
  local text = file:read("*a")
  file:close()
  local ok, decoded = pcall(lunajson.decode, text)
  if not ok or type(decoded) ~= "table" then
    return nil
  end
  local entries = 0
  for _ in pairs(decoded) do
    entries = entries + 1
  end
  return entries
end

local headers, stream = assert(http_request.new_from_uri("https://wiki.mudlet.org/w/Manual:Lua_Functions"):go())
local body = assert(stream:get_body_as_string())
if headers:get ":status" ~= "200" then
  error(string.format("the wiki manual returned HTTP %s: %s", tostring(headers:get ":status"), string.sub(body, 1, 500)))
end

local data, kept = scrapeLuaFunctions(body)

local previous = entriesIn(arg[1])
if previous and kept < previous * keepRatio then
  error(string.format("only %d function names scraped, down from %d in %s - the manual does not lose that many in a week, so the markup has changed",
                      kept, previous, arg[1]))
end

local out = assert(io.open(arg[1], "w"), "cannot write " .. tostring(arg[1]))
out:write(data)
out:close()
