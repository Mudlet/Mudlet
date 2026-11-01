-- sysConsoleSizeChanged names a console and its columns and rows, but only
-- once its pane has been resized to a grid it has not reported yet: a font
-- change alone reports nothing, and a pane resized while hidden reports as it
-- shows. With no main window the null view keeps the same rules.
describe("Tests sysConsoleSizeChanged for a mini console", function()
  local suffix = ("-%d-%d"):format(os.time(), math.random(100000))
  local reports, handler

  before_each(function()
    reports = {}
    handler = registerAnonymousEventHandler("sysConsoleSizeChanged", function(_, name, ...)
      if name:find(suffix, 1, true) then
        reports[#reports + 1] = {name, ...}
      end
    end)
  end)

  after_each(function()
    killAnonymousEventHandler(handler)
  end)

  local function grid(name)
    return {name, getColumnCount(name), getRowCount(name), 0}
  end

  local function made(base)
    local name = base .. suffix
    assert.is_true(createMiniConsole(name, 0, 0, 300, 200))
    return name
  end

  it("reports its size as it is made and as it is resized", function()
    local name = made("specSizeMade")
    finally(function() deleteMiniConsole(name) end)
    assert.are.same({grid(name)}, reports)

    assert.is_true(resizeWindow(name, 400, 250))
    assert.are.same({grid(name)}, {reports[2]})
    assert.are.equal(2, #reports)
  end)

  it("reports nothing for a font change, or for a resize to the size it has", function()
    local name = made("specSizeFont")
    finally(function() deleteMiniConsole(name) end)
    reports = {}
    local rows = getRowCount(name)

    assert.is_true(setFontSize(name, getFontSize(name) + 6))
    assert.is_true(getRowCount(name) < rows)
    assert.is_true(resizeWindow(name, 300, 200))
    assert.are.same({}, reports)
  end)

  it("reports a resize made while it was hidden once it shows", function()
    local name = made("specSizeHidden")
    finally(function() deleteMiniConsole(name) end)
    reports = {}

    hideWindow(name)
    assert.is_true(resizeWindow(name, 400, 250))
    assert.are.same({}, reports)
    showWindow(name)
    assert.are.same({grid(name)}, reports)

    -- one that ends at the size it had reports nothing as it shows
    hideWindow(name)
    resizeWindow(name, 200, 100)
    resizeWindow(name, 400, 250)
    showWindow(name)
    assert.are.equal(1, #reports)
  end)

  it("reports a resize made while it was hidden once setWindow() shows it", function()
    local name = made("specSizeSetWindow")
    finally(function() deleteMiniConsole(name) end)
    reports = {}

    hideWindow(name)
    assert.is_true(resizeWindow(name, 400, 250))
    assert.is_true(setWindow("main", name, 0, 0, true))
    assert.are.same({grid(name)}, reports)
    showWindow(name)
    assert.are.equal(1, #reports)
  end)

  it("reports a resize made while it was hidden even if its timestamps change before it shows", function()
    local name = made("specSizeHiddenTimeStamps")
    finally(function() deleteMiniConsole(name) end)
    reports = {}

    hideWindow(name)
    assert.is_true(resizeWindow(name, 400, 250))
    enableTimeStamps(name)
    disableTimeStamps(name)
    assert.are.same({}, reports)
    showWindow(name)
    assert.are.same({grid(name)}, reports)
  end)

  it("reports one in a hidden user window only once the window shows", function()
    local window = "specSizeHiddenWindow" .. suffix
    local name = "specSizeInHiddenWindow" .. suffix
    assert.is_true(openUserWindow(window, false, false))
    finally(function()
      deleteMiniConsole(name)
      closeUserWindow(window)
    end)
    hideWindow(window)
    reports = {}

    assert.is_true(createMiniConsole(window, name, 0, 0, 300, 200))
    assert.is_true(resizeWindow(name, 400, 250))
    assert.is_true(resizeWindow(name, 300, 200))
    local function of(console)
      local found = {}
      for _, report in ipairs(reports) do
        if report[1] == console then
          found[#found + 1] = report
        end
      end
      return found
    end
    assert.are.same({}, of(name))
    showWindow(window)
    assert.are.same({grid(name)}, of(name))
  end)

  it("keeps a user window's size right when a handler resizes it", function()
    local window = "specSizeSnapped" .. suffix
    assert.is_true(openUserWindow(window, false, false))
    local snapped, resizes = false, 0
    local snapper = registerAnonymousEventHandler("sysConsoleSizeChanged", function(_, name)
      if name == window and not snapped then
        snapped = true
        resizeWindow(window, 333, 222)
      end
    end)
    local counter = registerAnonymousEventHandler("sysUserWindowResizeEvent", function(_, _, _, name)
      if name == window then
        resizes = resizes + 1
      end
    end)
    finally(function()
      killAnonymousEventHandler(snapper)
      killAnonymousEventHandler(counter)
      closeUserWindow(window)
    end)

    resizeWindow(window, 400, 300)
    assert.is_true(snapped)
    resizes = 0
    resizeWindow(window, 400, 300)
    assert.is_true(resizes > 0)
  end)

  it("reports the rows its own command line takes and gives back", function()
    local name = made("specSizeCommandLine")
    finally(function()
      disableCommandLine(name)
      deleteMiniConsole(name)
    end)
    reports = {}

    assert.is_true(enableCommandLine(name))
    assert.are.same({grid(name)}, reports)
    assert.is_true(disableCommandLine(name))
    assert.are.same(grid(name), reports[2])
    assert.are.equal(2, #reports)
    assert.are_not.same(reports[1][3], reports[2][3])
  end)

  it("never reports a buffer, which is never shown", function()
    local name = "specSizeBuffer" .. suffix
    createBuffer(name)
    enableTimeStamps(name)
    disableTimeStamps(name)
    assert.are.same({}, reports)
  end)
end)
