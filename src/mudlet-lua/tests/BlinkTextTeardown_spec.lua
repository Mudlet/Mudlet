describe("Deleting the last console showing blinking text", function()
  -- The deferred delete only runs inside pumpEvents() when it was posted from that loop
  it("leaves the console's freed buffer alone while its panes are destroyed", function()
    if not os.getenv("MUDLET_TEST_MODE") then
      pending("needs pumpEvents(), which only test mode offers")
      return
    end
    local name = "blinkTextTeardown"
    local wasEnabled = getConfig("enableBlinkText")
    setConfig("enableBlinkText", true)
    -- Blinking text left on the main window by another spec would keep the blink timer running
    clearWindow()
    createMiniConsole(name, 0, 0, 400, 200)
    setTextFormat(name, 192, 192, 192, 0, 0, 0, false, false, false, false, false, false, "slow")
    echo(name, "This one blinks\n")
    showWindow(name)
    pumpEvents(300)

    tempTimer(0, function() deleteMiniConsole(name) end)
    pumpEvents(300)
    setConfig("enableBlinkText", wasEnabled)

    assert.is_nil(windowType(name))
  end)
end)
