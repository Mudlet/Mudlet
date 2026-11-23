-- Fixture for CI/test-check-event-names.lua: CI/check-event-names.lua must find
-- nothing to report here.

function fixtureRaisesGoodNames()
  raiseEvent("sysFixtureSwept")
  raiseEvent('sysFixtureSaving', 1)
  raiseEvent(
    "sysFixtureWrapped"
  )
  -- Only names starting with sys claim to be Mudlet's
  raiseEvent("myPackageEvent")
  -- raiseEvent("sysFixtureCommentedEvent")
end
