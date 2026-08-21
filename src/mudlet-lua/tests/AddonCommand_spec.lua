-- The cheaper half of the addon command API: what every binding answers for an
-- id it does not know, what the pulse refuses, and that a command's surfaces
-- stay in step. The parts needing a real click, a real event loop turn or a
-- second profile are in test/functional_tests/AddonControlsTest.cpp instead.

describe("addon commands", function()
  local placed = {}

  local function place(fields)
    local id = addCommand(fields)
    if id then placed[#placed + 1] = id end
    return id
  end

  after_each(function()
    for _, id in ipairs(placed) do removeCommand(id) end
    placed = {}
  end)

  describe("placing one", function()
    it("hands back a number, which is what the click event carries", function()
      local id = place{name = "SpecCommand", menuPath = "Spec"}
      assert.is_number(id)
      assert.is_true(id > 0)
    end)

    it("refuses a command with no name to show", function()
      local id, why = addCommand{menuPath = "Spec"}
      assert.is_nil(id)
      assert.is_string(why)
    end)

    it("refuses a surface this client does not have", function()
      local id, why = addCommand{name = "Nowhere", surfaces = "hologram"}
      assert.is_nil(id)
      assert.is_string(why)
    end)

    it("takes a single surface as a bare string", function()
      assert.is_number(place{name = "MenuOnly", surfaces = "menu"})
    end)

    it("takes several surfaces as a list", function()
      assert.is_number(place{name = "BothOfThem", surfaces = {"menu", "toolbar"}})
    end)

    it("gives each command an id of its own", function()
      local first = place{name = "FirstSpec", menuPath = "Spec"}
      local second = place{name = "SecondSpec", menuPath = "Spec"}
      assert.is_true(first ~= second)
    end)
  end)

  describe("an id nobody knows", function()
    -- One sequence covers every command, so an unknown id names nothing at all
    -- rather than something of another kind
    local unknown = 999999

    it("is refused by every setter rather than erroring", function()
      assert.is_false(removeCommand(unknown))
      assert.is_false(enableCommand(unknown))
      assert.is_false(disableCommand(unknown))
      assert.is_false(setCommandChecked(unknown, true))
      assert.is_false(setCommandIcon(unknown, ""))
      assert.is_false(setCommandTooltip(unknown, "nothing"))
    end)

    it("is what an id becomes once its command is removed", function()
      local id = addCommand{name = "Fleeting", menuPath = "Spec"}
      assert.is_true(removeCommand(id))
      assert.is_false(removeCommand(id))
      assert.is_false(enableCommand(id))
    end)
  end)

  describe("changing one", function()
    local id

    before_each(function()
      id = place{name = "Mutable", menuPath = "Spec", tooltip = "before"}
    end)

    it("enables and disables through a function each, not a boolean argument", function()
      assert.is_true(disableCommand(id))
      assert.is_true(enableCommand(id))
    end)

    it("takes a checkmark", function()
      assert.is_true(setCommandChecked(id, true))
      assert.is_true(setCommandChecked(id, false))
    end)

    it("takes a new tooltip, markup characters and all", function()
      assert.is_true(setCommandTooltip(id, [[Damage 5 <10 & rising <b>now</b>]]))
    end)

    it("takes a new icon, including one that resolves to nothing", function()
      assert.is_true(setCommandIcon(id, ""))
      assert.is_true(setCommandIcon(id, "/no/such/icon.png"))
    end)
  end)

  describe("the pulse", function()
    it("runs on a command that has a button", function()
      local id = place{name = "Pulsing", menuPath = "Spec"}
      assert.is_true(setCommandPulse(id, true, "#22aa44", "#116622", 700))
      assert.is_true(setCommandPulse(id, false))
    end)

    it("refuses a colour Qt cannot parse, rather than painting the button black", function()
      local id = place{name = "BadColour", menuPath = "Spec"}
      local ok, why = setCommandPulse(id, true, "reed", "bluu", 400)
      assert.is_nil(ok)
      assert.is_string(why)
    end)

    it("refuses a colour carrying its own stylesheet", function()
      local id = place{name = "Injected", menuPath = "Spec"}
      local ok = setCommandPulse(id, true, "red; background-image: url(x)", "#cc0000", 400)
      assert.is_nil(ok)
    end)

    it("refuses an interval that would restyle on every pass of the event loop", function()
      local id = place{name = "TooFast", menuPath = "Spec"}
      local ok = setCommandPulse(id, true, "#22aa44", "#116622", 0)
      assert.is_nil(ok)
    end)

    it("refuses a command that has no button to colour", function()
      local id = place{name = "MenuOnlyPulse", surfaces = "menu"}
      local ok, why = setCommandPulse(id, true)
      assert.is_nil(ok)
      assert.is_string(why)
    end)
  end)
end)
