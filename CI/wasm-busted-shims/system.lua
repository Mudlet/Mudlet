-- What busted takes from luasystem, a C module a browser build cannot load: the time for its reports,
-- a monotonic clock for durations, and sleep(). Emscripten's clock() counts from performance.now(),
-- so it goes on through the time a page spends waiting, which a process's CPU time would not.
local system = {}

function system.gettime()
  return getEpoch()
end

function system.monotime()
  return os.clock()
end

-- Busy, as nothing in a page can block without handing the thread back to the browser
function system.sleep(seconds)
  local deadline = os.clock() + seconds
  while os.clock() < deadline do
  end
end

return system
