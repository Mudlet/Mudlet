describe("Tests CursorShapes.lua", function()
  local labelName = "cursorShapesSpecLabel"

  setup(function()
    createLabel(labelName, 0, 0, 20, 20, 1)
    hideWindow(labelName)
  end)

  teardown(function()
    deleteLabel(labelName)
  end)

  describe("Tests the contract of mudlet.cursor", function()
    it("Should give every shape a value the C++ label layer accepts", function()
      -- the numbers are Qt::CursorShape and TMainConsole::setLabelCursor range
      -- checks them, so one copied in from a newer Qt looks usable but is refused
      local checked = 0
      for name, shape in pairs(mudlet.cursor) do
        checked = checked + 1
        assert.is_true(setLabelCursor(labelName, shape), ("mudlet.cursor.%s (%s) was refused"):format(name, tostring(shape)))
      end
      assert.is_true(checked >= 23, "every documented cursor shape should still be in the table")
    end)

    it("Should be refused for a shape outside the range the C++ layer knows", function()
      -- pairs with the assertion above: without it, a layer that accepted
      -- anything at all would make that loop pass no matter what is in the table
      local ok, err = setLabelCursor(labelName, 22)
      assert.is_nil(ok)
      assert.is_string(err)
      ok, err = setLabelCursor(labelName, -2)
      assert.is_nil(ok)
      assert.is_string(err)
    end)

    it("Should accept every shape by its name as well as by its number", function()
      -- nothing reads a label's cursor back, so this can only show the name was
      -- accepted, not which shape it landed on
      for name, shape in pairs(mudlet.cursor) do
        assert.is_true(setLabelCursor(labelName, name), ("the name %q was refused"):format(name))
        assert.equals("number", type(shape))
      end
    end)

    it("Should not give two names the same shape", function()
      local seen = {}
      for name, shape in pairs(mudlet.cursor) do
        assert.is_nil(seen[shape], ("%s and %s both map to shape %s"):format(tostring(seen[shape]), name, tostring(shape)))
        seen[shape] = name
      end
    end)

    it("Should reset through the value resetLabelCursor asks the C++ layer for", function()
      assert.equals(-1, mudlet.cursor.Reset)
      assert.is_true(setLabelCursor(labelName, "OpenHand"))
      assert.is_true(resetLabelCursor(labelName))
    end)
  end)
end)
