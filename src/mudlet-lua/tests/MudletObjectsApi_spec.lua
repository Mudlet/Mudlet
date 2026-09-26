-- Edges of the Lua API in TLuaInterpreterMudletObjects.cpp that the per-subsystem
-- specs (Trigger_spec, Other_spec, UI_spec...) leave out: argument validation,
-- the failure returns, and the less travelled branches of the creators.
describe("Mudlet object API edges", function()

  describe("permanent trigger creators", function()
    local creators = {
      permRegexTrigger = function(name, parent, code) return permRegexTrigger(name, parent, {"^objSpecNeverSeen$"}, code or [[]]) end,
      permBeginOfLineStringTrigger = function(name, parent, code) return permBeginOfLineStringTrigger(name, parent, {"objSpecNeverSeen"}, code or [[]]) end,
      permSubstringTrigger = function(name, parent, code) return permSubstringTrigger(name, parent, {"objSpecNeverSeen"}, code or [[]]) end,
      permExactMatchTrigger = function(name, parent, code) return permExactMatchTrigger(name, parent, {"objSpecNeverSeen"}, code or [[]]) end,
      permPromptTrigger = function(name, parent, code) return permPromptTrigger(name, parent, code or [[]]) end,
    }

    for fname, create in pairs(creators) do
      it(fname .. " raises and creates nothing when the parent does not exist", function()
        local name = "objSpecOrphan_" .. fname
        assert.equals(0, exists(name, "trigger"))
        local ok, err = pcall(create, name, "objSpecNoSuchTriggerGroup")
        assert.is_false(ok)
        assert.is_truthy(tostring(err):find(fname .. ": cannot create trigger (parent 'objSpecNoSuchTriggerGroup' not found)", 1, true),
          "got: " .. tostring(err))
        assert.equals(0, exists(name, "trigger"))
      end)

      it(fname .. " raises and creates nothing when its code does not compile", function()
        local name = "objSpecBadCode_" .. fname
        assert.equals(0, exists(name, "trigger"))
        local ok, err = pcall(create, name, "", "if then")
        assert.is_false(ok)
        assert.is_truthy(tostring(err):find("invalid Lua code", 1, true), "got: " .. tostring(err))
        assert.equals(0, exists(name, "trigger"))
      end)

      it(fname .. " rejects a non-string name or parent", function()
        -- a number is string-coercible, so a table is the wrong type here
        assert.has_error(function() create({}, "") end)
        assert.has_error(function() create("objSpecBadParent", {}) end)
      end)
    end

    it("the list-taking creators reject a non-table pattern list", function()
      for _, fname in ipairs({"permRegexTrigger", "permBeginOfLineStringTrigger", "permSubstringTrigger", "permExactMatchTrigger"}) do
        local ok, err = pcall(_G[fname], "objSpecNoList", "", "^not a table$", [[]])
        assert.is_false(ok, fname .. " accepted a string as its pattern list")
        assert.is_truthy(tostring(err):find("bad argument #3 type", 1, true), fname .. " gave: " .. tostring(err))
      end
    end)
  end)

  describe("other permanent item creators", function()
    it("permScript, permTimer and permKey reject a non-string name", function()
      assert.has_error(function() permScript({}, "", [[]]) end)
      assert.has_error(function() permTimer({}, "", 1, [[]]) end)
      assert.has_error(function() permKey({}, "", 65, [[]]) end)
    end)

    it("permScript and permTimer reject a non-string parent", function()
      assert.has_error(function() permScript("objSpecScriptBadParent", {}, [[]]) end)
      assert.has_error(function() permTimer("objSpecTimerBadParent", {}, 1, [[]]) end)
    end)

    it("setScript rejects a script position that is not a number", function()
      local ok, err = pcall(setScript, "objSpecAnyScript", [[]], "first")
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("script position", 1, true), "got: " .. tostring(err))
    end)
  end)

  describe("exists and isActive", function()
    it("exists finds a script by its ID and not an ID beyond it", function()
      -- scripts cannot be deleted from Lua, so the name is made unique per run
      local id = permScript(("objSpecExistsScript-%d-%d"):format(os.time(), math.random(100000)), "", "")
      assert.is_number(id)
      assert.equals(1, exists(id, "script"))
      assert.equals(0, exists(id + 100000, "script"))
    end)

    it("isActive rejects a non-boolean ancestor check flag", function()
      local ok, err = pcall(isActive, "objSpecAnyTrigger", "trigger", "yes")
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("also check ancestors", 1, true), "got: " .. tostring(err))
    end)
  end)

  describe("temporary trigger creators", function()
    local created = {}
    local function track(id)
      created[#created + 1] = id
      return id
    end

    before_each(function()
      _G.ObjSpec = {count = 0}
    end)

    after_each(function()
      for _, id in ipairs(created) do
        killTrigger(id)
      end
      created = {}
      _G.ObjSpec = nil
    end)

    it("tempBeginOfLineTrigger runs string code and stops after its expiration count", function()
      track(tempBeginOfLineTrigger("objSpecBolString", [[_G.ObjSpec.count = _G.ObjSpec.count + 1]], 2))
      feedTriggers("\nsomething objSpecBolString\n")
      assert.equals(0, _G.ObjSpec.count)
      for _ = 1, 3 do
        feedTriggers("\nobjSpecBolString here\n")
      end
      assert.equals(2, _G.ObjSpec.count)
    end)

    it("tempBeginOfLineTrigger rejects an expiration count below one", function()
      local id, err = tempBeginOfLineTrigger("objSpecBolZero", [[]], 0)
      if type(id) == "number" and id > 0 then track(id) end
      assert.is_nil(id)
      assert.is_truthy(tostring(err):find("greater than zero", 1, true), "got: " .. tostring(err))
    end)

    it("tempBeginOfLineTrigger rejects a non-string, non-function body", function()
      assert.has_error(function() tempBeginOfLineTrigger("objSpecBolBody", {}) end)
    end)

    it("tempExactMatchTrigger runs string code and stops after its expiration count", function()
      track(tempExactMatchTrigger("objSpecExactString", [[_G.ObjSpec.count = _G.ObjSpec.count + 1]], 1))
      feedTriggers("\nobjSpecExactString not alone\n")
      assert.equals(0, _G.ObjSpec.count)
      feedTriggers("\nobjSpecExactString\n")
      feedTriggers("\nobjSpecExactString\n")
      assert.equals(1, _G.ObjSpec.count)
    end)

    it("tempExactMatchTrigger validates its expiration count", function()
      local id, err = tempExactMatchTrigger("objSpecExactZero", [[]], 0)
      if type(id) == "number" and id > 0 then track(id) end
      assert.is_nil(id)
      assert.is_truthy(tostring(err):find("greater than zero", 1, true), "got: " .. tostring(err))
      assert.has_error(function() tempExactMatchTrigger("objSpecExactLater", [[]], "later") end)
    end)

    it("tempRegexTrigger validates its expiration count", function()
      local id, err = tempRegexTrigger("^objSpecRegexZero$", [[]], 0)
      if type(id) == "number" and id > 0 then track(id) end
      assert.is_nil(id)
      assert.is_truthy(tostring(err):find("greater than zero", 1, true), "got: " .. tostring(err))
      assert.has_error(function() tempRegexTrigger("^objSpecRegexLater$", [[]], "later") end)
    end)

    it("tempPromptTrigger accepts string code and validates its expiration count", function()
      local id = track(tempPromptTrigger([[_G.ObjSpec.count = _G.ObjSpec.count + 1]]))
      assert.is_number(id)
      assert.is_true(id > 0)
      assert.has_error(function() tempPromptTrigger([[]], "later") end)
    end)

    it("tempLineTrigger rejects a non-string, non-function body", function()
      assert.has_error(function() tempLineTrigger(1, 1, {}) end)
    end)

    it("tempComplexRegexTrigger validates its expiration count", function()
      local function create(expiry)
        return tempComplexRegexTrigger("objSpecComplexExpiry", "^objSpecComplex$", [[]], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, expiry)
      end
      local id, err = create(0)
      if type(id) == "number" and id > 0 then track(id) end
      assert.is_nil(id)
      assert.is_truthy(tostring(err):find("greater than zero", 1, true), "got: " .. tostring(err))
      assert.has_error(function() create("later") end)
    end)

    it("the pattern-taking creators reject a non-string pattern", function()
      for fname, call in pairs({
        tempAlias = function() return tempAlias({}, [[]]) end,
        tempBeginOfLineTrigger = function() return tempBeginOfLineTrigger({}, [[]]) end,
        tempExactMatchTrigger = function() return tempExactMatchTrigger({}, [[]]) end,
        tempRegexTrigger = function() return tempRegexTrigger({}, [[]]) end,
        tempComplexRegexTrigger = function() return tempComplexRegexTrigger("objSpecComplexBad", {}, [[]], 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) end,
      }) do
        local ok, err = pcall(call)
        assert.is_false(ok, fname .. " accepted a table")
        assert.is_truthy(tostring(err):find("bad argument #", 1, true), fname .. " gave: " .. tostring(err))
      end
    end)

    -- tempColorTrigger's legacy 1-16 numbering runs light/dark pairs from
    -- black upwards; each is checked here against the SGR background code of
    -- the ANSI colour it stands for, and against its pair, which must not match.
    local legacyBackgrounds = {
      {1, 100, 40}, {2, 40, 100}, {3, 101, 41}, {4, 41, 101},
      {5, 102, 42}, {6, 42, 102}, {7, 103, 43}, {8, 43, 103},
      {9, 104, 44}, {10, 44, 104}, {11, 105, 45}, {12, 45, 105},
      {13, 106, 46}, {14, 46, 106}, {15, 107, 47}, {16, 47, 107},
    }
    for _, entry in ipairs(legacyBackgrounds) do
      local legacy, matchingSgr, pairSgr = entry[1], entry[2], entry[3]
      it(("tempColorTrigger background %d matches SGR %d and not SGR %d"):format(legacy, matchingSgr, pairSgr), function()
        track(tempColorTrigger(-1, legacy, function() _G.ObjSpec.count = _G.ObjSpec.count + 1 end))
        feedTriggers(("\n\27[%dmobjSpecBgPair\27[0m\n"):format(pairSgr))
        assert.equals(0, _G.ObjSpec.count, "fired on its light/dark pair")
        feedTriggers(("\n\27[%dmobjSpecBgMatch\27[0m\n"):format(matchingSgr))
        assert.equals(1, _G.ObjSpec.count, "did not fire on its own colour")
      end)
    end

    it("tempColorTrigger background 0 matches the default background only", function()
      track(tempColorTrigger(-1, 0, function() _G.ObjSpec.count = _G.ObjSpec.count + 1 end))
      feedTriggers("\n\27[41mobjSpecBgRed\27[0m\n")
      assert.equals(0, _G.ObjSpec.count)
      feedTriggers("\n\27[0mobjSpecBgDefault\n")
      assert.equals(1, _G.ObjSpec.count)
    end)

    it("tempColorTrigger foreground 0 matches the default foreground only", function()
      track(tempColorTrigger(0, -1, function() _G.ObjSpec.count = _G.ObjSpec.count + 1 end))
      feedTriggers("\n\27[31mobjSpecFgRed\27[0m\n")
      assert.equals(0, _G.ObjSpec.count)
      feedTriggers("\n\27[0mobjSpecFgDefault\n")
      assert.equals(1, _G.ObjSpec.count)
    end)
  end)

  describe("tempButtonToolbar and tempButton refusals", function()
    local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
    local toolbarName = "objSpecToolbar" .. suffix
    local buttonName = "objSpecButton" .. suffix

    it("tempButton creates nothing on a toolbar that does not exist", function()
      local orphan = "objSpecOrphanButton" .. suffix
      assert.equals(0, select("#", tempButton("objSpecNoSuchToolbar" .. suffix, orphan, 0)))
      assert.equals(0, exists(orphan, "button"))
    end)

    it("tempButtonToolbar and tempButton return nothing for a name already in use", function()
      -- buttons cannot be deleted from Lua, hence the per-run names
      assert.is_number(tempButtonToolbar(toolbarName, 0, 0))
      assert.equals(0, select("#", tempButtonToolbar(toolbarName, 0, 0)))
      assert.equals(1, exists(toolbarName, "button"))

      assert.is_number(tempButton(toolbarName, buttonName, 0))
      assert.equals(0, select("#", tempButton(toolbarName, buttonName, 0)))
      assert.equals(1, exists(buttonName, "button"))
    end)

    it("reject non-string names", function()
      assert.has_error(function() tempButtonToolbar({}, 0, 0) end)
      assert.has_error(function() tempButton(toolbarName, {}, 0) end)
    end)
  end)

  describe("raiseEvent argument types", function()
    after_each(function()
      _G.ObjSpec = nil
    end)

    it("passes a function argument through to the handler as a callable", function()
      _G.ObjSpec = {}
      local handler = registerAnonymousEventHandler("objSpecFunctionEvent", function(_, fn)
        _G.ObjSpec.result = fn(20)
      end, true)
      raiseEvent("objSpecFunctionEvent", function(x) return x + 1 end)
      killAnonymousEventHandler(handler)
      assert.equals(21, _G.ObjSpec.result)
    end)

    it("rejects an argument of a type an event cannot carry", function()
      local ok, err = pcall(raiseEvent, "objSpecThreadEvent", coroutine.create(function() end))
      assert.is_false(ok)
      assert.is_truthy(tostring(err):find("got a thread", 1, true), "got: " .. tostring(err))
    end)
  end)

  describe("stopwatch argument handling", function()
    local created = {}
    local function track(id)
      created[#created + 1] = id
      return id
    end

    after_each(function()
      for _, id in ipairs(created) do
        pcall(deleteStopWatch, id)
      end
      created = {}
    end)

    it("each stopwatch function rejects a first argument that is neither number nor string", function()
      local calls = {
        adjustStopWatch = function() return adjustStopWatch({}, 1) end,
        deleteStopWatch = function() return deleteStopWatch({}) end,
        getStopWatchBrokenDownTime = function() return getStopWatchBrokenDownTime({}) end,
        resetStopWatch = function() return resetStopWatch({}) end,
        setStopWatchName = function() return setStopWatchName({}, "x") end,
        setStopWatchPersistence = function() return setStopWatchPersistence({}, false) end,
        startStopWatch = function() return startStopWatch({}) end,
        stopStopWatch = function() return stopStopWatch({}) end,
      }
      for fname, call in pairs(calls) do
        local ok, err = pcall(call)
        assert.is_false(ok, fname .. " accepted a table")
        assert.is_truthy(tostring(err):find(fname .. ": bad argument #1 type", 1, true), fname .. " gave: " .. tostring(err))
      end
    end)

    it("functions resolving a name report one that does not exist", function()
      local missing = "objSpecNoSuchStopWatch"
      for fname, call in pairs({
        adjustStopWatch = function() return adjustStopWatch(missing, 1) end,
        deleteStopWatch = function() return deleteStopWatch(missing) end,
        getStopWatchBrokenDownTime = function() return getStopWatchBrokenDownTime(missing) end,
        setStopWatchPersistence = function() return setStopWatchPersistence(missing, false) end,
      }) do
        local ok, err = call()
        assert.is_nil(ok, fname)
        assert.equals("stopwatch with name '" .. missing .. "' not found", err, fname)
      end
    end)

    it("getStopWatchTime of an empty name reports that there is no unnamed stopwatch", function()
      for id, watch in pairs(getStopWatches()) do
        assert.is_true(watch.name ~= "", "precondition: unnamed stopwatch " .. id .. " left over by another spec")
      end
      local ok, err = getStopWatchTime("")
      assert.is_nil(ok)
      assert.equals("no unnamed stopwatches found", err)
    end)

    it("getStopWatchTime of an empty name finds an unnamed stopwatch", function()
      local id = track(createStopWatch(false))
      adjustStopWatch(id, 3)
      assert.equals(3, getStopWatchTime(""))
    end)

    it("resetStopWatch resolves a stopwatch by name", function()
      local name = "objSpecResetByName"
      local id = track(createStopWatch(name))
      adjustStopWatch(id, 30)
      assert.equals(30, getStopWatchTime(id))
      assert.is_true(resetStopWatch(name))
      assert.equals(0, getStopWatchTime(id))
      local ok, err = resetStopWatch("objSpecNoSuchStopWatch")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("stopStopWatch reports an unknown numeric id", function()
      local ok, err = stopStopWatch(555555)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("setStopWatchName rejects a non-string new name", function()
      local id = track(createStopWatch(false))
      assert.has_error(function() setStopWatchName(id, {}) end)
    end)
  end)

  describe("command line names", function()
    after_each(function()
      clearCmdLine()
    end)

    it("an empty command line name addresses the main command line", function()
      clearCmdLine()
      assert.equals("", getCmdLine())
      printCmdLine("", "objSpecMainCmdLine")
      assert.equals("objSpecMainCmdLine", getCmdLine())
    end)
  end)
end)
