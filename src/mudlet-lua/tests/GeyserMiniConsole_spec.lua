local function geometry(name)
  local x, y, width, height = getWindowGeometry(name)
  return {x = x, y = y, width = width, height = height}
end

describe("Tests functionality of Geyser.MiniConsole", function()
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

  describe("Geyser.MiniConsole:new/new2", function()
    it("creates a miniconsole widget at the constrained geometry", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcNew", x = 30, y = 40, width = 300, height = 150}))
      -- Geyser's own type string is camel cased, Mudlet's windowType is not
      assert.are.equal("miniConsole", console.type)
      assert.are.equal("miniconsole", windowType("gmcNew"))
      assert.are.same({x = 30, y = 40, width = 300, height = 150}, geometry("gmcNew"))
      assert.is_true(windowVisible("gmcNew"))
    end)

    it("resolves percentages against its container", function()
      local container = track(Geyser.Container:new({name = "gmcBox", x = 100, y = 50, width = 400, height = 200}))
      track(Geyser.MiniConsole:new({name = "gmcInBox", x = "25%", y = "50%", width = "50%", height = "50%"}, container))
      assert.are.same({x = 200, y = 150, width = 200, height = 100}, geometry("gmcInBox"))
    end)

    it("takes the font size from its container when it is not given one", function()
      local container = track(Geyser.Container:new({name = "gmcFontBox", x = 0, y = 0, width = 200, height = 100, fontSize = 12}))
      track(Geyser.MiniConsole:new({name = "gmcInheritsFont"}, container))
      assert.are.equal(12, getFontSize("gmcInheritsFont"))
    end)

    it("new2 marks the console as using add2", function()
      local console = track(Geyser.MiniConsole:new2({name = "gmcNew2", x = 0, y = 0, width = 100, height = 50}))
      assert.is_true(console.useAdd2)
      assert.are.equal("miniconsole", windowType("gmcNew2"))
    end)
  end)

  describe("Geyser.MiniConsole geometry and visibility", function()
    local console

    before_each(function()
      console = track(Geyser.MiniConsole:new({name = "gmcMove", x = 10, y = 20, width = 200, height = 100}))
    end)

    it("moves and resizes the widget", function()
      console:move(60, 70)
      console:resize(120, 60)
      assert.are.same({x = 60, y = 70, width = 120, height = 60}, geometry("gmcMove"))
    end)

    it("hides and shows the widget", function()
      console:hide()
      assert.is_false(windowVisible("gmcMove"))
      console:show()
      assert.is_true(windowVisible("gmcMove"))
    end)

    it("follows its container when the container moves", function()
      local container = track(Geyser.Container:new({name = "gmcDragBox", x = 0, y = 0, width = 200, height = 100}))
      track(Geyser.MiniConsole:new({name = "gmcDragged", x = 0, y = 0, width = "100%", height = "100%"}, container))
      container:move(150, 30)
      assert.are.same({x = 150, y = 30, width = 200, height = 100}, geometry("gmcDragged"))
    end)
  end)

  describe("Geyser.MiniConsole:setWrap/enableAutoWrap/disableAutoWrap/resetAutoWrap", function()
    it("sets the wrap column", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcWrap", x = 0, y = 0, width = 300, height = 100}))
      console:setWrap(42)
      assert.are.equal(42, console.wrapAt)
      assert.are.equal(42, getWindowWrap("gmcWrap"))
    end)

    it("refuses to set the wrap while auto wrap is on", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcWrapLocked", x = 0, y = 0, width = 300, height = 100}))
      console:enableAutoWrap()
      local derivedWrap = getWindowWrap("gmcWrapLocked")
      local result, message = console:setWrap(11)
      assert.is_nil(result)
      assert.is_truthy(message:find("autoWrap is enabled", 1, true))
      assert.are.equal(derivedWrap, getWindowWrap("gmcWrapLocked"))
    end)

    it("derives the wrap from the width when auto wrap is on", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcAutoWrap", x = 0, y = 0, width = 300, height = 100, wrapAt = "auto"}))
      local charWidth = calcFontSize("gmcAutoWrap")
      assert.is_true(console.autoWrap)
      assert.are.equal(math.floor(300 / charWidth), getWindowWrap("gmcAutoWrap"))
    end)

    it("re-derives the wrap when the console is resized", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcRewrap", x = 0, y = 0, width = 300, height = 100, autoWrap = true}))
      local charWidth = calcFontSize("gmcRewrap")
      assert.are.equal(math.floor(300 / charWidth), getWindowWrap("gmcRewrap"))
      console:resize(150, 100)
      assert.are.equal(math.floor(150 / charWidth), getWindowWrap("gmcRewrap"))
    end)

    it("stops re-deriving the wrap once auto wrap is disabled", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcNoAutoWrap", x = 0, y = 0, width = 300, height = 100, autoWrap = true}))
      console:disableAutoWrap()
      console:setWrap(17)
      console:resize(150, 100)
      assert.is_false(console.autoWrap)
      assert.are.equal(17, getWindowWrap("gmcNoAutoWrap"))
    end)

    it("reports that resetAutoWrap has nothing to do when auto wrap is off", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcResetWrap", x = 0, y = 0, width = 300, height = 100}))
      local result, message = console:resetAutoWrap()
      assert.is_nil(result)
      assert.is_truthy(message:find("Autowrap is not enabled", 1, true))
    end)
  end)

  describe("Geyser.MiniConsole:setFontSize/getFont", function()
    it("changes the font size of the console", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcFont", x = 0, y = 0, width = 300, height = 100, fontSize = 8}))
      assert.are.equal(8, getFontSize("gmcFont"))
      console:setFontSize(14)
      assert.are.equal(14, getFontSize("gmcFont"))
      assert.are.equal(14, console.fontSize)
    end)

    it("re-derives an auto wrap from the new font size", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcFontWrap", x = 0, y = 0, width = 300, height = 100, fontSize = 8, autoWrap = true}))
      local smallWrap = getWindowWrap("gmcFontWrap")
      console:setFontSize(20)
      local bigWrap = getWindowWrap("gmcFontWrap")
      assert.are.equal(math.floor(300 / calcFontSize("gmcFontWrap")), bigWrap)
      assert.is_true(bigWrap < smallWrap)
    end)

    it("reads the font family back out of Mudlet", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcFontFamily", x = 0, y = 0, width = 300, height = 100}))
      local family = getFont("gmcFontFamily")
      assert.are.equal("string", type(family))
      assert.is_true(#family > 0)
      -- getFont refreshes the cached family rather than reporting the cache
      console.font = "not the real font"
      assert.are.equal(family, console:getFont())
      assert.are.equal(family, console.font)
    end)
  end)

  describe("Geyser.MiniConsole:clear", function()
    it("empties the console", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcClear", x = 0, y = 0, width = 300, height = 100}))
      console:echo("one\ntwo\n")
      assert.is_true(getLineCount("gmcClear") > 1)
      console:clear()
      assert.are.equal(0, getLineCount("gmcClear"))
    end)
  end)

  describe("Geyser.MiniConsole:type_delete", function()
    it("deletes the widget with the object", function()
      local console = track(Geyser.MiniConsole:new({name = "gmcDelete", x = 0, y = 0, width = 100, height = 50}))
      assert.is_not_nil(getWindowGeometry("gmcDelete"))
      console:delete()
      assert.is_nil(getWindowGeometry("gmcDelete"))
      assert.is_nil(Geyser.windowList.gmcDelete)
    end)
  end)
end)
