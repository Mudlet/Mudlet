-- verify-mudlet stages this as "!verify_results_spec.lua": busted sorts spec paths and "!" sorts first,
-- so it subscribes before any other spec runs. Each outcome is written and flushed as it happens,
-- so a run that crashes still says how far it got and which test it died in.
-- Bound now: specs stub and spy on globals such as pcall, and count the calls
local pcall, tostring, type, insert, concat = pcall, tostring, type, table.insert, table.concat
local path = os.getenv("MUDLET_VERIFY_RESULTS")
local b = rawget(_G, "busted")
if not path or type(b) ~= "table" or type(b.subscribe) ~= "function" or type(b.context) ~= "table" then
  return
end
local out = io.open(path, "w")
local details = io.open(path .. ".details", "w")
if not out or not details then
  return
end

local function clean(text)
  return (tostring(text):gsub("[\t\r\n]", " "))
end

local function fullName(element)
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
local function subscribe(channel, handler)
  b.subscribe(channel, function(...)
    pcall(handler, ...)
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
