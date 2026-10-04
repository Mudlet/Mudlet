mpackage = [[mudlet-spec-trickyconfig]]
author = [[Mudlet test suite]]
title = [[Fixture whose config.lua leaves its globals in an awkward state]]
-- Each of these used to abort Mudlet while the manifest was read back
local globals = _G
globals[1] = [[a global with a number for its name]]
_VERSION = nil
_G = nil
setmetatable(globals, {
  __index = function() error("no such global") end,
  __newindex = function() error("no new globals") end,
})
