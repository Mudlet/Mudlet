-- Every one of these finds its console by name: an empty name or "main" is the
-- main console, and any other is a mini console, user window or buffer. Every
-- kind of window shares the one name space, so a name that belongs to some
-- other kind must not be taken for a console. getFont and setFont also reach a
-- label, but only once no console answers to the name.
describe("Tests that the font functions find their console by name", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local unknown = "specFontsByNameNoSuchWindow" .. suffix
  -- both are bundled with Mudlet
  local families = {"Ubuntu Mono", "Bitstream Vera Sans Mono"}

  local function otherFamily(current)
    return current == families[1] and families[2] or families[1]
  end

  -- every spec after this one lays itself out against the main console's font,
  -- so the family and size on show go back as they were even if a case fails
  -- before its own finally() has restored them. Setting the main console's
  -- family counts as the user choosing it, so in a profile whose own family is
  -- not installed, the stand-in shown for it is what the profile saves after this.
  local mainFont, mainFontSize
  setup(function()
    mainFont, mainFontSize = getFont("main"), getFontSize("main")
  end)
  teardown(function()
    if getFont("main") ~= mainFont then
      setFont("main", mainFont)
    end
    if getFontSize("main") ~= mainFontSize then
      setFontSize("main", mainFontSize)
    end
  end)

  -- an array rather than a keyed table so the specs are always generated in
  -- the same order
  local consoleOnly = {
    {"getFontSize", function(n) return getFontSize(n) end},
    {"setFontSize", function(n) return setFontSize(n, 10) end},
  }
  local labelsToo = {
    {"getFont", function(n) return getFont(n) end},
    {"setFont", function(n) return setFont(n, families[1]) end},
  }

  local function assertRefuses(windowName, calls)
    local notFound = ('window "%s" not found'):format(windowName)
    for _, entry in ipairs(calls) do
      local functionName, call = entry[1], entry[2]
      assert.are.equal(2, select("#", call(windowName)), functionName)
      local ok, err = call(windowName)
      assert.is_nil(ok, functionName)
      assert.are.equal(notFound, err, functionName)
    end
  end

  -- the arguments of every sysSettingChanged event raised for the main
  -- console's font while fn runs
  local function mainFontEvents(fn)
    local seen = {}
    local id = registerAnonymousEventHandler("sysSettingChanged", function(_, key, family, size)
      if key == "main window font" then
        seen[#seen + 1] = {family, size}
      end
    end)
    local ok, err = pcall(fn)
    killAnonymousEventHandler(id)
    assert(ok, err)
    return seen
  end

  it("refuses a name that is no window at all", function()
    assertRefuses(unknown, consoleOnly)
    assertRefuses(unknown, labelsToo)
  end)

  describe("with the name of another kind of window", function()
    local otherName = "specFontsByNameOtherWindow" .. suffix

    after_each(function()
      deleteLabel(otherName)
      deleteScrollBox(otherName)
      deleteTextEdit(otherName)
      deleteCommandLine(otherName)
    end)

    local kinds = {
      {"scroll box", function() return createScrollBox(otherName, 0, 0, 50, 20) end},
      {"text edit", function() return createTextEdit("main", otherName, 0, 0, 50, 20) end},
      {"command line", function() return createCommandLine(otherName, 0, 0, 50, 20) end},
    }

    for _, kind in ipairs(kinds) do
      it("refuses every font function for a " .. kind[1], function()
        assert.is_true(kind[2]())
        assertRefuses(otherName, consoleOnly)
        assertRefuses(otherName, labelsToo)
      end)
    end

    it("refuses the size of a label, but reaches its font", function()
      assert.is_true(createLabel(otherName, 0, 0, 50, 20, 1))
      assertRefuses(otherName, consoleOnly)
      assert.is_true(setFont(otherName, families[1]))
      assert.are.equal(families[1], getFont(otherName))
    end)
  end)

  it("checks the arguments before it looks for the console", function()
    for _, call in ipairs({
      function() getFont({}) end,
      function() getFontSize({}) end,
      function() setFont({}, families[1]) end,
      function() setFont(unknown, {}) end,
      function() setFontSize({}, 10) end,
      function() setFontSize(unknown, "big") end,
    }) do
      assert.has_error(call)
    end

    assert.are.same({nil, "font must not be empty"}, {setFont(unknown, "  ")})
    local missing = "specFontsByNameNoSuchFamily" .. suffix
    assert.are.same({nil, ("font '%s' is not available"):format(missing)}, {setFont(unknown, missing)})
    -- a name is one family, not the comma separated, quoted list Qt reads a
    -- font string as, where these name no family or an installed one
    for _, name in ipairs({missing .. ",", ",", "'", "''", missing .. ", " .. families[1]}) do
      assert.are.same({nil, ("font '%s' is not available"):format(name)}, {setFont(unknown, name)})
    end
    assert.are.same({nil, "size cannot be 0 or negative"}, {setFontSize(unknown, 0)})
    -- far enough up, Qt gives up on the font and getFont() reports no family
    assert.are.same({nil, "size 999999 is too large, it cannot be more than 1000"}, {setFontSize(unknown, 999999)})
    assert.are.same({nil, "size 1001 is too large, it cannot be more than 1000"}, {setFontSize(unknown, 1001)})
  end)

  describe("with a sub-console's name", function()
    local kinds = {
      {"mini console", function(name) return createMiniConsole(name, 0, 0, 300, 200) end, deleteMiniConsole},
      -- once a docked user window has been deleted, every one docked after it
      -- opens with no height, so this one is floated at a fixed size, and only
      -- closed and left behind so it cannot do the same to later specs
      {"user window", function(name)
        local opened = openUserWindow(name, false, false)
        resizeWindow(name, 300, 400)
        return opened
      end, closeUserWindow},
      -- a buffer cannot be deleted, so it is left behind under its unique name
      {"buffer", function(name) createBuffer(name) return true end, function() end},
    }

    for _, kind in ipairs(kinds) do
      local window = "specFontsByName " .. kind[1] .. suffix

      describe("of a " .. kind[1], function()
        setup(function()
          assert.is_true(kind[2](window))
        end)

        teardown(function()
          kind[3](window)
        end)

        it("sets and reads that console's font size", function()
          local mainSize = getFontSize("main")
          local size = getFontSize(window) == 13 and 14 or 13

          local events = mainFontEvents(function()
            assert.are.same({true}, {setFontSize(window, size)})
          end)
          assert.are.equal(size, getFontSize(window))
          assert.are.same({}, events)
          assert.are.equal(mainSize, getFontSize("main"))
        end)

        it("takes the largest size it allows and refuses one past it", function()
          local family = getFont(window)
          assert.are.same({true}, {setFontSize(window, 1000)})
          assert.are.equal(1000, getFontSize(window))
          assert.are.equal(family, getFont(window))
          assert.are.same({nil, "size 1001 is too large, it cannot be more than 1000"}, {setFontSize(window, 1001)})
          assert.are.equal(1000, getFontSize(window))
          setFontSize(window, 13)
        end)

        it("sets and reads that console's font, keeping its size", function()
          local mainFont = getFont("main")
          local size = getFontSize(window)
          local family = otherFamily(getFont(window))

          local events = mainFontEvents(function()
            assert.are.same({true}, {setFont(window, family)})
          end)
          assert.are.equal(family, getFont(window))
          assert.are.equal(size, getFontSize(window))
          assert.are.same({}, events)
          assert.are.equal(mainFont, getFont("main"))

          -- a style is taken as the weight, so the family stays the base one
          family = otherFamily(family)
          assert.is_true(setFont(window, family .. " Bold"))
          assert.are.equal(family, getFont(window))
        end)
      end)
    end

    for _, kind in ipairs({kinds[1], kinds[2]}) do
      local window = "specFontsByName command line of a " .. kind[1] .. suffix

      it("gives a " .. kind[1] .. " a command line of its own and hides it again", function()
        assert.is_true(kind[2](window))
        finally(function()
          disableCommandLine(window)
          kind[3](window)
        end)
        pumpEvents(100)
        local rows = getRowCount(window)
        assert.are.same({nil, ('command line "%s" not found'):format(window)}, {getCmdLine(window)})

        assert.are.same({true}, {enableCommandLine(window)})
        pumpEvents(100)
        assert.are.equal("", getCmdLine(window))
        assert.is_true(getRowCount(window) < rows)

        assert.are.same({true}, {disableCommandLine(window)})
        pumpEvents(100)
        assert.are.equal(rows, getRowCount(window))
      end)
    end
  end)

  describe("with the main console", function()
    it("takes an empty name, \"main\" or none for the main console", function()
      local font, size = getFont("main"), getFontSize("main")
      assert.is_string(font)
      assert.is_number(size)
      assert.are.equal(font, getFont(""))
      assert.are.equal(font, getFont())
      assert.are.equal(size, getFontSize(""))
      assert.are.equal(size, getFontSize())
    end)

    for _, name in ipairs({"", "main"}) do
      it(("sets the profile's font size by %q"):format(name), function()
        local original = getFontSize("main")
        local size = original == 13 and 14 or 13
        finally(function()
          setFontSize("main", original)
        end)

        local events = mainFontEvents(function()
          assert.are.same({true}, {setFontSize(name, size)})
        end)
        assert.are.equal(size, getFontSize("main"))
        assert.are.equal(1, #events)
        assert.are.equal(size, events[1][2])
      end)

      it(("sets the profile's font by %q, keeping its size"):format(name), function()
        local original = getFont("main")
        local size = getFontSize("main")
        local family = otherFamily(original)
        finally(function()
          setFont("main", original)
        end)

        local events = mainFontEvents(function()
          assert.are.same({true}, {setFont(name, family)})
        end)
        assert.are.equal(family, getFont("main"))
        assert.are.equal(size, getFontSize("main"))
        assert.are.same({{family, size}}, events)
      end)
    end

    it("takes no name for the main console", function()
      local originalFont, originalSize = getFont("main"), getFontSize("main")
      local family = otherFamily(originalFont)
      local size = originalSize == 13 and 14 or 13
      finally(function()
        setFont("main", originalFont)
        setFontSize("main", originalSize)
      end)

      assert.are.same({true}, {setFontSize(size)})
      assert.are.equal(size, getFontSize("main"))
      assert.are.same({true}, {setFont(family)})
      assert.are.equal(family, getFont("main"))
    end)

    it("refuses the main console's command line", function()
      local refusal = {nil, "this function is not permitted on the main command line"}
      assert.are.same(refusal, {enableCommandLine("main")})
      assert.are.same(refusal, {disableCommandLine("")})
    end)
  end)
end)
