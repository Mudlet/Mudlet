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

    -- removeCommand defers the menu action's destruction past the event loop
    -- turn, so it was still a child of the main window holding its shortcut.
    -- Remove-then-re-add in one pass - a package reload, or changing a
    -- command's key - was then refused in the name of a command that had just
    -- been removed and could not be found by anything the package could call.
    it("frees a shortcut as soon as the command is removed, not a turn later", function()
      local first = addCommand{name = "SpecShortcut", menuPath = "Spec", shortcut = "Ctrl+Alt+F9"}
      assert.is_number(first)
      assert.is_true(removeCommand(first))

      local second, why = place{name = "SpecShortcutAgain", menuPath = "Spec", shortcut = "Ctrl+Alt+F9"}
      assert.is_number(second, "the shortcut was still held by the removed command: " .. tostring(why))
    end)

    it("gives each command an id of its own", function()
      local first = place{name = "FirstSpec", menuPath = "Spec"}
      local second = place{name = "SecondSpec", menuPath = "Spec"}
      assert.is_true(first ~= second)
    end)
  end)

  describe("a shortcut it cannot honour", function()
    -- Mudlet's own sequences do not all sit on menu actions: the profile
    -- switching keys never do, and hiding the menu bar moves every other one
    -- onto a plain QShortcut as well. Answering from what is currently wired
    -- up therefore handed Ctrl+1 to a package and left the player with two
    -- things on one key, which Qt resolves by disabling both.
    it("refuses one Mudlet uses for switching profiles, and says what holds it", function()
      local id, why = addCommand{name = "ProfileKeySpec", shortcut = "Ctrl+1"}
      assert.is_nil(id, "Ctrl+1 switches to the first profile and was handed out anyway")
      -- naming the holder is the difference between a package author fixing
      -- their shortcut and guessing at one
      assert.is_truthy(why:find("profile", 1, true), "the refusal does not say what holds the key: " .. tostring(why))
    end)

    it("refuses one Mudlet uses for the next profile", function()
      local id, why = addCommand{name = "NextProfileKeySpec", shortcut = "Ctrl+Tab"}
      assert.is_nil(id, "Ctrl+Tab moves to the next profile and was handed out anyway")
      assert.is_truthy(why:find("profile", 1, true), "the refusal does not say what holds the key: " .. tostring(why))
    end)

    -- Qt parses an unreadable chunk into Key_unknown rather than dropping it,
    -- so a typo in any chunk but the first passed the emptiness test, showed a
    -- half-written sequence in the menu and never fired
    it("refuses one whose second chunk is a typo", function()
      local id, why = addCommand{name = "TypoKeySpec", shortcut = "Ctrl+Alt+F10, Ctrl+Shft+B"}
      assert.is_nil(id, "the sequence holds an unparseable chunk and was accepted anyway")
      assert.is_string(why)
    end)

    it("refuses one on a command kept off the menu, whatever the bars are doing", function()
      local id, why = addCommand{name = "ToolbarKeySpec", surfaces = "toolbar", shortcut = "Ctrl+Alt+F11"}
      assert.is_nil(id)
      assert.is_truthy(why:find("shortcut", 1, true))
    end)
  end)

  describe("surfaces", function()
    -- Each of these used to fall through to "both", so a package naming
    -- nothing usable got its command placed everywhere instead of an answer
    it("refuses an empty list, which asks for the command to go nowhere", function()
      local id, why = addCommand{name = "NowhereSpec", surfaces = {}}
      assert.is_nil(id)
      assert.is_string(why)
    end)

    it("refuses a value that is neither a name nor a list", function()
      local id, why = addCommand{name = "BooleanSurfaceSpec", surfaces = true}
      assert.is_nil(id)
      assert.is_string(why)
    end)

    -- The list holds names, so a keyed table quotes the type it found: the
    -- message used to quote the empty string and send packages looking for a
    -- surface with no name
    it("refuses a keyed table and says what it found", function()
      local id, why = addCommand{name = "KeyedSurfaceSpec", surfaces = {menu = true, toolbar = true}}
      assert.is_nil(id)
      assert.is_truthy(why:find("boolean", 1, true))
    end)

    it("refuses a menu path on a command kept off the menu", function()
      local id, why = addCommand{name = "PathlessSpec", surfaces = "toolbar", menuPath = "Spec"}
      assert.is_nil(id, "the menu path had nowhere to go and was dropped without a word")
      assert.is_truthy(why:find("menu path", 1, true))
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
