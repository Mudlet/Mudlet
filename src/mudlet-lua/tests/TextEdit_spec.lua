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

    it("Should set font size", function()
      assert.is_true(setTextEditFontSize(name, 14))
    end)

    it("Should set tab moves focus", function()
      assert.is_true(setTextEditTabMovesFocus(name, true))
      assert.is_true(setTextEditTabMovesFocus(name, false))
    end)
  end)

  describe("Tests error handling", function()
    it("Should fail gracefully with non-existent text edit", function()
      local ok, err = getTextEditText("doesNotExist")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditText with non-existent text edit", function()
      local ok, err = setTextEditText("doesNotExist", "text")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail clearTextEdit with non-existent text edit", function()
      local ok, err = clearTextEdit("doesNotExist")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditReadOnly with non-existent text edit", function()
      local ok, err = setTextEditReadOnly("doesNotExist", true)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditPlaceholder with non-existent text edit", function()
      local ok, err = setTextEditPlaceholder("doesNotExist", "text")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditStyleSheet with non-existent text edit", function()
      local ok, err = setTextEditStyleSheet("doesNotExist", "css")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditFont with non-existent text edit", function()
      local ok, err = setTextEditFont("doesNotExist", "Arial")
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditFontSize with non-existent text edit", function()
      local ok, err = setTextEditFontSize("doesNotExist", 12)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should fail setTextEditTabMovesFocus with non-existent text edit", function()
      local ok, err = setTextEditTabMovesFocus("doesNotExist", true)
      assert.is_nil(ok)
      assert.is_string(err)
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

  describe("Tests Geyser.TextEdit wrapper", function()
    local editor

    it("Should create a Geyser.TextEdit", function()
      editor = Geyser.TextEdit:new({
        name = "testGeyserTE",
        x = 0, y = 0,
        width = 200, height = 100,
      })
      assert.is_table(editor)
      assert.are.equal("textedit", windowType("testGeyserTE"))
    end)

    it("Should set and get text via Geyser wrapper", function()
      editor:setText("Geyser text")
      assert.are.equal("Geyser text", editor:getText())
    end)

    it("Should clear via Geyser wrapper", function()
      editor:setText("some text")
      editor:clear()
      assert.are.equal("", editor:getText())
    end)

    it("Should set properties via Geyser wrapper", function()
      assert.has_no.errors(function() editor:setReadOnly(true) end)
      assert.has_no.errors(function() editor:setReadOnly(false) end)
      assert.has_no.errors(function() editor:setPlaceholder("placeholder") end)
      assert.has_no.errors(function() editor:setFontSize(14) end)
      assert.has_no.errors(function() editor:setFont("monospace") end)
      assert.has_no.errors(function() editor:setTabMovesFocus(true) end)
      assert.has_no.errors(function() editor:setStyleSheet("QPlainTextEdit { color: #eee; }") end)
    end)

    it("Should delete via Geyser wrapper", function()
      editor:delete()
      assert.is_nil(windowType("testGeyserTE"))
    end)
  end)
end)
