-- Fixture for CI/test-check-event-names.lua: every line ending in "-- BAD" must
-- be reported by CI/check-event-names.lua, and no other line in this file.

function fixtureRaisesBadNames()
  raiseEvent("sysFixtureResizeEvent") -- BAD
  raiseEvent('sysFixtureProgress', 50) -- BAD
  raiseEvent(
    "sysFixtureWrapEvent", -- BAD
    1
  )
  raiseEvent("sysFixtureWalked")
end
