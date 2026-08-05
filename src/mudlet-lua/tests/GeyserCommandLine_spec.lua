-- Geyser.CommandLine wraps Mudlet's sub-command-line primitive, so everything
-- it does is read back through windowType/getWindowGeometry/windowVisible and
-- getCmdLine rather than through the Geyser object's own bookkeeping.
local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.CommandLine", function()
  local created

  local function track(object)
    created[#created + 1] = object
    return object
  end

  local function alive(object)
    if not object or not object.container or not object.container.windowList then
      return false
    end
    return object.container.windowList[object.name] == object
  end

  before_each(function()
    created = {}
  end)

  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  describe("Geyser.CommandLine:new/new2", function()
    it("creates a command line widget at the constrained geometry", function()
      local commandLine = track(Geyser.CommandLine:new({name = "gclNew", x = 30, y = 40, width = 200, height = 30}))
      -- Geyser's own type string is camel cased, Mudlet's windowType is not
      assert.are.equal("commandLine", commandLine.type)
      assert.are.equal("commandline", windowType("gclNew"))
      assert.are.same({x = 30, y = 40, width = 200, height = 30}, geometry("gclNew"))
      assert.is_true(windowVisible("gclNew"))
      assert.are.equal(commandLine, Geyser.windowList.gclNew)
      assert.are.equal("main", commandLine.windowname)
    end)

    it("uses Geyser's defaults when no constraints are given", function()
      track(Geyser.CommandLine:new({name = "gclDefaults"}))
      assert.are.same({x = 10, y = 10, width = 300, height = 200}, geometry("gclDefaults"))
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "gclBox", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.CommandLine:new({name = "gclInBox", x = "25%", y = "50%", width = "50%", height = "25%"}, container))
      assert.are.same({x = 200, y = 150, width = 200, height = 50}, geometry("gclInBox"))
    end)

    it("new2 marks the command line as using add2", function()
      local commandLine = track(Geyser.CommandLine:new2({name = "gclNew2", x = 0, y = 0, width = 100, height = 30}))
      assert.is_true(commandLine.useAdd2)
      assert.are.equal("commandline", windowType("gclNew2"))
    end)

    it("keeps a new command line of a hidden add2 container hidden", function()
      local container = track(Geyser.Container:new2({name = "gclHiddenBox", x = 0, y = 0, width = 200, height = 100}))
      container:hide()
      local commandLine = track(Geyser.CommandLine:new2({name = "gclHiddenChild", x = 0, y = 0, width = 50, height = 20}, container))
      assert.is_true(commandLine.auto_hidden)
      assert.is_false(windowVisible("gclHiddenChild"))
      container:show()
      assert.is_true(windowVisible("gclHiddenChild"))
    end)
  end)

  describe("Geyser.CommandLine:print/append/getText/clear", function()
    local commandLine

    before_each(function()
      commandLine = track(Geyser.CommandLine:new({name = "gclText", x = 0, y = 0, width = 200, height = 30}))
    end)

    it("prints text into the command line", function()
      commandLine:print("hello")
      assert.are.equal("hello", commandLine:getText())
      assert.are.equal("hello", getCmdLine("gclText"))
    end)

    it("replaces what was there on the next print", function()
      commandLine:print("first")
      commandLine:print("second")
      assert.are.equal("second", commandLine:getText())
    end)

    it("appends to the text already in the command line", function()
      commandLine:print("hello")
      commandLine:append(" world")
      assert.are.equal("hello world", commandLine:getText())
    end)

    it("appends into an empty command line", function()
      commandLine:append("only")
      assert.are.equal("only", commandLine:getText())
    end)

    it("clears the command line", function()
      commandLine:print("something")
      commandLine:clear()
      assert.are.equal("", commandLine:getText())
      assert.are.equal("", getCmdLine("gclText"))
    end)
  end)

  describe("Geyser.CommandLine:selectText", function()
    it("reports success", function()
      local commandLine = track(Geyser.CommandLine:new({name = "gclSelect", x = 0, y = 0, width = 100, height = 30}))
      commandLine:print("select me")
      assert.is_true(commandLine:selectText())
      -- selecting must not disturb what is typed
      assert.are.equal("select me", commandLine:getText())
    end)
  end)

  pending("Geyser.CommandLine:selectText selects every character - the selection itself is not readable from Lua and needs a getCmdLineSelection getter")

  describe("Geyser.CommandLine:setStyleSheet", function()
    it("reuses the remembered stylesheet when called without one", function()
      local commandLine = track(Geyser.CommandLine:new({name = "gclCss", x = 0, y = 0, width = 100, height = 30}))
      commandLine:setStyleSheet("background-color: red;")
      assert.are.equal("background-color: red;", commandLine.stylesheet)
      commandLine:setStyleSheet()
      assert.are.equal("background-color: red;", commandLine.stylesheet)
    end)
  end)

  pending("Geyser.CommandLine:setStyleSheet applies the stylesheet to the widget - needs a getCmdLineStyleSheet getter")

  describe("Geyser.CommandLine:setAction/resetAction", function()
    it("remembers the action and its arguments, and forgets them again", function()
      local commandLine = track(Geyser.CommandLine:new({name = "gclAction", x = 0, y = 0, width = 100, height = 30}))
      local action = function() end
      commandLine:setAction(action, "one", "two")
      assert.are.equal(action, commandLine.actionFunc)
      assert.are.same({"one", "two"}, commandLine.actionArgs)
      commandLine:resetAction()
      assert.is_nil(commandLine.actionFunc)
      assert.is_nil(commandLine.actionArgs)
    end)
  end)

  pending("Geyser.CommandLine:setAction runs the action when the command line sends its text - no Lua API submits input to a command line, so this needs a functional test")

  describe("Geyser.CommandLine geometry and visibility", function()
    local commandLine

    before_each(function()
      commandLine = track(Geyser.CommandLine:new({name = "gclMove", x = 10, y = 20, width = 200, height = 30}))
    end)

    it("moves and resizes the widget", function()
      commandLine:move(60, 70)
      commandLine:resize(120, 40)
      assert.are.same({x = 60, y = 70, width = 120, height = 40}, geometry("gclMove"))
    end)

    it("hides and shows the widget", function()
      commandLine:hide()
      assert.is_true(commandLine.hidden)
      assert.is_false(windowVisible("gclMove"))
      commandLine:show()
      assert.is_false(commandLine.hidden)
      assert.is_true(windowVisible("gclMove"))
    end)

    it("follows its container when the container moves", function()
      local container = track(Geyser.Container:new({name = "gclDragBox", x = 0, y = 0, width = 200, height = 100}))
      track(Geyser.CommandLine:new({name = "gclDragged", x = 0, y = 0, width = "100%", height = 30}, container))
      container:move(150, 30)
      assert.are.same({x = 150, y = 30, width = 200, height = 30}, geometry("gclDragged"))
    end)
  end)

  describe("Geyser.CommandLine error paths", function()
    it("raises on a constraint it cannot parse, leaving no widget behind", function()
      -- the object is registered before its constraints are resolved, so the
      -- failed attempt has to be swept out of the root window list by hand
      finally(function()
        local zombie = Geyser.windowList.gclBadConstraint
        if zombie then
          zombie:delete()
        end
      end)
      local ok, message = pcall(function()
        return Geyser.CommandLine:new({name = "gclBadConstraint", x = 0, y = 0, width = true, height = 20})
      end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("GeyserSetConstraints.lua", 1, true))
      assert.is_nil(windowType("gclBadConstraint"))
    end)

    it("raises when printing something that is not text, leaving the text alone", function()
      local commandLine = track(Geyser.CommandLine:new({name = "gclBadPrint", x = 0, y = 0, width = 100, height = 30}))
      commandLine:print("kept")
      local ok, message = pcall(function() commandLine:print(nil) end)
      assert.is_false(ok)
      assert.is_truthy(tostring(message):find("printCmdLine", 1, true))
      assert.are.equal("kept", commandLine:getText())
    end)
  end)

  describe("Geyser.CommandLine:type_delete", function()
    it("deletes the widget with the object", function()
      local commandLine = track(Geyser.CommandLine:new({name = "gclDelete", x = 0, y = 0, width = 100, height = 30}))
      assert.are.equal("commandline", windowType("gclDelete"))
      commandLine:delete()
      assert.is_nil(windowType("gclDelete"))
      assert.is_nil(getWindowGeometry("gclDelete"))
      assert.is_nil(Geyser.windowList.gclDelete)
    end)
  end)
end)
