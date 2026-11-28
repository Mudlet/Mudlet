-- verify-mudlet stages this as "!verify_results_spec.lua": busted sorts spec paths and "!" sorts first,
-- so it subscribes before any other spec runs. Each outcome is written and flushed as it happens,
-- so a run that crashes still says how far it got and which test it died in.
-- Bound now: specs stub and spy on globals such as pcall, and count the calls
local pcall, tostring, type, error, insert, concat = pcall, tostring, type, error, table.insert, table.concat
local path = os.getenv("MUDLET_VERIFY_RESULTS")
local b = rawget(_G, "busted")
if not path then
  return
end
if type(b) ~= "table" or type(b.subscribe) ~= "function" or type(b.context) ~= "table" then
  error("verify-mudlet results hook: this busted has no subscribe/context API")
end
local out = assert(io.open(path, "w"))
local details = assert(io.open(path .. ".details", "w"))

local function clean(text)
  return (tostring(text):gsub("[\t\r\n]", " "))
end

local function fullName(element)
  -- a file's name is its staged path, which differs between the two sides of a diff
  if element.descriptor == "file" then
    return "file"
  end
  local names = {element.name or element.descriptor}
  local parent = b.context.parent(element)
  while parent and (parent.name or parent.descriptor) and parent.descriptor ~= "file" do
    insert(names, 1, parent.name or parent.descriptor)
    parent = b.context.parent(parent)
  end
  return clean(concat(names, " "))
end

local function fileOf(element)
  local name = element.descriptor == "file" and element.name
  if not name then
    local file = b.getFile(element)
    name = file and file.name or "?"
  end
  return clean(name:match("([^/\\]+)$") or name)
end

local function write(kind, element)
  out:write(kind, "\t", fileOf(element), "\t", fullName(element), "\n")
  out:flush()
end

-- busted turns a subscriber's error into a failure of the test being reported, so none may escape
local broken = false
local function subscribe(channel, handler)
  b.subscribe(channel, function(...)
    local ok, err = pcall(handler, ...)
    if not ok and not broken then
      broken = true
      details:write("=== verify-mudlet results hook failed\n", clean(err), "\n")
      details:flush()
    end
    return nil, true
  end)
end

subscribe({"test", "start"}, function(element)
  write("start", element)
end)
subscribe({"test", "end"}, function(element, _, status)
  write(status, element)
end)
-- A test's own failure is counted by its "end"; outside a test - in a hook such as before_each, or a
-- spec file that does not load - this is the only record of it
for _, kind in ipairs({"failure", "error"}) do
  subscribe({kind}, function(element, _, message, debug)
    if element.descriptor ~= "it" and element.descriptor ~= "pending" then
      write(kind, element)
    end
    local trace = type(debug) == "table" and debug.traceback or ""
    details:write("=== ", kind, " ", fileOf(element), ": ", fullName(element), "\n", tostring(message), "\n", tostring(trace), "\n")
    details:flush()
  end)
end
-- What tells a run that stopped between tests or in an after_all hook from one that finished
subscribe({"suite", "end"}, function()
  local finished = assert(io.open(path .. ".finished", "w"))
  finished:write("1\n")
  finished:close()
end)
