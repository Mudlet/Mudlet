-- The script editor reads code in the display font until it is given one of its
-- own: editorFontFollowsDisplay says which, and editorFont and editorFontSize
-- are the family and size it reads code in either way. Asking for a family or a
-- size other than the one in use is asking for a font of the editor's own.
describe("Tests the script editor's own font", function()
  -- both are bundled with Mudlet
  local families = {"Ubuntu Mono", "Bitstream Vera Sans Mono"}

  local function otherFamily(current)
    return current == families[1] and families[2] or families[1]
  end

  -- every key moved here goes back as it was, and so does the main console's
  -- size, which one case moves to watch a following editor go with it
  local saved
  before_each(function()
    saved = {
      follows = getConfig("editorFontFollowsDisplay"),
      font = getConfig("editorFont"),
      size = getConfig("editorFontSize"),
      mainSize = getFontSize("main"),
    }
  end)
  after_each(function()
    if getFontSize("main") ~= saved.mainSize then
      setFontSize("main", saved.mainSize)
    end
    if saved.follows then
      setConfig("editorFontFollowsDisplay", true)
    else
      setConfig("editorFontFollowsDisplay", false)
      setConfig("editorFont", saved.font)
      setConfig("editorFontSize", saved.size)
    end
  end)

  -- the arguments of every sysSettingChanged event for the editor's font, and
  -- for the switch, raised while fn runs
  local function editorFontEvents(fn)
    local seen = {}
    local id = registerAnonymousEventHandler("sysSettingChanged", function(_, key, first, second)
      if key == "editor font" or key == "editorFontFollowsDisplay" then
        seen[#seen + 1] = {key, first, second}
      end
    end)
    local ok, err = pcall(fn)
    killAnonymousEventHandler(id)
    assert(ok, err)
    return seen
  end

  it("reads code in the display font in a profile that never chose another", function()
    assert.is_true(setConfig("editorFontFollowsDisplay", true))
    assert.is_true(getConfig("editorFontFollowsDisplay"))
    assert.equals(getFont("main"), getConfig("editorFont"))
    assert.equals(getFontSize("main"), getConfig("editorFontSize"))
    local config = getConfig()
    assert.equals(getConfig("editorFont"), config.editorFont)
    assert.equals(getConfig("editorFontSize"), config.editorFontSize)
    assert.is_true(config.editorFontFollowsDisplay)
  end)

  it("takes a family of its own and stops following the display font", function()
    setConfig("editorFontFollowsDisplay", true)
    local mainFont, mainSize = getFont("main"), getFontSize("main")
    local wanted = otherFamily(mainFont)

    assert.is_true(setConfig("editorFont", wanted))
    assert.equals(wanted, getConfig("editorFont"))
    assert.is_false(getConfig("editorFontFollowsDisplay"))
    -- the size it starts from is the one that was on show
    assert.equals(mainSize, getConfig("editorFontSize"))
    -- ...and the display font is where it was
    assert.equals(mainFont, getFont("main"))

    -- matched case-insensitively, read back as the font database spells it
    assert.is_true(setConfig("editorFont", otherFamily(wanted):upper()))
    assert.equals(otherFamily(wanted), getConfig("editorFont"))
  end)

  it("takes a size of its own and stops following the display font", function()
    setConfig("editorFontFollowsDisplay", true)
    local mainFont, mainSize = getFont("main"), getFontSize("main")
    local wanted = mainSize + 5

    assert.is_true(setConfig("editorFontSize", wanted))
    assert.equals(wanted, getConfig("editorFontSize"))
    assert.equals(mainFont, getConfig("editorFont"))
    assert.is_false(getConfig("editorFontFollowsDisplay"))
    assert.equals(mainSize, getFontSize("main"))
  end)

  it("keeps following when handed back the font it already reads code in", function()
    setConfig("editorFontFollowsDisplay", true)
    assert.is_true(setConfig("editorFont", getConfig("editorFont")))
    assert.is_true(setConfig("editorFontSize", getConfig("editorFontSize")))
    assert.is_true(getConfig("editorFontFollowsDisplay"))
    -- the table form applies its keys in no set order, which must not matter either
    local config = getConfig({"editorFont", "editorFontSize", "editorFontFollowsDisplay"})
    assert.is_true(setConfig(config))
    assert.is_true(getConfig("editorFontFollowsDisplay"))
  end)

  it("goes back to the display font when following again", function()
    assert.is_true(setConfig("editorFont", otherFamily(getFont("main"))))
    assert.is_true(setConfig("editorFontSize", getFontSize("main") + 3))
    assert.is_true(setConfig("editorFontFollowsDisplay", true))
    assert.equals(getFont("main"), getConfig("editorFont"))
    assert.equals(getFontSize("main"), getConfig("editorFontSize"))
  end)

  it("refuses a family that is not there, or none at all", function()
    setConfig("editorFont", otherFamily(getFont("main")))
    local before = getConfig("editorFont")

    local ok, err = setConfig("editorFont", "No Such Font At All")
    assert.is_nil(ok)
    assert.equals("font 'No Such Font At All' is not available", err)
    ok, err = setConfig("editorFont", "   ")
    assert.is_nil(ok)
    assert.equals("editorFont must not be empty", err)
    assert.equals(before, getConfig("editorFont"))
  end)

  it("refuses a size outside 1 to 1000", function()
    setConfig("editorFontSize", 12)
    for _, size in ipairs({0, -3, 1001}) do
      local ok, err = setConfig("editorFontSize", size)
      assert.is_nil(ok, "setConfig accepted the size " .. size)
      assert.equals(("editorFontSize %d is out of range, it must be between 1 and 1000"):format(size), err)
    end
    assert.equals(12, getConfig("editorFontSize"))
    -- both ends are sizes
    assert.is_true(setConfig("editorFontSize", 1))
    assert.equals(1, getConfig("editorFontSize"))
    assert.is_true(setConfig("editorFontSize", 1000))
    assert.equals(1000, getConfig("editorFontSize"))
  end)

  it("says what moved with sysSettingChanged", function()
    setConfig("editorFontFollowsDisplay", true)
    local mainFont, mainSize = getFont("main"), getFontSize("main")
    local wanted = otherFamily(mainFont)

    local seen = editorFontEvents(function()
      setConfig("editorFont", wanted)
    end)
    assert.same({{"editorFontFollowsDisplay", false}, {"editor font", wanted, mainSize}}, seen)

    seen = editorFontEvents(function()
      setConfig("editorFontSize", mainSize + 2)
    end)
    assert.same({{"editor font", wanted, mainSize + 2}}, seen)

    -- a display font moving is no change to an editor with a font of its own
    seen = editorFontEvents(function()
      setFontSize("main", mainSize + 1)
    end)
    assert.same({}, seen)
    setFontSize("main", mainSize)

    seen = editorFontEvents(function()
      setConfig("editorFontFollowsDisplay", true)
    end)
    assert.same({{"editorFontFollowsDisplay", true}, {"editor font", mainFont, mainSize}}, seen)

    -- ...and is one to an editor following it
    seen = editorFontEvents(function()
      setFontSize("main", mainSize + 1)
    end)
    assert.same({{"editor font", mainFont, mainSize + 1}}, seen)
  end)
end)
