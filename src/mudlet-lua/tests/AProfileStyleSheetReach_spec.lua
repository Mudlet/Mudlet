-- Named to sort early (busted runs its files in sorted order), after AFreshProfile_spec.lua:
-- every profile style sheet change repolishes each live widget, which takes seconds once the
-- rest of the suite has left its windows and labels behind
describe("Application and profile style sheets reach what is inside", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))

  teardown(function()
    setProfileStyleSheet("")
  end)

  -- A label's size hint follows its font, so labels with the same text and font agree on it
  local function hint(label)
    return table.concat({getLabelSizeHint(label)}, "x")
  end

  local function sheetWindows(tag)
    local window = "uiSheet" .. tag .. suffix
    local labels = {own = window .. "Own", main = window .. "Main", window = window .. "Inside"}
    openUserWindow(window, false, false)
    createLabel(labels.own, 0, 0, 100, 20, 1)
    createLabel(labels.main, 0, 30, 100, 20, 1)
    createLabel(window, labels.window, 0, 0, 100, 20, 1)
    for _, label in pairs(labels) do
      echo(label, "label text")
    end
    -- Busted keeps one finally per test, so this one is the test's
    finally(function()
      setProfileStyleSheet("")
      setUserWindowStyleSheet(window, "")
      for _, label in pairs(labels) do
        deleteLabel(label)
      end
      deleteMiniConsole(window .. "Other")
      deleteMiniConsole(window .. "Mini")
      deleteMiniConsole(window .. "MiniInside")
      deleteMiniConsole(window .. "MiniLater")
      deleteMiniConsole(window .. "MiniKept")
      deleteMiniConsole(window .. "MiniMoved")
      deleteMiniConsole(window .. "BoxMini")
      deleteLabel(window .. "BoxLabel")
      deleteScrollBox(window .. "Box")
      deleteMiniConsole(window)
    end)
    return window, labels
  end

  it("gives the labels on the main console and in user windows its font, text and sizes", function()
    local _, labels = sheetWindows("Labels")
    assert.is_true(setLabelStyleSheet(labels.own, "QLabel { font-size: 23pt; }"))
    local big = hint(labels.own)
    assert.is_true(setProfileStyleSheet("QLabel { font-size: 23pt; }"))
    assert.are.equal(big, hint(labels.main))
    assert.are.equal(big, hint(labels.window))

    -- a label's own sheet wins over the one it inherits, whatever their selectors
    assert.is_true(setLabelStyleSheet(labels.own, "QLabel { font-size: 7pt; }"))
    local small = hint(labels.own)
    assert.is_true(setProfileStyleSheet("QPushButton { font-size: 5pt; } TLabel, QLabel:enabled { font-size: 23pt; }"))
    assert.are.equal(small, hint(labels.own))
    assert.are.equal(big, hint(labels.main))
    assert.are.equal(big, hint(labels.window))

    assert.is_true(setProfileStyleSheet("QLabel { min-width: 311px; min-height: 87px; }"))
    assert.are.equal("311x87", hint(labels.main))
    assert.are.equal("311x87", hint(labels.window))

    assert.is_true(setProfileStyleSheet('QLabel { qproperty-text: "from the profile"; }'))
    for _, label in pairs(labels) do
      assert.are.equal("from the profile", getLabelText(label))
    end
    echo(labels.main, "echoed since")
    assert.is_true(setProfileStyleSheet(""))
    assert.are.equal("echoed since", getLabelText(labels.main))
    assert.are.equal("from the profile", getLabelText(labels.window))
    assert.are_not.equal(big, hint(labels.window))
  end)

  it("gives a label made under it the same as one made before", function()
    local window, labels = sheetWindows("Made")
    assert.is_true(setProfileStyleSheet("QLabel { font-size: 23pt; }"))
    local big = hint(labels.main)
    for _, where in ipairs({"main", window}) do
      local late = window .. "Late" .. where
      createLabel(where, late, 0, 60, 100, 20, 1)
      echo(late, "label text")
      local made = hint(late)
      deleteLabel(late)
      assert.are.equal(big, made)
    end
  end)

  it("leaves the main console's labels alone when only a user window is given a sheet", function()
    local window, labels = sheetWindows("OneWindow")
    assert.is_true(setLabelStyleSheet(labels.own, "QLabel { font-size: 23pt; }"))
    local big = hint(labels.own)
    assert.is_true(setUserWindowStyleSheet(window, 'QLabel { font-size: 23pt; qproperty-text: "label text"; }'))
    assert.are.equal(big, hint(labels.window))
    assert.are_not.equal(big, hint(labels.main))
  end)

  it("gives the mini consoles on the main console and in user windows the font of their text", function()
    local window = sheetWindows("Minis")
    local mini, inside, later = window .. "Mini", window .. "MiniInside", window .. "MiniLater"
    createMiniConsole(mini, 0, 60, 200, 100)
    createMiniConsole(window, inside, 0, 30, 200, 100)
    setMiniConsoleFontSize(mini, 8)
    setMiniConsoleFontSize(inside, 8)
    assert.is_true(setProfileStyleSheet("QLabel { font-size: 23pt; } TConsole { font-size: 19pt; }"))
    assert.are.same({8, 8}, {getFontSize(mini), getFontSize(inside)})
    assert.is_true(setProfileStyleSheet("TTextEdit { font-size: 21pt; }"))
    assert.are.same({21, 21}, {getFontSize(mini), getFontSize(inside)})
    -- the sheet's font stays over one a script then sets, which shows once the sheet has gone
    setMiniConsoleFontSize(mini, 11)
    assert.are.equal(21, getFontSize(mini))
    createMiniConsole(later, 0, 170, 200, 100)
    assert.are.equal(21, getFontSize(later))
    assert.is_true(setProfileStyleSheet(""))
    assert.are.same({11, 8, 12}, {getFontSize(mini), getFontSize(inside), getFontSize(later)})
    assert.is_true(setUserWindowStyleSheet(window, "font-size: 17pt;"))
    assert.are.same({11, 17}, {getFontSize(mini), getFontSize(inside)})
    -- a sheet that styles the console as well as its text leaves its font on one made under it
    local kept = window .. "MiniKept"
    createMiniConsole(window, kept, 0, 140, 200, 100)
    assert.is_true(setUserWindowStyleSheet(window, ""))
    assert.are.same({8, 17}, {getFontSize(inside), getFontSize(kept)})
  end)

  it("gives a mini console moved into a user window that window's font, and takes it back as it leaves", function()
    local window = sheetWindows("Moved")
    local other, moved = window .. "Other", window .. "MiniMoved"
    openUserWindow(other, false, false)
    createMiniConsole(moved, 0, 60, 200, 100)
    setMiniConsoleFontSize(moved, 8)
    assert.is_true(setUserWindowStyleSheet(window, "TTextEdit { font-size: 20pt; }"))
    local sizes = {getFontSize(moved)}
    for _, where in ipairs({window, other, window, "main"}) do
      setWindow(where, moved, 0, 0, true)
      sizes[#sizes + 1] = getFontSize(moved)
    end
    assert.are.same({8, 20, 8, 20, 8}, sizes)
  end)

  it("restyles a label moved into a user window with a sheet, and again as it leaves", function()
    local window, labels = sheetWindows("MovedLabel")
    local sheet = 'QLabel { font-size: 23pt; qproperty-text: "from the window"; }'
    assert.is_true(setLabelStyleSheet(labels.own, sheet))
    local big = hint(labels.own)
    local plain = hint(labels.main)
    assert.is_true(setUserWindowStyleSheet(window, sheet))
    assert.are.equal(plain, hint(labels.main))
    setWindow(window, labels.main, 0, 0, true)
    assert.are.equal(big, hint(labels.main))
    assert.are.equal("from the window", getLabelText(labels.main))
    setWindow("main", labels.main, 0, 0, true)
    assert.are_not.equal(big, hint(labels.main))
  end)

  it("restyles what a scroll box holds as the box moves into a user window with a sheet", function()
    local window, labels = sheetWindows("MovedBox")
    local box, label, mini = window .. "Box", window .. "BoxLabel", window .. "BoxMini"
    local sheet = 'QLabel { font-size: 23pt; qproperty-text: "from the window"; } TTextEdit { font-size: 20pt; }'
    assert.is_true(setLabelStyleSheet(labels.own, sheet))
    local big = hint(labels.own)
    createScrollBox("main", box, 0, 60, 300, 200)
    createLabel(box, label, 0, 0, 100, 20, 1)
    echo(label, "label text")
    createMiniConsole(box, mini, 0, 30, 200, 100)
    setMiniConsoleFontSize(mini, 8)
    assert.is_true(setUserWindowStyleSheet(window, sheet))
    setWindow(window, box, 0, 0, true)
    assert.are.same({big, "from the window", 20}, {hint(label), getLabelText(label), getFontSize(mini)})
    setWindow("main", box, 0, 60, true)
    assert.are_not.equal(big, hint(label))
    assert.are.equal(8, getFontSize(mini))
  end)

  it("leaves none of the windows the tests above made", function()
    for _, tag in ipairs({"Moved", "MovedLabel", "MovedBox"}) do
      local window = "uiSheet" .. tag .. suffix
      for _, name in ipairs({window, window .. "Other", window .. "MiniMoved", window .. "Own", window .. "Main", window .. "Inside", window .. "Box", window .. "BoxLabel", window .. "BoxMini"}) do
        assert.is_nil(windowType(name), name)
      end
    end
  end)
end)
