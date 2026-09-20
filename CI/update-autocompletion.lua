local http_request = require "http.request"
local lunajson = require "lunajson"

-- A scrape that keeps far less than the list it replaces has hit a markup change
-- rather than a week of wiki edits, so it must not be shipped. One or two headings
-- the scraper cannot read are a quirk of the manual and only warn; more than the
-- allowance is the markup having moved underneath it.
local keepRatio = 0.95
local unusableAllowance = 5

local function trim(s)
  return (s:gsub("^%s*(.-)%s*$", "%1"))
end

local function magiclines(s)
  if s:sub(-1)~="\n" then s=s.."\n" end
  return s:gmatch("(.-)\n")
end

-- ::warning:: turns this into an annotation on the GitHub Actions run. The prefix
-- says which kind it is: the workflow lists the two under separate headings, and
-- a wiki problem and a scraper problem need different people to act on them.
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
  -- The heading also holds MediaWiki's section-edit link, and the capture above runs
  -- past the headline's own </span> to reach the closing </h2>, so both cuts below are
  -- needed on every heading: the first drops that link, the second the headline's own
  -- closing tag (and any tag a heading is wrapped in). Without the first, the link's
  -- visible text stays behind and the name reads "addAreaName[edit | edit source]".
  name = name:gsub('<span class="mw%-editsection">.*', "")
  return trim((name:gsub("<[^>]*>", "")))
end

-- The manual documents a seventh of its functions under a namespace - db:add,
-- table.insert, string.cut, utf8.width, lfs.attributes. They are callable, but the
-- editor's list is of plain names, so they are left out, and silently. Recognising
-- them positively is what keeps the warning below a genuine catch-all: a section-edit
-- link that leaked into a name often carries a colon of its own, as in
-- "addAreaName[edit | Manual:edit source]", and would otherwise be taken for one of these.
local function namespacedName(name)
  -- every segment on either side of a separator has to be an identifier of its own:
  -- "db:add:" and "table.insert." are a heading something has mangled, not a namespace
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
      -- a heading always re-binds the name, even while waiting for a signature: a section
      -- that writes its signature in prose instead of <dl><dt> would otherwise be handed
      -- the next section's signature, and that section's own heading lost with it
      name = heading
      state = 1
    elseif state == 1 then
      usage = string.match(line, '<dl><dt>(.-)</dt>')
      if usage then
        state = 0
        local func = {}
        func.name = name
        -- the signature is shown beside the name in the editor, so it has to be text:
        -- a wiki edit that bolds or links part of it would otherwise put tags on screen
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

  -- Nothing downstream can tell a wiki that lost functions from a scraper that stopped
  -- reading them: the update opens a green pull request either way. So refuse here.
  if kept == 0 then
    error("no function names could be read from the manual - the wiki's heading markup has changed and the patterns in this script no longer match it")
  end
  -- the allowance is a count and nothing else: measuring the unreadable headings against
  -- the size of the page lets a handful of them through on a page this long, and they are
  -- functions the editor stops offering
  if unusable > unusableAllowance then
    error(string.format("%d headings could not be read as a function name, more than the %d a week of wiki edits explains - the wiki's heading markup has changed",
                        unusable, unusableAllowance))
  end

  return lunajson.encode(funcsHash), kept
end

-- how many functions the list being replaced holds, or nil when there is none to compare with
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
