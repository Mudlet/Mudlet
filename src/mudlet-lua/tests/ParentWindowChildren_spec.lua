-- The create...() calls take the parent window's name first and the new
-- element's name second. A parent window name that matches nothing is refused:
-- it used to be replaced by the main console without a word, so the element was
-- painted over the game text under the name the caller passed second - not the
-- one it meant - while the call answered "true". createLabel() always refused.
--
-- windowVisible() is the oracle for "did the element go where it was asked
-- for": reading an element back answers the same wherever it landed, while
-- hiding its parent window only hides what is really inside it. UI_spec.lua
-- uses the same idiom for labels in a user window.

describe("Tests elements created into a named parent window", function()
  -- a user window is a mini console with a dock, so windowType() tells the two
  -- apart while deleteMiniConsole() removes either
  local deleters = {
    userwindow = deleteMiniConsole,
    miniconsole = deleteMiniConsole,
    scrollbox = deleteScrollBox,
    commandline = deleteCommandLine,
    textedit = deleteTextEdit,
    label = deleteLabel,
  }

  local created = {}

  local function track(name)
    created[#created + 1] = name
    return name
  end

  after_each(function()
    -- newest first, so an element goes before the window it was created into;
    -- a name that was refused, or that its parent's deletion already took, is
    -- not a window type any more and is skipped
    for index = #created, 1, -1 do
      local name = created[index]
      local delete = deleters[windowType(name)]
      if delete then
        delete(name)
      end
    end
    created = {}
  end)

  describe("a parent window name that matches nothing", function()
    it("refuses to create a scroll box rather than putting it on the main console", function()
      local ok, message = createScrollBox("pwcNoSuchWindow", track("pwcStrayBox"), 0, 0, 120, 80)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_nil(windowType("pwcStrayBox"))
    end)

    it("refuses to create a mini console rather than putting it on the main console", function()
      local ok, message = createMiniConsole("pwcNoSuchWindow", track("pwcStrayConsole"), 0, 0, 120, 80)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_nil(windowType("pwcStrayConsole"))
    end)

    it("refuses to create a command line rather than putting it on the main console", function()
      -- createCommandLine and createTextEdit report a refusal as nil where the
      -- other three use false; the shapes are inconsistent, and these examples
      -- pin what each one does today rather than what it ought to do
      local ok, message = createCommandLine("pwcNoSuchWindow", track("pwcStrayCmdLine"), 0, 0, 120, 30)

      assert.is_nil(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_nil(windowType("pwcStrayCmdLine"))
    end)

    it("refuses to create a text edit rather than putting it on the main console", function()
      local ok, message = createTextEdit("pwcNoSuchWindow", track("pwcStrayTextEdit"), 0, 0, 120, 80)

      assert.is_nil(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_nil(windowType("pwcStrayTextEdit"))
    end)

    it("refuses a label the same way, as it always has", function()
      local ok, message = createLabel("pwcNoSuchWindow", track("pwcStrayLabel"), 0, 0, 120, 80, true)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_nil(windowType("pwcStrayLabel"))
    end)

    it("refuses to create a map, which can only go in a user window", function()
      local ok, message = createMapper("pwcNoSuchWindow", 0, 0, 300, 300)

      assert.is_nil(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
    end)

    it("refuses when the parent and element names are given the wrong way round", function()
      assert.is_true(openUserWindow(track("pwcSwapped"), false))

      -- createScrollBox(parentWindowName, scrollBoxName, ...): passing the
      -- scroll box's name first names a window that does not exist
      local ok, message = createScrollBox("pwcSwappedBox", "pwcSwapped", 0, 0, 220, 140)

      assert.is_false(ok)
      assert.are.equal("window 'pwcSwappedBox' not found", message)
      assert.is_nil(windowType("pwcSwappedBox"))
      -- windowType() answers "userwindow" for that name either way, because a
      -- user window is resolved before a scroll box of the same name; asking
      -- the scroll box map is what says no scroll box was created under it
      local deleted = deleteScrollBox("pwcSwapped")
      assert.is_false(deleted)
      assert.are.equal("userwindow", windowType("pwcSwapped"))
    end)

    it("refuses a parent window that existed and has since been deleted", function()
      assert.is_true(openUserWindow("pwcBuried", false))
      assert.is_true(deleteMiniConsole("pwcBuried"))
      assert.is_nil(windowType("pwcBuried"))

      local ok, message = createScrollBox("pwcBuried", track("pwcAfterTheFuneral"), 0, 0, 80, 40)

      assert.is_false(ok)
      assert.are.equal("window 'pwcBuried' not found", message)
      assert.is_nil(windowType("pwcAfterTheFuneral"))
    end)

    it("refuses a name that is registered but cannot hold anything", function()
      assert.is_true(createMiniConsole(track("pwcNotAParent"), 0, 0, 120, 60))

      local ok, message = createScrollBox("pwcNotAParent", track("pwcInAConsole"), 0, 0, 60, 40)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNotAParent' not found", message)
      assert.is_nil(windowType("pwcInAConsole"))
    end)

    it("refuses before moving an element that already exists under that name", function()
      assert.is_true(createScrollBox("main", track("pwcAlreadyThere"), 0, 0, 50, 50))

      local ok, message = createScrollBox("pwcNoSuchWindow", "pwcAlreadyThere", 99, 99, 10, 10)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      -- a create...() call on an existing name moves and resizes it; a parent
      -- window that cannot be resolved stops that before anything is touched
      local x, y, width, height = getWindowGeometry("pwcAlreadyThere")
      assert.are.equal(0, x)
      assert.are.equal(0, y)
      assert.are.equal(50, width)
      assert.are.equal(50, height)
    end)
  end)

  describe("a parent window name that does match", function()
    it("still creates into the main console", function()
      assert.is_true(createScrollBox("main", track("pwcMainBox"), 0, 0, 120, 80))
      assert.is_true(createMiniConsole("main", track("pwcMainConsole"), 0, 90, 120, 60))
      assert.is_true(createCommandLine("main", track("pwcMainCmdLine"), 0, 160, 120, 30))
      assert.is_true(createTextEdit("main", track("pwcMainTextEdit"), 0, 200, 120, 80))
      assert.is_true(createLabel("main", track("pwcMainLabel"), 0, 290, 120, 30, true))

      assert.are.equal("scrollbox", windowType("pwcMainBox"))
      assert.are.equal("miniconsole", windowType("pwcMainConsole"))
      assert.are.equal("commandline", windowType("pwcMainCmdLine"))
      assert.are.equal("textedit", windowType("pwcMainTextEdit"))
      assert.are.equal("label", windowType("pwcMainLabel"))
    end)

    it("takes the main console spelled in any case, as setWindow does", function()
      assert.is_true(createScrollBox("Main", track("pwcCaseBox"), 0, 0, 120, 80))
      assert.is_true(createCommandLine("MAIN", track("pwcCaseCmdLine"), 0, 90, 120, 30))

      assert.are.equal("scrollbox", windowType("pwcCaseBox"))
      assert.are.equal("commandline", windowType("pwcCaseCmdLine"))
    end)

    it("still creates into a user window, and the elements are really inside it", function()
      assert.is_true(openUserWindow(track("pwcHost"), false))

      assert.is_true(createScrollBox("pwcHost", track("pwcHostBox"), 0, 0, 120, 80))
      assert.is_true(createMiniConsole("pwcHost", track("pwcHostConsole"), 0, 90, 120, 60))
      assert.is_true(createCommandLine("pwcHost", track("pwcHostCmdLine"), 0, 160, 120, 30))
      assert.is_true(createTextEdit("pwcHost", track("pwcHostTextEdit"), 0, 200, 120, 60))
      assert.is_true(createLabel("pwcHost", track("pwcHostLabel"), 0, 270, 120, 30, true))

      assert.are.equal("scrollbox", windowType("pwcHostBox"))
      assert.are.equal("miniconsole", windowType("pwcHostConsole"))
      assert.are.equal("commandline", windowType("pwcHostCmdLine"))
      assert.are.equal("textedit", windowType("pwcHostTextEdit"))
      assert.are.equal("label", windowType("pwcHostLabel"))

      -- hiding the user window hides its dock, so anything that answers
      -- "visible" afterwards is not in it but on the main console
      hideWindow("pwcHost")
      assert.is_false(windowVisible("pwcHostBox"))
      assert.is_false(windowVisible("pwcHostConsole"))
      assert.is_false(windowVisible("pwcHostCmdLine"))
      assert.is_false(windowVisible("pwcHostTextEdit"))
      assert.is_false(windowVisible("pwcHostLabel"))

      showWindow("pwcHost")
      assert.is_true(windowVisible("pwcHostBox"))
      assert.is_true(windowVisible("pwcHostLabel"))
    end)

    it("still creates into a scroll box, and the elements are really inside it", function()
      assert.is_true(createScrollBox(track("pwcOuterBox"), 0, 0, 200, 200))

      assert.is_true(createScrollBox("pwcOuterBox", track("pwcInnerBox"), 0, 0, 100, 40))
      assert.is_true(createMiniConsole("pwcOuterBox", track("pwcInnerConsole"), 0, 50, 120, 60))
      assert.is_true(createCommandLine("pwcOuterBox", track("pwcInnerCmdLine"), 0, 120, 120, 30))
      assert.is_true(createTextEdit("pwcOuterBox", track("pwcInnerTextEdit"), 0, 160, 120, 30))

      assert.are.equal("scrollbox", windowType("pwcInnerBox"))
      assert.are.equal("miniconsole", windowType("pwcInnerConsole"))
      assert.are.equal("commandline", windowType("pwcInnerCmdLine"))
      assert.are.equal("textedit", windowType("pwcInnerTextEdit"))

      hideWindow("pwcOuterBox")
      assert.is_false(windowVisible("pwcInnerBox"))
      assert.is_false(windowVisible("pwcInnerConsole"))
      assert.is_false(windowVisible("pwcInnerCmdLine"))
      assert.is_false(windowVisible("pwcInnerTextEdit"))

      showWindow("pwcOuterBox")
      assert.is_true(windowVisible("pwcInnerConsole"))
    end)
  end)

  -- A user window's elements belong to its dock widget's content widget, so
  -- deleting the window destroys them with it - but through deleteLater(),
  -- which Qt only delivers once control is back in Mudlet's own event loop. A
  -- spec runs inside one Lua call and never reaches that, so it cannot observe
  -- the destruction; what it can check is that the window's own name goes at
  -- once. (In that intermediate state the children's names are still
  -- registered, which is why an element can still be created into a scroll box
  -- that is already on its way out. Nothing here relies on either.)
  describe("deleting a user window", function()
    it("frees the window's own name at once", function()
      assert.is_true(openUserWindow("pwcDoomed", false))
      assert.is_true(createScrollBox("pwcDoomed", track("pwcDoomedBox"), 0, 0, 220, 140))
      assert.are.equal("scrollbox", windowType("pwcDoomedBox"))

      assert.is_true(deleteMiniConsole("pwcDoomed"))

      assert.is_nil(windowType("pwcDoomed"))
    end)
  end)

  describe("the wrappers that create these elements", function()
    it("createConsole reports and returns a refusal instead of creating nothing quietly", function()
      local reported
      -- a spec runs in busted's own environment, so the stub has to go into _G
      -- for the library code to see it
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function() _G.printError = realPrintError end)

      local ok, message = createConsole("pwcNoSuchWindow", track("pwcConsoleInNowhere"), 8, 80, 20, 200, 400)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_nil(windowType("pwcConsoleInNowhere"))
      assert.is_truthy(reported)
      assert.is_truthy(reported:find("pwcConsoleInNowhere", 1, true))
    end)

    it("createConsole still creates a console on the main console", function()
      assert.is_true(createConsole(track("pwcGoodConsole"), 8, 80, 20, 200, 400))
      assert.are.equal("miniconsole", windowType("pwcGoodConsole"))
    end)

    it("Geyser.ScrollBox says so rather than handing back an object with no widget", function()
      local reported
      -- a spec runs in busted's own environment, so the stub has to go into _G
      -- for the library code to see it
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function()
        _G.printError = realPrintError
        Geyser.parentWindows["pwcGeyserBox"] = nil
      end)

      local box = Geyser.ScrollBox:new({name = track("pwcGeyserBox"), windowname = "pwcNoSuchWindow", x = 0, y = 0, width = 100, height = 100})

      assert.is_not_nil(box)
      assert.is_nil(windowType("pwcGeyserBox"))
      assert.is_truthy(reported)
      assert.is_truthy(reported:find("pwcGeyserBox", 1, true))
      assert.is_truthy(reported:find("window 'pwcNoSuchWindow' not found", 1, true))
    end)

    it("Geyser.MiniConsole says so too", function()
      local reported
      -- a spec runs in busted's own environment, so the stub has to go into _G
      -- for the library code to see it
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function() _G.printError = realPrintError end)

      Geyser.MiniConsole:new({name = track("pwcGeyserConsole"), windowname = "pwcNoSuchWindow", x = 0, y = 0, width = 100, height = 50})

      assert.is_nil(windowType("pwcGeyserConsole"))
      assert.is_truthy(reported)
      assert.is_truthy(reported:find("window 'pwcNoSuchWindow' not found", 1, true))
    end)

    -- An element that already exists under the name being created is the case
    -- the wrappers cannot read off windowType(): the native creators check the
    -- parent window before they look at the element's name, so the refusal
    -- arrives while a window of that name is there to be found.
    it("createConsole reports a refused parent window even when that console exists", function()
      assert.is_true(createConsole(track("pwcConsoleTwice"), 8, 40, 10, 100, 100))
      local x, y, width, height = getWindowGeometry("pwcConsoleTwice")

      local reported
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function() _G.printError = realPrintError end)

      local ok, message = createConsole("pwcNoSuchWindow", "pwcConsoleTwice", 14, 20, 5, 300, 300)

      assert.is_false(ok)
      assert.are.equal("window 'pwcNoSuchWindow' not found", message)
      assert.is_truthy(reported)
      assert.is_truthy(reported:find("window 'pwcNoSuchWindow' not found", 1, true))
      -- the refusal has to stop the wrapper before it re-fonts, resizes and
      -- moves the console that happens to hold the name
      assert.are.same({x, y, width, height}, {getWindowGeometry("pwcConsoleTwice")})
    end)

    it("createConsole still moves a console of its own when the parent window is fine", function()
      assert.is_true(createConsole(track("pwcConsoleAgain"), 8, 40, 10, 100, 100))

      local reported
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function() _G.printError = realPrintError end)

      -- calling a creator again to move or resize what it made is an idiom, not
      -- a failure, and stays as quiet as it always was
      assert.is_true(createConsole("main", "pwcConsoleAgain", 8, 40, 10, 20, 20))

      assert.is_nil(reported)
      local x, y = getWindowGeometry("pwcConsoleAgain")
      assert.are.equal(20, x)
      assert.are.equal(20, y)
    end)

    it("Geyser.MiniConsole reports it when a console already holds the name", function()
      assert.is_true(createMiniConsole(track("pwcTakenConsole"), 0, 0, 100, 50))

      local reported
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function()
        _G.printError = realPrintError
        Geyser.windowList["pwcTakenConsole"] = nil
      end)

      Geyser.MiniConsole:new({name = "pwcTakenConsole", windowname = "pwcNoSuchWindow", x = 0, y = 0, width = 100, height = 50})

      assert.is_truthy(reported)
      assert.is_truthy(reported:find("window 'pwcNoSuchWindow' not found", 1, true))
    end)

    it("Geyser.ScrollBox reports it before taking a name it did not create as a parent window", function()
      assert.is_true(createScrollBox(track("pwcTakenBox"), 0, 0, 120, 120))

      local reported
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function()
        _G.printError = realPrintError
        Geyser.parentWindows["pwcTakenBox"] = nil
        Geyser.windowList["pwcTakenBox"] = nil
      end)

      Geyser.ScrollBox:new({name = "pwcTakenBox", windowname = "pwcNoSuchWindow", x = 0, y = 0, width = 100, height = 100})

      assert.is_truthy(reported)
      assert.is_truthy(reported:find("window 'pwcNoSuchWindow' not found", 1, true))
      -- the object still registers itself under that name, as every Geyser
      -- constructor hands one back whatever happened; what this pins is that
      -- the refusal is said out loud rather than swallowed
      assert.is_not_nil(Geyser.parentWindows["pwcTakenBox"])
    end)

    it("Geyser.MiniConsole stays quiet when it does create its console", function()
      local reported
      -- a spec runs in busted's own environment, so the stub has to go into _G
      -- for the library code to see it
      local realPrintError = printError
      _G.printError = function(message) reported = message end
      finally(function() _G.printError = realPrintError end)

      Geyser.MiniConsole:new({name = track("pwcGeyserGoodConsole"), x = 0, y = 0, width = 100, height = 50})

      assert.are.equal("miniconsole", windowType("pwcGeyserGoodConsole"))
      assert.is_nil(reported)
    end)
  end)
end)
