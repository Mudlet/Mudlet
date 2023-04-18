describe("Tests functionality of Geyser.StyleSheet", function()

  describe("Tests the functionality of Geyser.StyleSheet:parseCSS", function()
    it("Should convert a string stylesheet into a table of properties", function()
      local sheet = [[
        background-color: black;
        color: green;
      ]]
      local expected = {
        ['background-color'] = "black",
        color = "green",
      }
      local actual = Geyser.StyleSheet:parseCSS(sheet)
      assert.are.same(expected, actual)
    end)

    it("Should extract the style and target if both are provided in the stylesheet", function()
      local sheet = [[
        QPlainTextEdit  { 
          background-color: black;
          color: green;
        }
      ]]
      local styleTable, target = Geyser.StyleSheet:parseCSS(sheet)
      local expectedTarget = "QPlainTextEdit"
      local expectedTable = {
        ['background-color'] = "black",
        color = "green",
      }
      assert.equal(expectedTarget, target)
      assert.are.same(expectedTable, styleTable)
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:get", function()
    local sheet = Geyser.StyleSheet:new([[
      background-color: black;
      color: green;
    ]])

    it("Should return nil if a property is not set", function()
      local actual = sheet:get("bobble")
      assert.is_nil(actual)
    end)

    it("Should return the property's value", function()
      local expected = "green"
      local actual = sheet:get("color")
      assert.equal(expected, actual)
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:set", function()
    local sheet

    before_each(function()
      sheet = Geyser.StyleSheet:new([[
        background-color: black;
        color: green;
      ]])
    end)

    it("Should change a property's value", function()
      assert.equal("green", sheet:get("color"))
      sheet:set("color", "blue")
      local expected = "blue"
      local actual = sheet:get("color")
      assert.equal(expected, actual)
    end)

    it("Should add a new property", function()
      assert.is_nil(sheet:get("bobble"))
      sheet:set("bobble", "babble")
      local expected = "babble"
      local actual = sheet:get("bobble")
      assert.equal(expected, actual)
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:getStyleTable", function()
    local parent, child
    before_each(function()
      parent = Geyser.StyleSheet:new([[
        background-color: black;
        color: green;
      ]])
      child = Geyser.StyleSheet:new([[
        color: blue;
        font: "Ubuntu Mono";
      ]], parent)
    end)

    it("Should return the combined/inherited stylesheet by default", function()
      local expected = {
        ['background-color'] = "black",
        ['color'] = "blue",
        ['font'] = '"Ubuntu Mono"'
      }
      local actual = child:getStyleTable()
      assert.are.same(expected, actual)
    end)

    it("Should return only its own properties if passed false as a parameter", function()
      local expected = {
        color = "blue",
        font = '"Ubuntu Mono"'
      }
      local actual = child:getStyleTable(false)
      assert.are.same(expected, actual)
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:setStyleTable", function()
    local parent, child
    before_each(function()
      parent = Geyser.StyleSheet:new([[
        background-color: black;
        color: green;
      ]])
      child = Geyser.StyleSheet:new([[
        color: blue;
        font: "Ubuntu Mono";
      ]], parent)
    end)

    it("Should replace the existing style table", function()
      local expected = {
        color = "purple"
      }
      child:setStyleTable({color = "purple"})
      local actual = child:getStyleTable(false)
      assert.are.same(expected, actual)
    end)

    it("Should clear the style table if called with no parameters", function()
      local expected = {}
      child:setStyleTable()
      local actual = child:getStyleTable(false)
      assert.are.same(expected, actual)
    end)

    it("Should raise an error if passed anything other than a table", function()
      local stt = function()
        child:setStyleTable(4)
      end
      assert.error_matches(stt, "bad argument #1 type")
    end)

    it("Should not effect the parent", function()
      child:setStyleTable({color = "purple"})
      local expected = "green"
      local actual = parent:getStyleTable().color
      assert.equal(expected, actual)
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:setParent", function()
    local parent, child
    before_each(function()
      parent = Geyser.StyleSheet:new([[
        background-color: black;
        color: green;
      ]])
      child = Geyser.StyleSheet:new([[
        color: blue;
        font: "Ubuntu Mono";
      ]])
    end)

    it("Should set the parent for your stylesheet", function()
      child:setParent(parent)
      assert.equal(parent, child.parent)
    end)

    it("Should clear the parent if no parent is passed in", function()
      child:setParent(parent) -- give it something to clear
      child:setParent()       -- and clear it
      assert.is_nil(child.parent)
    end)

    it("Should throw an error if passed anything but a Geyser.StyleSheet", function()
      local sp = function()
        child:setParent("Not proper")
      end
      assert.error_matches(sp, "bad argument #1 type")
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:setTarget", function()
    local stylesheet
    before_each(function()
      stylesheet = Geyser.StyleSheet:new([[
        background-color: black;
        color: green;
      ]])
    end)

    it("Should set the target", function()
      stylesheet:setTarget("QPlainTextEdit")
      local expected = "QPlainTextEdit"
      local actual = stylesheet.target
      assert.equal(expected, actual)
    end)

    it("Should clear the target if no target is passed in", function()
      stylesheet:setTarget("QPlainTextEdit") -- set it
      stylesheet:setTarget()                 -- and clear it
      assert.is_nil(stylesheet.target)
    end)

    it("Should throw an error if anything besides a string is passed in", function()
      local st = function()
        stylesheet:setTarget(4)
      end
      assert.error_matches(st, "bad argument #1 type")
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:setCSS", function()
    local stylesheet
    before_each(function()
      stylesheet = Geyser.StyleSheet:new("")
    end)

    it("Should configure the stylesheet the same as passing the css to new", function()
      local sheet = [[
        background-color: black;
        color: green;
      ]]
      local stylesheet2 = Geyser.StyleSheet:new(sheet)
      stylesheet:setCSS(sheet)
      local expected = stylesheet2:getStyleTable()
      local actual = stylesheet:getStyleTable()
      assert.are.same(expected, actual)
    end)

    it("Should configure the target if it's included in the css", function()
      local sheet = [[QLabel {
        background-color: black;
        color: green;
      }]]
      stylesheet:setCSS(sheet)
      local expected = "QLabel"
      local actual = stylesheet.target
      assert.equal(expected, actual)
    end)

    it("Should clear the stylesheet if passed an empty string", function()
      local sheet = [[
        background-color: black;
        color: green;
      ]]
      stylesheet:setCSS(sheet) -- give it something to clear
      stylesheet:setCSS()      -- and clear it
      local expected = {}
      local actual = stylesheet:getStyleTable()
      assert.are.same(expected, actual)
    end)

    it("Should throw an error if passed something besides a string", function()
      local sc = function()
        stylesheet:setCSS(5)
      end
      assert.error_matches(sc, "bad argument #1 type")
    end)
  end)

  describe("Tests the functionality of Geyser.StyleSheet:getCSS", function()
    local parent, child
    before_each(function()
      parent = Geyser.StyleSheet:new([[
        background-color: black;
        color: green;
      ]])
      child = Geyser.StyleSheet:new([[
        color: blue;
        font: "Ubuntu Mono";
      ]], parent)
    end)

    it("Should assemble the string stylesheet", function()
      local expected = "background-color: black;\ncolor: green;\n"
      local actual = parent:getCSS()
      assert.equal(expected, actual)
    end)

    it("Should include values inherited from the parent by default", function()
      local expected = 'background-color: black;\ncolor: blue;\nfont: "Ubuntu Mono";\n'
      local actual = child:getCSS()
      assert.equal(expected, actual)
    end)

    it("Should ignore values inherited from the parent if passed false as a parameter", function()
      local expected = 'color: blue;\nfont: "Ubuntu Mono";\n'
      local actual = child:getCSS(false)
      assert.equal(expected, actual)
    end)

    it("Should wrap it in 'target { }' if a target is set", function()
      child:setTarget("QLabel")
      local expected = 'QLabel {\nbackground-color: black;\ncolor: blue;\nfont: "Ubuntu Mono";\n}'
      local actual = child:getCSS()
      assert.equal(expected, actual)
    end)
  end)
end)