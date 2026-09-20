-- enableX()/disableX() look an item up by name and have to toggle EVERY item
-- sharing that name, not just the first one found - two groups both named
-- "Druid Aliases", say. Aliases, triggers, timers and keys share one
-- equal_range lookup in their unit, while scripts are found by a separate scan
-- of the script list, so each type is driven here rather than one standing in
-- for the rest: two same-named items plus a differently-named control, toggled
-- through the real Lua functions and read back per item by id.
--
-- Permanent items have no removal API, so every block switches its own off
-- again. That state is saved with the profile, which is what keeps a second run
-- against the same profile from finding a pile of same-named items still live.

describe("enable/disable by name", function()

  local permanentItems

  before_each(function()
    permanentItems = {}
  end)

  after_each(function()
    -- the disable is the same call the specs exercise, so a failure here is the
    -- regression they are about and has to be said out loud: left enabled, these
    -- items are saved with the profile and fire on the next run
    local stillEnabled = {}
    for _, item in ipairs(permanentItems) do
      if not item.disable(item.name) then
        stillEnabled[#stillEnabled + 1] = item.name
      end
    end
    permanentItems = {}
    _G.EnableDisableSpec = nil
    assert.are.equal("", table.concat(stillEnabled, ", "), "these items could not be switched off again")
  end)

  local function track(disable, name)
    permanentItems[#permanentItems + 1] = {disable = disable, name = name}
    return name
  end

  -- enableX(dupName) activates BOTH same-named items, disableX(dupName)
  -- deactivates both while leaving the differently-named control untouched, and
  -- enableX(dupName) brings both back.
  local function togglesEverySameNamedItem(itemType, dupName, soloName, ids, enable, disable)
    local function active(id)
      return isActive(id, itemType) == 1
    end

    enable(dupName)
    enable(soloName)
    assert.is_true(active(ids.first), "the first " .. itemType .. " should be enabled after enabling by name")
    assert.is_true(active(ids.second), "the second same-named " .. itemType .. " should be enabled after enabling by name")
    assert.is_true(active(ids.control), "the control " .. itemType .. " should be enabled after enabling by name")

    disable(dupName)
    assert.is_false(active(ids.first), "the first " .. itemType .. " should be disabled")
    assert.is_false(active(ids.second), "the second same-named " .. itemType .. " should ALSO be disabled")
    assert.is_true(active(ids.control), "the differently-named " .. itemType .. " must stay enabled")

    enable(dupName)
    assert.is_true(active(ids.first), "the first " .. itemType .. " should be re-enabled")
    assert.is_true(active(ids.second), "the second same-named " .. itemType .. " should ALSO be re-enabled")
  end

  local function assertCreated(ids)
    assert.is_true(ids.first > 0, "the first item should have been created")
    assert.is_true(ids.second > 0, "the second same-named item should have been created")
    assert.is_true(ids.control > 0, "the control item should have been created")
  end

  it("toggles every alias sharing a name", function()
    local dupName = track(disableAlias, "Spec Dup Aliases")
    local soloName = track(disableAlias, "Spec Solo Alias")
    local ids = {
      first = permAlias(dupName, "", "^spec_dup_alias_1$", ""),
      second = permAlias(dupName, "", "^spec_dup_alias_2$", ""),
      control = permAlias(soloName, "", "^spec_dup_alias_3$", ""),
    }
    assertCreated(ids)

    togglesEverySameNamedItem("alias", dupName, soloName, ids, enableAlias, disableAlias)
  end)

  it("toggles every trigger sharing a name", function()
    local dupName = track(disableTrigger, "Spec Dup Triggers By Name")
    local soloName = track(disableTrigger, "Spec Solo Trigger By Name")
    local ids = {
      first = permSubstringTrigger(dupName, "", {"spec_dup_trig_1"}, ""),
      second = permSubstringTrigger(dupName, "", {"spec_dup_trig_2"}, ""),
      control = permSubstringTrigger(soloName, "", {"spec_dup_trig_3"}, ""),
    }
    assertCreated(ids)

    togglesEverySameNamedItem("trigger", dupName, soloName, ids, enableTrigger, disableTrigger)
  end)

  it("toggles every timer sharing a name", function()
    local dupName = track(disableTimer, "Spec Dup Timers")
    local soloName = track(disableTimer, "Spec Solo Timer")
    local ids = {
      first = permTimer(dupName, "", 60, ""),
      second = permTimer(dupName, "", 60, ""),
      control = permTimer(soloName, "", 60, ""),
    }
    assertCreated(ids)

    togglesEverySameNamedItem("timer", dupName, soloName, ids, enableTimer, disableTimer)
  end)

  it("toggles every key sharing a name", function()
    local dupName = track(disableKey, "Spec Dup Keys")
    local soloName = track(disableKey, "Spec Solo Key")
    local ids = {
      first = permKey(dupName, "", mudlet.key.F7, ""),
      second = permKey(dupName, "", mudlet.key.F8, ""),
      control = permKey(soloName, "", mudlet.key.F9, ""),
    }
    assertCreated(ids)

    togglesEverySameNamedItem("keybind", dupName, soloName, ids, enableKey, disableKey)
  end)

  it("toggles every script sharing a name", function()
    local dupName = track(disableScript, "Spec Dup Scripts")
    local soloName = track(disableScript, "Spec Solo Script")
    -- an empty body would make each of these a script folder rather than a
    -- script, and the folder is not the thing this is about
    local body = "-- deliberately does nothing"
    local ids = {
      first = permScript(dupName, "", body),
      second = permScript(dupName, "", body),
      control = permScript(soloName, "", body),
    }
    assertCreated(ids)

    togglesEverySameNamedItem("script", dupName, soloName, ids, enableScript, disableScript)
  end)

  -- setTriggerStayOpen() looks triggers up by name through the same path as the
  -- enable/disable functions, so it has to update EVERY same-named trigger too.
  it("sets the stay-open line count on every trigger sharing a name", function()
    -- mKeepFiring has no Lua getter, so it is read behaviourally: while it is
    -- positive a parent offers its children lines its own pattern does not
    -- match, counting one down per line - so a parent set to five gives its
    -- child exactly the next five lines and nothing after them.
    local dupName = track(disableTrigger, "Spec StayOpen Parents")
    local soloName = track(disableTrigger, "Spec Solo StayOpen")
    local fired = {}
    _G.EnableDisableSpec = {record = function(which) fired[#fired + 1] = which end}

    assert.is_true(permSubstringTrigger(dupName, "", {"so_gate_a"}, "") > 0)
    assert.is_true(permSubstringTrigger(track(disableTrigger, "Spec StayOpen Child A"), dupName, {"so_"},
                                        [[_G.EnableDisableSpec.record("A")]]) > 0)
    -- the second parent is created after the first child, so the child added
    -- next lands under it rather than under the first parent
    assert.is_true(permSubstringTrigger(dupName, "", {"so_gate_b"}, "") > 0)
    assert.is_true(permSubstringTrigger(track(disableTrigger, "Spec StayOpen Child B"), dupName, {"so_"},
                                        [[_G.EnableDisableSpec.record("B")]]) > 0)
    assert.is_true(permSubstringTrigger(soloName, "", {"so_gate_c"}, "") > 0)
    assert.is_true(permSubstringTrigger(track(disableTrigger, "Spec StayOpen Child C"), soloName, {"so_"},
                                        [[_G.EnableDisableSpec.record("C")]]) > 0)

    -- each parent reaches its own child and no other, or the check below would
    -- pass on one parent doing all the work
    fired = {}
    feedTriggers("so_gate_a\n")
    assert.are.equal("A", table.concat(fired, ","), "the first parent should reach only the first child")
    fired = {}
    feedTriggers("so_gate_b\n")
    assert.are.equal("B", table.concat(fired, ","), "the second parent should reach only the second child")
    fired = {}
    feedTriggers("so_gate_c\n")
    assert.are.equal("C", table.concat(fired, ","), "the control parent should reach only the control child")

    fired = {}
    setTriggerStayOpen(dupName, 5)
    for _ = 1, 6 do
      feedTriggers("so_payload\n")
    end

    assert.are.equal("A,B,A,B,A,B,A,B,A,B", table.concat(fired, ","),
      "both same-named parents should stay open for exactly five lines, and the differently-named one not at all")
  end)

end)
