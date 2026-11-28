describe("Deleting the last console showing blinking text", function()
  -- Bites only under AddressSanitizer: the unfixed pane reads its miniconsole's freed buffer
  it("leaves the console's freed buffer alone while its panes are destroyed", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("needs pumpEvents(), which only test mode offers")
      return
    end
    local name = "blinkTextTeardown"
    local wasEnabled = getConfig("enableBlinkText")
    finally(function()
      deleteMiniConsole(name)
      setConfig("enableBlinkText", wasEnabled)
    end)
    setConfig("enableBlinkText", true)
    -- Blinking text left on the main window by another spec would keep the blink timer running
    clearWindow()
    createMiniConsole(name, 0, 0, 400, 200)
    setTextFormat(name, 192, 192, 192, 0, 0, 0, false, false, false, false, false, false, "slow")
    echo(name, "This one blinks\n")
    showWindow(name)
    assert.is_true(pumpEvents(300))

    -- The deferred delete only runs inside pumpEvents() when it was posted from that loop
    local deleted
    tempTimer(0, function() deleted = deleteMiniConsole(name) end)
    assert.is_true(pumpEvents(300))
    assert.is_true(deleted)
  end)
end)
