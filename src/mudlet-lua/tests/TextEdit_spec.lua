-- The Geyser.TextEdit wrapper around these is covered in GeyserTextEdit_spec.lua.
describe("Tests TextEdit widget functions", function()

  describe("Tests createTextEdit and deleteTextEdit", function()
    it("Should create a text edit", function()
      assert.is_true(createTextEdit("main", "testTextEdit", 0, 0, 200, 100))
    end)

    it("Should identify a text edit with windowType", function()
      createTextEdit("main", "testWindowTypeTE", 0, 0, 200, 100)
      assert.are.equal("textedit", windowType("testWindowTypeTE"))
      deleteTextEdit("testWindowTypeTE")
    end)

    it("Should not identify a non-existing text edit", function()
      assert.is_nil(windowType("fakeTextEdit"))
    end)

    it("Should delete a text edit", function()
      createTextEdit("main", "testDeleteTE", 0, 0, 200, 100)
      assert.is_true(deleteTextEdit("testDeleteTE"))
      assert.is_nil(windowType("testDeleteTE"))
    end)

    it("Should fail to delete a non-existent text edit", function()
      local success, err = deleteTextEdit("nonExistentTextEdit")
      assert.is_false(success)
      assert.is_string(err)
    end)

    teardown(function()
      deleteTextEdit("testTextEdit")
    end)
  end)

  describe("Tests text content functions", function()
    local name = "testContentTE"

    setup(function()
      createTextEdit("main", name, 0, 0, 200, 100)
    end)

    teardown(function()
      deleteTextEdit(name)
    end)

    it("Should set and get text", function()
      setTextEditText(name, "Hello World")
      assert.are.equal("Hello World", getTextEditText(name))
    end)

    it("Should handle multi-line text", function()
      setTextEditText(name, "Line 1\nLine 2\nLine 3")
      assert.are.equal("Line 1\nLine 2\nLine 3", getTextEditText(name))
    end)

    it("Should handle empty text", function()
      setTextEditText(name, "")
      assert.are.equal("", getTextEditText(name))
    end)

    it("Should clear text", function()
      setTextEditText(name, "some text")
      clearTextEdit(name)
      assert.are.equal("", getTextEditText(name))
    end)

    it("Should handle unicode text", function()
      setTextEditText(name, "Hello 世界 🌍")
      assert.are.equal("Hello 世界 🌍", getTextEditText(name))
    end)
  end)

  describe("Tests property functions", function()
    local name = "testPropsTE"

    setup(function()
      createTextEdit("main", name, 0, 0, 200, 100)
    end)

    teardown(function()
      deleteTextEdit(name)
    end)

    it("Should set read-only mode", function()
      assert.is_true(setTextEditReadOnly(name, true))
      assert.is_true(setTextEditReadOnly(name, false))
    end)

    it("Should set placeholder text", function()
      assert.is_true(setTextEditPlaceholder(name, "Type here..."))
    end)

    it("Should set stylesheet", function()
      assert.is_true(setTextEditStyleSheet(name, "QPlainTextEdit { background: #222; }"))
    end)

    it("Should set font", function()
      assert.is_true(setTextEditFont(name, "Bitstream Vera Sans Mono"))
    end)

    it("Should pass a font the database does not list through to Qt", function()
      -- the font database leaves out names the platform still resolves, such as
      -- the fontconfig aliases on Linux, so an unlisted name is not refused
      assert.is_true(setTextEditFont(name, "No Such Font At All"))
      -- Ubuntu Mono ships with Mudlet, so it is there on every platform
      assert.is_true(setTextEditFont(name, "Ubuntu Mono"))
    end)

    it("Should set font size", function()
      assert.is_true(setTextEditFontSize(name, 14))
    end)

    it("Should set tab moves focus", function()
      assert.is_true(setTextEditTabMovesFocus(name, true))
      assert.is_true(setTextEditTabMovesFocus(name, false))
    end)
  end)

  describe("Tests error handling", function()
    local calls = {
      getTextEditText = function(n) return getTextEditText(n) end,
      setTextEditText = function(n) return setTextEditText(n, "text") end,
      clearTextEdit = function(n) return clearTextEdit(n) end,
      setTextEditReadOnly = function(n) return setTextEditReadOnly(n, true) end,
      setTextEditPlaceholder = function(n) return setTextEditPlaceholder(n, "text") end,
      setTextEditStyleSheet = function(n) return setTextEditStyleSheet(n, "css") end,
      setTextEditFont = function(n) return setTextEditFont(n, "Arial") end,
      setTextEditFontSize = function(n) return setTextEditFontSize(n, 12) end,
      setTextEditTabMovesFocus = function(n) return setTextEditTabMovesFocus(n, true) end,
    }

    for functionName, call in pairs(calls) do
      it("Should fail " .. functionName .. " with non-existent text edit", function()
        local ok, err = call("doesNotExist")
        assert.is_nil(ok)
        assert.are.equal("text edit name 'doesNotExist' not found", err)
      end)
    end

    -- the other window kinds share the name space, and the functions for
    -- labels and plain windows are shaped like these, so a name that is only
    -- some other kind of window must not be taken for a text edit
    describe("with the name of another kind of window", function()
      local otherName = "teOtherKindOfWindow"

      after_each(function()
        deleteLabel(otherName)
        deleteScrollBox(otherName)
        deleteCommandLine(otherName)
      end)

      local kinds = {
        label = function() return createLabel(otherName, 0, 0, 50, 20, 1) end,
        ["scroll box"] = function() return createScrollBox(otherName, 0, 0, 50, 20) end,
        ["command line"] = function() return createCommandLine(otherName, 0, 0, 50, 20) end,
      }

      for kind, create in pairs(kinds) do
        it("Should refuse every text edit function for a " .. kind, function()
          assert.is_true(create())
          for functionName, call in pairs(calls) do
            local ok, err = call(otherName)
            assert.is_nil(ok, functionName)
            assert.are.equal(("text edit name '%s' not found"):format(otherName), err, functionName)
          end
        end)
      end
    end)

    it("Should not answer getFont or setFont for a text edit", function()
      local teName = "teNotAConsole"
      createTextEdit("main", teName, 0, 0, 50, 20)
      finally(function() deleteTextEdit(teName) end)
      local ok, err = getFont(teName)
      assert.is_nil(ok)
      assert.are.equal(('window "%s" not found'):format(teName), err)
      ok, err = setFont(teName, "Ubuntu Mono")
      assert.is_nil(ok)
      assert.are.equal(('window "%s" not found'):format(teName), err)
    end)
  end)

  describe("Tests window functions with text edit", function()
    local name = "testWindowTE"

    setup(function()
      createTextEdit("main", name, 0, 0, 200, 100)
    end)

    teardown(function()
      deleteTextEdit(name)
    end)

    it("Should show and hide", function()
      assert.has_no.errors(function() hideWindow(name) end)
      assert.is_true(showWindow(name))
    end)

    it("Should move", function()
      assert.has_no.errors(function() moveWindow(name, 50, 50) end)
    end)

    it("Should resize", function()
      assert.has_no.errors(function() resizeWindow(name, 300, 200) end)
    end)
  end)
end)
