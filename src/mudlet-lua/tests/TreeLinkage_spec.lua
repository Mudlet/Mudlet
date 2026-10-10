-- Triggers, aliases, timers, keys and scripts are all nodes in a tree: each one
-- holds a list of its children and a pointer back to its parent. isActive(name,
-- type, true) reads the parent chain, by name or by ID, and the timer block
-- below is the only enable/disable that descends the children list - the other
-- four types are switched through their unit's name lookup table - so between
-- them both links are covered.
--
-- Each item type keeps its own copy of that walk, so every type is driven here
-- rather than one standing in for the rest. Permanent items have no removal API,
-- so every block switches its own items off again - left enabled they are saved
-- with the profile and are live on the next run.

describe("tree linkage", function()

  local permanentItems

  before_each(function()
    permanentItems = {}
  end)

  after_each(function()
    local stillEnabled = {}
    for _, item in ipairs(permanentItems) do
      if not item.disable(item.name) then
        stillEnabled[#stillEnabled + 1] = item.name
      end
    end
    permanentItems = {}
    assert.are.equal("", table.concat(stillEnabled, ", "), "these items could not be switched off again")
  end)

  local function track(disable, name)
    permanentItems[#permanentItems + 1] = {disable = disable, name = name}
    return name
  end

  local function assertCreated(ids, itemType)
    for _, key in ipairs({"top", "mid", "leaf", "sibling"}) do
      assert.is_true((ids[key] or 0) > 0, "the " .. key .. " " .. itemType .. " should have been created")
    end
  end

  -- The shape every block builds:
  --
  -- top
  --  |- mid
  --  |   `- leaf
  --  `- sibling

  -- Asked by ID, isActive() must give the same answer as asked by name.
  local function askBoth(itemType, names, ids, key, checkAncestors)
    local byName = isActive(names[key], itemType, checkAncestors)
    assert.are.equal(byName, isActive(ids[key], itemType, checkAncestors),
      "asked by id, the " .. key .. " " .. itemType .. " should get the same answer as asked by name")
    return byName == 1
  end

  -- Turning a group off silences what hangs below it without touching those
  -- items' own switches, and reaches only its own branch.
  local function silencesOnlyItsOwnBranch(itemType, names, ids, enable, disable)
    local function effective(key)
      return askBoth(itemType, names, ids, key, true)
    end
    local function switchedOn(key)
      return askBoth(itemType, names, ids, key, false)
    end

    enable(names.top)
    enable(names.mid)
    enable(names.leaf)
    enable(names.sibling)
    assert.is_true(effective("leaf"), "the leaf should be effective with both groups above it on")
    assert.is_true(effective("sibling"), "the sibling should be effective with its group on")

    disable(names.top)
    assert.is_false(effective("leaf"), "the leaf should be silenced by the group two levels above it")
    assert.is_false(effective("sibling"), "the sibling should be silenced by the group above it")
    assert.is_true(switchedOn("leaf"), "the leaf's own switch must survive an ancestor being turned off")
    assert.is_true(switchedOn("sibling"), "the sibling's own switch must survive an ancestor being turned off")

    enable(names.top)
    assert.is_true(effective("leaf"), "the leaf should come back when the group above it does")
    assert.is_true(effective("sibling"), "the sibling should come back when its group does")

    disable(names.mid)
    assert.is_false(effective("leaf"), "the leaf should be silenced by its immediate group")
    assert.is_true(effective("sibling"), "the sibling hangs off top, not mid, so it must be untouched")
  end

  it("links triggers to the groups above them", function()
    local names = {
      top = track(disableTrigger, "Spec Tree Trigger Top"),
      mid = track(disableTrigger, "Spec Tree Trigger Mid"),
      leaf = track(disableTrigger, "Spec Tree Trigger Leaf"),
      sibling = track(disableTrigger, "Spec Tree Trigger Sibling"),
    }
    local ids = {
      top = permSubstringTrigger(names.top, "", {}, ""),
      mid = permSubstringTrigger(names.mid, names.top, {}, ""),
      leaf = permSubstringTrigger(names.leaf, names.mid, {"spec_tree_trig_leaf"}, ""),
      sibling = permSubstringTrigger(names.sibling, names.top, {"spec_tree_trig_sibling"}, ""),
    }
    assertCreated(ids, "trigger")

    silencesOnlyItsOwnBranch("trigger", names, ids, enableTrigger, disableTrigger)
  end)

  it("links aliases to the groups above them", function()
    local names = {
      top = track(disableAlias, "Spec Tree Alias Top"),
      mid = track(disableAlias, "Spec Tree Alias Mid"),
      leaf = track(disableAlias, "Spec Tree Alias Leaf"),
      sibling = track(disableAlias, "Spec Tree Alias Sibling"),
    }
    local ids = {
      top = permAlias(names.top, "", "", ""),
      mid = permAlias(names.mid, names.top, "", ""),
      leaf = permAlias(names.leaf, names.mid, "^spec_tree_alias_leaf$", ""),
      sibling = permAlias(names.sibling, names.top, "^spec_tree_alias_sibling$", ""),
    }
    assertCreated(ids, "alias")

    silencesOnlyItsOwnBranch("alias", names, ids, enableAlias, disableAlias)
  end)

  it("links keys to the groups above them", function()
    local names = {
      top = track(disableKey, "Spec Tree Key Top"),
      mid = track(disableKey, "Spec Tree Key Mid"),
      leaf = track(disableKey, "Spec Tree Key Leaf"),
      sibling = track(disableKey, "Spec Tree Key Sibling"),
    }
    local ids = {
      top = permKey(names.top, "", -1, ""),
      mid = permKey(names.mid, names.top, -1, ""),
      leaf = permKey(names.leaf, names.mid, mudlet.key.F10, ""),
      sibling = permKey(names.sibling, names.top, mudlet.key.F11, ""),
    }
    assertCreated(ids, "keybind")

    silencesOnlyItsOwnBranch("keybind", names, ids, enableKey, disableKey)
  end)

  it("links scripts to the groups above them", function()
    local names = {
      top = track(disableScript, "Spec Tree Script Top"),
      mid = track(disableScript, "Spec Tree Script Mid"),
      leaf = track(disableScript, "Spec Tree Script Leaf"),
      sibling = track(disableScript, "Spec Tree Script Sibling"),
    }
    -- an empty body is what makes an item a script folder rather than a script,
    -- which is exactly what the two groups have to be
    local body = "-- deliberately does nothing"
    local ids = {
      top = permScript(names.top, "", ""),
      mid = permScript(names.mid, names.top, ""),
      leaf = permScript(names.leaf, names.mid, body),
      sibling = permScript(names.sibling, names.top, body),
    }
    assertCreated(ids, "script")

    silencesOnlyItsOwnBranch("script", names, ids, enableScript, disableScript)
  end)

  -- Timers are the one type that cannot merely be masked by an ancestor: a timer
  -- under a group that goes off has to have its real QTimer stopped, so
  -- disabling a node walks the children list turning every descendant off
  -- outright rather than leaving their own switches up. Enabling walks the same
  -- list to start them again.
  it("stops the timers below a group outright, and starts them again", function()
    local names = {
      top = track(disableTimer, "Spec Tree Timer Top"),
      mid = track(disableTimer, "Spec Tree Timer Mid"),
      leaf = track(disableTimer, "Spec Tree Timer Leaf"),
      sibling = track(disableTimer, "Spec Tree Timer Sibling"),
    }
    local ids = {
      top = permTimer(names.top, "", 0, ""),
      mid = permTimer(names.mid, names.top, 0, ""),
      leaf = permTimer(names.leaf, names.mid, 60, ""),
      sibling = permTimer(names.sibling, names.top, 60, ""),
    }
    assertCreated(ids, "timer")

    local function running(key)
      return askBoth("timer", names, ids, key, true)
    end
    local function switchedOn(key)
      return askBoth("timer", names, ids, key, false)
    end

    enableTimer(names.top)
    enableTimer(names.mid)
    enableTimer(names.leaf)
    enableTimer(names.sibling)
    assert.is_true(running("leaf"), "the leaf timer should be running with both groups above it on")
    assert.is_true(running("sibling"), "the sibling timer should be running with its group on")

    disableTimer(names.top)
    assert.is_false(running("leaf"), "the leaf timer should stop when a group above it is turned off")
    assert.is_false(running("sibling"), "the sibling timer should stop when its group is turned off")
    assert.is_false(switchedOn("leaf"), "the leaf timer's own switch goes down too - its QTimer really has to stop")
    assert.is_false(switchedOn("sibling"), "the sibling timer's own switch goes down too")

    enableTimer(names.top)
    assert.is_true(running("leaf"), "the leaf timer should start again with the group above it")
    assert.is_true(running("sibling"), "the sibling timer should start again with its group")

    disableTimer(names.mid)
    enableTimer(names.leaf)
    assert.is_true(switchedOn("leaf"), "the leaf timer's own switch can go up again under a group that is off")
    assert.is_false(running("leaf"), "a leaf timer switched on under a group that is off must not count as running")
  end)
end)
