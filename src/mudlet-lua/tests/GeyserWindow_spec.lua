-- Geyser.Window is the abstract base for the Mudlet primitives that hold text.
-- Its methods are exercised through a Geyser.MiniConsole, the simplest
-- subclass that owns a real widget, and read back with the console getters.

-- Selects the line last written by a newline terminated echo so
-- getCurrentLine/getTextFormat report on it. getLineCount returns the index of
-- the console's last line rather than a count, and the trailing newline leaves
-- the cursor on that (still empty) last line, so the text is one line above it.
local function lastLine(name)
  local index = getLineCount(name) - 1
  assert.is_true(index >= 0, "nothing has been echoed to " .. name .. " yet")
  moveCursor(name, 0, index)
  selectCurrentLine(name)
  return getCurrentLine(name)
end

describe("Tests functionality of Geyser.Window", function()
  local created
  local console

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
    console = track(Geyser.MiniConsole:new({name = "gwsConsole", x = 0, y = 0, width = 300, height = 100}))
  end)

  after_each(function()
    for _, object in ipairs(created) do
      if alive(object) then
        object:delete()
      end
    end
    created = {}
  end)

  describe("Geyser.Window:new", function()
    it("defaults the type to window and owns no widget of its own", function()
      local window = track(Geyser.Window:new({name = "gwsAbstract", x = 0, y = 0, width = 100, height = 100}))
      assert.are.equal("window", window.type)
      assert.are.equal(window, Geyser.windowList.gwsAbstract)
      -- Geyser.Window is abstract: only its subclasses create a Mudlet primitive
      assert.is_nil(getWindowGeometry("gwsAbstract"))
    end)

    it("provides the colour defaults its subclasses inherit", function()
      local window = track(Geyser.Window:new({name = "gwsColours", x = 0, y = 0, width = 10, height = 10}))
      assert.are.equal("white", window.fgColor)
      assert.are.equal("black", window.bgColor)
      assert.are.equal("#202020", window.color)
      assert.are.equal("", window.message)
    end)
  end)

  describe("Geyser.Window:echo/cecho/decho/hecho", function()
    it("echoes plain text and remembers the message", function()
      console:echo("plain line\n")
      assert.are.equal("plain line\n", console.message)
      assert.are.equal("plain line", lastLine("gwsConsole"))
    end)

    it("cecho colours the text by name", function()
      console:cecho("<green>green line\n")
      assert.are.equal("green line", lastLine("gwsConsole"))
      assert.are.same({0, 255, 0}, getTextFormat("gwsConsole").foreground)
    end)

    it("decho colours the text by rgb triple", function()
      console:decho("<0,0,255>blue line\n")
      assert.are.equal("blue line", lastLine("gwsConsole"))
      assert.are.same({0, 0, 255}, getTextFormat("gwsConsole").foreground)
    end)

    it("hecho colours the text by hex code", function()
      console:hecho("|cff0000red line\n")
      assert.are.equal("red line", lastLine("gwsConsole"))
      assert.are.same({255, 0, 0}, getTextFormat("gwsConsole").foreground)
    end)

    it("reuses the last message when called without one", function()
      console:echo("remembered\n")
      console:cecho()
      assert.are.equal("remembered\n", console.message)
      assert.are.equal("remembered", lastLine("gwsConsole"))
    end)

    it("echo without a message redisplays the stored one instead of wiping it", function()
      console:echo("kept\n")
      local lines = getLineCount("gwsConsole")
      console:echo()
      assert.are.equal("kept\n", console.message)
      -- the line count pins that it was written again, not merely left alone
      assert.are.equal(lines + 1, getLineCount("gwsConsole"))
      assert.are.equal("kept", lastLine("gwsConsole"))
    end)
  end)

  describe("Geyser.Window:getFgColor/getBgColor/setBgColor/setFgColor", function()
    it("round-trips the foreground colour", function()
      console:setFgColor(255, 0, 0)
      console:echo("coloured\n")
      lastLine("gwsConsole")
      assert.are.same({255, 0, 0}, getTextFormat("gwsConsole").foreground)
      local red, green, blue = console:getFgColor()
      assert.are.same({255, 0, 0}, {red, green, blue})
    end)

    it("round-trips the background colour", function()
      console:setBgColor(0, 0, 255)
      console:echo("coloured\n")
      lastLine("gwsConsole")
      assert.are.same({0, 0, 255}, getTextFormat("gwsConsole").background)
      local red, green, blue = console:getBgColor()
      assert.are.same({0, 0, 255}, {red, green, blue})
    end)

    it("accepts a colour name", function()
      console:setFgColor("green")
      console:echo("named\n")
      lastLine("gwsConsole")
      assert.are.same({0, 255, 0}, getTextFormat("gwsConsole").foreground)
    end)

    it("accepts a hex colour", function()
      console:setFgColor("#ff00ff")
      console:echo("hexed\n")
      lastLine("gwsConsole")
      assert.are.same({255, 0, 255}, getTextFormat("gwsConsole").foreground)
    end)
  end)

  describe("Geyser.Window:setTextFormat/setBold/setUnderline/setItalics", function()
    it("starts out with no attributes set", function()
      console:echo("first\n")
      lastLine("gwsConsole")
      local format = getTextFormat("gwsConsole")
      assert.is_false(format.bold)
      assert.is_false(format.italic)
      assert.is_false(format.underline)
    end)

    it("turns bold, italics and underline on and off again", function()
      console:setBold(true)
      console:setItalics(true)
      console:setUnderline(true)
      console:echo("styled\n")
      lastLine("gwsConsole")
      local styled = getTextFormat("gwsConsole")
      assert.is_true(styled.bold)
      assert.is_true(styled.italic)
      assert.is_true(styled.underline)
      console:setBold(false)
      console:setItalics(false)
      console:setUnderline(false)
      console:echo("plain\n")
      lastLine("gwsConsole")
      local plain = getTextFormat("gwsConsole")
      assert.is_false(plain.bold)
      assert.is_false(plain.italic)
      assert.is_false(plain.underline)
    end)

    it("sets both colours and all attributes at once", function()
      -- the first colour triple is the background and the second the
      -- foreground, matching Mudlet's setTextFormat
      console:setTextFormat(10, 20, 30, 200, 100, 50, true, true, true)
      console:echo("formatted\n")
      lastLine("gwsConsole")
      local format = getTextFormat("gwsConsole")
      assert.are.same({10, 20, 30}, format.background)
      assert.are.same({200, 100, 50}, format.foreground)
      assert.is_true(format.bold)
      assert.is_true(format.underline)
      assert.is_true(format.italic)
    end)
  end)

  describe("Geyser.Window:paste", function()
    it("pastes a selection copied from another console", function()
      local source = track(Geyser.MiniConsole:new({name = "gwsSource", x = 0, y = 0, width = 300, height = 100}))
      source:echo("copy me\n")
      moveCursor("gwsSource", 0, 0)
      selectCurrentLine("gwsSource")
      copy("gwsSource")
      console:paste()
      moveCursor("gwsConsole", 0, 0)
      selectCurrentLine("gwsConsole")
      assert.are.equal("copy me", getCurrentLine("gwsConsole"))
    end)
  end)
end)
