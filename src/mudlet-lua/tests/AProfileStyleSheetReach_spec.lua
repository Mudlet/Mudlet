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
      deleteCommandLine(window .. "PlainLine")
      deleteCommandLine(window .. "Line")
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

  it("reaches a widget by what it is inside, as Qt's selectors match", function()
    local window, labels = sheetWindows("Selectors")
    local profile = getProfileName()
    assert.is_true(setLabelStyleSheet(labels.own, "QLabel { font-size: 23pt; }"))
    local big = hint(labels.own)
    local plain = hint(labels.main)
    local function reach(sheet)
      assert.is_true(setProfileStyleSheet(sheet))
      local reached = {}
      for _, where in ipairs({"main", "window"}) do
        reached[#reached + 1] = hint(labels[where]) == big and where or "-"
      end
      assert.is_true(setProfileStyleSheet(""))
      assert.are.equal(plain, hint(labels.main))
      return table.concat(reached, " ")
    end
    assert.are.equal("main -", reach("TMainConsole QLabel { font-size: 23pt; }"))
    assert.are.equal("main -", reach(('TMainConsole[objectName="%s"] QLabel { font-size: 23pt; }'):format(profile)))
    assert.are.equal("main -", reach("#MainFrame > QLabel { font-size: 23pt; }"))
    assert.are.equal("- window", reach("QDockWidget > TConsole > QLabel { font-size: 23pt; }"))
    assert.are.equal("- window", reach(('[objectName="dockWindow_%s_%s"] QLabel { font-size: 23pt; }'):format(profile, window)))
    assert.are.equal("main window", reach("QMainWindow QLabel { font-size: 23pt; }"))
    -- an ancestor's states are not looked at, only the widget's own
    assert.are.equal("- window", reach("QDockWidget:hover QLabel { font-size: 23pt; }"))
    assert.are.equal("- -", reach("TConsole QLabel:hover { font-size: 23pt; }"))
    -- a selector is matched from the widget out and takes the first ancestor that fits, never trying a further one
    assert.are.equal("- -", reach("QMainWindow > QSplitter QLabel { font-size: 23pt; }"))
    assert.are.equal("- -", reach("TMainConsole > QWidget QLabel { font-size: 23pt; }"))
    assert.are.equal("- -", reach("QLabel QLabel { font-size: 23pt; }"))
    assert.are.equal("- -", reach("QLabel + QLabel { font-size: 23pt; }"))
    -- the more specific rule wins, whatever its order
    assert.are.equal("main window", reach("TConsole QLabel { font-size: 23pt; } QLabel { font-size: 7pt; }"))
    -- Qt stops reading a sheet at a selector it cannot parse, keeping the rules before it
    assert.are.equal("main -", reach("TMainConsole QLabel { font-size: 23pt; } QLabel:nth-child(1) { color: red; } QDockWidget QLabel { font-size: 23pt; }"))
    assert.are.equal("main -", reach("TMainConsole QLabel { font-size: 23pt; } QLabel, 1abc { color: red; } QDockWidget QLabel { font-size: 23pt; }"))
    -- but reads a function of a name, which styles a sub-control and never the widget
    assert.are.equal("main window", reach("TMainConsole QLabel { font-size: 23pt; } QLabel:nth-child(odd) { color: red; } QDockWidget QLabel { font-size: 23pt; }"))
    assert.are.equal("- -", reach("QLabel:nth-child(hover) { font-size: 23pt; }"))
  end)

  it("reaches a widget by its properties", function()
    local window, labels = sheetWindows("Properties")
    local profile = getProfileName()
    assert.is_true(setLabelStyleSheet(labels.own, "QLabel { font-size: 23pt; }"))
    local big = hint(labels.own)
    local function reached(sheet)
      assert.is_true(setProfileStyleSheet(sheet))
      local answer = {hint(labels.main) == big, hint(labels.window) == big}
      assert.is_true(setProfileStyleSheet(""))
      return answer
    end
    assert.are.same({true, true}, reached('QLabel[objectName^="label_"] { font-size: 23pt; }'))
    assert.are.same({true, false}, reached(('QLabel[objectName="label_%s_%s"] { font-size: 23pt; }'):format(profile, labels.main)))
    assert.are.same({true, true}, reached('QLabel[class="TLabel"], QLabel[class~=TLabel] { font-size: 23pt; }'))
    assert.are.same({true, true}, reached('QLabel[text="label text"] { font-size: 23pt; }'))
    assert.are.same({true, true}, reached("QLabel[text] { font-size: 23pt; }"))
    assert.are.same({false, false}, reached("QLabel[text=\"other text\"], QLabel[nosuch] { font-size: 23pt; }"))
    assert.are.same({true, false}, reached(('[HostName="%s"] QLabel { font-size: 23pt; }'):format(profile)))
    assert.are.same({false, true}, reached('QDockWidget[floating="false"] QLabel { font-size: 23pt; }'))
    assert.is_true(setUserWindowStyleSheet(window, 'QMainWindow > QDockWidget[floating="false"] { qproperty-windowTitle: "by selector"; }'))
    assert.are.equal("by selector", getUserWindowTitle(window))
    -- A label emptied by a script has text, which only one never given any lacks
    echo(labels.main, "")
    local empty = hint(labels.main)
    assert.is_true(setProfileStyleSheet('QLabel[text=""] { min-width: 311px; }'))
    assert.are_not.equal(empty, hint(labels.main))
    assert.is_true(setProfileStyleSheet(""))
  end)

  it("checks a styled label's text again when its window's sheet is applied again", function()
    local window, labels = sheetWindows("Reapplied")
    local sheet = 'QLabel[text="ready"] { min-width: 311px; }'
    for _, label in ipairs({labels.main, labels.window}) do
      assert.is_true(setLabelStyleSheet(label, "QLabel { color: red; }"))
      echo(label, "waiting")
    end
    assert.is_true(setProfileStyleSheet(sheet))
    assert.is_true(setUserWindowStyleSheet(window, sheet))
    echo(labels.main, "ready")
    echo(labels.window, "ready")
    assert.is_true(setProfileStyleSheet(sheet))
    assert.is_true(setUserWindowStyleSheet(window, sheet))
    assert.is_true(getLabelSizeHint(labels.main) >= 311)
    assert.is_true(getLabelSizeHint(labels.window) >= 311)
  end)

  it("reaches the text of a mini console and a command line by what they are inside", function()
    local window = sheetWindows("Panes")
    local mini, inside = window .. "Mini", window .. "MiniInside"
    createMiniConsole(mini, 0, 60, 200, 100)
    createMiniConsole(window, inside, 0, 30, 200, 100)
    setMiniConsoleFontSize(mini, 8)
    setMiniConsoleFontSize(inside, 8)
    assert.is_true(setProfileStyleSheet("#MainFrame > #MainDisplay TTextEdit { font-size: 21pt; }"))
    assert.are.same({21, 21}, {getFontSize(mini), getFontSize(inside)})
    assert.is_true(setProfileStyleSheet("QDockWidget TTextEdit { font-size: 21pt; }"))
    assert.are.same({8, 21}, {getFontSize(mini), getFontSize(inside)})
    assert.is_true(setProfileStyleSheet("#" .. mini .. " TTextEdit { font-size: 21pt; }"))
    assert.are.same({21, 8}, {getFontSize(mini), getFontSize(inside)})
    assert.is_true(setProfileStyleSheet(""))
    local plainLine, line = window .. "PlainLine", window .. "Line"
    createCommandLine(window, plainLine, 0, 0, 100, 20)
    createCommandLine(window, line, 0, 30, 100, 20)
    assert.is_true(setCmdLineStyleSheet(plainLine, "TCommandLine { min-height: 80px; }"))
    assert.is_true(setCmdLineStyleSheet(line, "QMainWindow > QDockWidget > TConsole > QPlainTextEdit { min-height: 80px; }"))
    local tall = select(4, getWindowGeometry(plainLine))
    assert.are_not.equal(20, tall)
    assert.are.equal(tall, select(4, getWindowGeometry(line)))
  end)

  it("restyles a label moved into a scroll box that a sheet names", function()
    local window, labels = sheetWindows("IntoBox")
    local box = window .. "Box"
    assert.is_true(setLabelStyleSheet(labels.own, "QLabel { font-size: 23pt; }"))
    local big = hint(labels.own)
    createScrollBox("main", box, 0, 60, 300, 200)
    assert.is_true(setProfileStyleSheet("TScrollBox QLabel { font-size: 23pt; }"))
    assert.are_not.equal(big, hint(labels.main))
    setWindow(box, labels.main, 0, 0, true)
    assert.are.equal(big, hint(labels.main))
    setWindow("main", labels.main, 0, 0, true)
    assert.are_not.equal(big, hint(labels.main))
  end)

  it("restyles the text of a mini console and a command line moved into and out of a scroll box that a sheet names", function()
    local window = sheetWindows("PaneIntoBox")
    local box, mini, line = window .. "Box", window .. "BoxMini", window .. "PlainLine"
    createScrollBox("main", box, 0, 60, 300, 200)
    createMiniConsole(mini, 0, 300, 200, 100)
    setMiniConsoleFontSize(mini, 8)
    assert.is_true(setProfileStyleSheet("TScrollBox TTextEdit { font-size: 21pt; }"))
    assert.are.equal(8, getFontSize(mini))
    setWindow(box, mini, 0, 0, true)
    assert.are.equal(21, getFontSize(mini))
    setWindow("main", mini, 0, 300, true)
    assert.are.equal(8, getFontSize(mini))
    assert.is_true(setProfileStyleSheet(""))
    createCommandLine(line, 0, 420, 100, 20)
    assert.is_true(setCmdLineStyleSheet(line, "TScrollBox TCommandLine { min-height: 80px; }"))
    assert.are.equal(20, select(4, getWindowGeometry(line)))
    setWindow(box, line, 0, 0, true)
    assert.are_not.equal(20, select(4, getWindowGeometry(line)))
    -- Leaving the box lifts the minimum, though not the height it gave
    setWindow("main", line, 0, 420, true)
    resizeWindow(line, 100, 20)
    assert.are.equal(20, select(4, getWindowGeometry(line)))
  end)

  it("gives the main console's text the font the sheet names for it, and keeps one that styled the console", function()
    local original = getFontSize("main")
    finally(function()
      setProfileStyleSheet("")
      -- by way of another size, as a size the console already has changes nothing
      setFontSize("main", original + 1)
      setFontSize("main", original)
    end)
    local function sizes(sheet)
      setFontSize("main", 12)
      assert.is_true(setProfileStyleSheet(sheet))
      local seen = {getFontSize("main")}
      setFontSize("main", 9)
      seen[#seen + 1] = getFontSize("main")
      assert.is_true(setProfileStyleSheet(""))
      seen[#seen + 1] = getFontSize("main")
      setFontSize("main", 12)
      seen[#seen + 1] = getFontSize("main")
      return seen
    end
    assert.are.same({20, 20, 9, 12}, sizes("TMainConsole TTextEdit { font-size: 20pt; }"))
    -- a sheet that styles the console gives its font to the text with each font a script sets
    assert.are.same({20, 20, 20, 12}, sizes("QWidget { font-size: 20pt; }"))
    assert.are.same({12, 20, 20, 12}, sizes("TConsole { font-size: 20pt; }"))
    assert.are.same({12, 9, 9, 12}, sizes("QLabel { font-size: 20pt; }"))
  end)

  it("gives a mini console the font of a sheet that styles the console, over one a script sets", function()
    local window = sheetWindows("ConsoleFont")
    local mini, inside = window .. "Mini", window .. "MiniInside"
    assert.is_true(setProfileStyleSheet("TConsole { font-size: 21pt; }"))
    assert.is_true(setUserWindowStyleSheet(window, "QWidget { font-size: 21pt; }"))
    createMiniConsole(mini, 0, 60, 200, 100)
    createMiniConsole(window, inside, 0, 30, 200, 100)
    assert.are.same({21, 21}, {getFontSize(mini), getFontSize(inside)})
    setMiniConsoleFontSize(mini, 9)
    setMiniConsoleFontSize(inside, 9)
    assert.are.same({21, 21}, {getFontSize(mini), getFontSize(inside)})
    assert.is_true(setProfileStyleSheet(""))
    assert.is_true(setUserWindowStyleSheet(window, ""))
    assert.are.same({21, 21}, {getFontSize(mini), getFontSize(inside)})
    -- TConsole::setFontSize() compares with the size scripts gave, which the sheet only lay over
    setMiniConsoleFontSize(mini, 9)
    setMiniConsoleFontSize(inside, 9)
    assert.are.same({21, 21}, {getFontSize(mini), getFontSize(inside)})
    setMiniConsoleFontSize(mini, 10)
    setMiniConsoleFontSize(inside, 10)
    assert.are.same({10, 10}, {getFontSize(mini), getFontSize(inside)})
    -- A new family takes the size of the console's font, sheet and all
    assert.is_true(setProfileStyleSheet("TConsole { font-size: 21pt; }"))
    setFont(mini, getFont(mini))
    assert.is_true(setProfileStyleSheet(""))
    assert.are.equal(21, getFontSize(mini))
  end)

  it("leaves none of the windows the tests above made", function()
    for _, tag in ipairs({"Moved", "MovedLabel", "MovedBox", "Selectors", "Properties", "Panes", "IntoBox", "PaneIntoBox", "ConsoleFont"}) do
      local window = "uiSheet" .. tag .. suffix
      for _, name in ipairs({window, window .. "Other", window .. "MiniMoved", window .. "Own", window .. "Main", window .. "Inside", window .. "Box", window .. "BoxLabel", window .. "BoxMini", window .. "Mini", window .. "MiniInside", window .. "PlainLine", window .. "Line"}) do
        assert.is_nil(windowType(name), name)
      end
    end
  end)
end)
