-- The cheaper half of the addon command API: what a request has to look like
-- to be placed at all, what every binding answers for an id it does not know,
-- and what the pulse refuses. Whether any of it reached a widget needs the
-- widget read back, so that, the click, and anything wanting a second profile
-- are in test/functional_tests/AddonControlsTest.cpp instead.
--
-- Refusals are translated, so an assertion matches only the parts of a message
-- that are not: a field name, a Lua type name, or the surface names a package
-- writes in its own code.

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

    -- Leaving a field out and giving it the wrong type are different mistakes.
    -- A menu path is conceptually a list and the sibling surfaces field really
    -- does take one, so this is an easy thing to write - and it used to place
    -- the command at the top of Extensions without a word.
    it("refuses a field of the wrong type rather than dropping it", function()
      local id, why = addCommand{name = "WrongTypeSpec", menuPath = {"Speech", "Voices"}}
      assert.is_nil(id, "the menu path was the wrong type and was dropped without a word")
      assert.is_truthy(why:find("menuPath", 1, true), "the refusal does not say which field: " .. tostring(why))
      assert.is_truthy(why:find("table", 1, true), "the refusal does not say what it found: " .. tostring(why))
    end)

    -- Lua 5.1 answers "yes" when asked whether a number is a string, so this is
    -- not a type mistake and has to reach the sequence parser to be refused for
    -- what it actually is
    it("leaves a number to whatever reads the field", function()
      local id, why = addCommand{name = "NumberFieldSpec", shortcut = 12345}
      assert.is_nil(id)
      assert.is_falsy(why:find("number", 1, true), "the number was refused as a type mistake: " .. tostring(why))
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
      -- their shortcut and guessing at one. The name is translated, so what
      -- can be checked here is that one was quoted at all
      assert.is_truthy(why:find('"', 1, true), "the refusal does not say what holds the key: " .. tostring(why))
    end)

    -- The other arm of the same question: this key does hang on a menu action,
    -- so it is found by the scan of those rather than by asking what Mudlet
    -- reserves. Ctrl+Alt+L is spelt the same way on every platform, which the
    -- profile tab keys are not - macOS puts those on the physical Ctrl key,
    -- while the Ctrl in a package's string is Cmd there.
    it("refuses one Mudlet uses from its own menu", function()
      local id, why = addCommand{name = "MenuKeySpec", shortcut = "Ctrl+Alt+L"}
      assert.is_nil(id, "Ctrl+Alt+L toggles logging and was handed out anyway")
      assert.is_truthy(why:find('"', 1, true), "the refusal does not say what holds the key: " .. tostring(why))
    end)

    -- Not every QShortcut on the window is a key the player can reach: a
    -- widget-context one on a widget that takes no focus never fires, and every
    -- console builds one for Ctrl+W that is connected to nothing. Counting
    -- those refused a free key to every package.
    it("hands out a key held only by a shortcut that could never fire", function()
      if getOS() == "mac" then
        -- Ctrl in a key sequence is Cmd there, and Cmd+W closes the profile
        return
      end
      local id, why = addCommand{name = "DeadShortcutSpec", shortcut = "Ctrl+W"}
      assert.is_number(id, "nothing on this platform uses Ctrl+W, yet: " .. tostring(why))
      removeCommand(id)
    end)

    -- Switching the buffer search off deletes its shortcuts through the event
    -- loop, so for the rest of the turn they were still on the window and F3
    -- was refused to a package that could by then have it
    it("hands out a key as soon as whatever held it is switched off", function()
      setConfig("f3SearchEnabled", true)
      local held = addCommand{name = "F3HeldSpec", shortcut = "F3"}
      if held then
        removeCommand(held)
      end
      assert.is_nil(held, "F3 is the buffer search's key while the search is on")

      setConfig("f3SearchEnabled", false)
      local id, why = addCommand{name = "F3FreeSpec", shortcut = "F3"}
      assert.is_number(id, "F3 was still refused after the search was switched off: " .. tostring(why))
      removeCommand(id)
    end)

    -- Qt keeps the first four chunks of a longer sequence and drops the rest,
    -- so the command went onto a key nobody had asked for
    it("refuses one of more steps than Qt can hold", function()
      local id, why = addCommand{name = "FiveStepSpec", shortcut = "Ctrl+Alt+F1, Ctrl+Alt+F2, Ctrl+Alt+F3, Ctrl+Alt+F4, Ctrl+Alt+F5"}
      assert.is_nil(id, "the fifth step was dropped and the rest handed out anyway")
      assert.is_string(why)
    end)

    it("takes one of exactly as many steps as Qt can hold", function()
      assert.is_number(place{name = "FourStepSpec", shortcut = "Ctrl+Alt+F5, Ctrl+Alt+F6, Ctrl+Alt+F7, Ctrl+Alt+F8"})
    end)

    -- The comma separates the steps and is also a key in its own right, so
    -- counting them naively reads "Ctrl+," as two steps of nothing
    it("takes the comma key itself", function()
      assert.is_number(place{name = "CommaKeySpec", shortcut = "Ctrl+,"})
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
      assert.is_truthy(why:find("toolbar", 1, true))
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
      assert.is_truthy(why:find("toolbar", 1, true))
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
