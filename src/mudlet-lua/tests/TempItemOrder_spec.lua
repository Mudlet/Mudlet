describe("Temporary items killed out of the order they were made in", function()
  local probe = "temp item order probe"

  it("leaves the surviving triggers firing in the order they were made", function()
    local fired = {}
    local ids = {}
    for i = 1, 6 do
      ids[i] = tempTrigger(probe, function() fired[#fired + 1] = i end)
    end
    killTrigger(ids[5])
    killTrigger(ids[2])
    killTrigger(ids[5])
    -- the line also frees the killed triggers
    feedTriggers(probe .. "\n")
    assert.are.same({1, 3, 4, 6}, fired)

    ids[7] = tempTrigger(probe, function() fired[#fired + 1] = 7 end)
    fired = {}
    feedTriggers(probe .. "\n")
    assert.are.same({1, 3, 4, 6, 7}, fired)

    for _, id in pairs(ids) do
      killTrigger(id)
    end
  end)

  it("leaves the surviving aliases running in the order they were made", function()
    local ran = {}
    local ids = {}
    for i = 1, 6 do
      ids[i] = tempAlias("^" .. probe .. "$", function() ran[#ran + 1] = i end)
    end
    killAlias(ids[1])
    killAlias(ids[4])
    killAlias(ids[1])
    expandAlias(probe, false)
    assert.are.same({2, 3, 5, 6}, ran)

    ids[7] = tempAlias("^" .. probe .. "$", function() ran[#ran + 1] = 7 end)
    ran = {}
    expandAlias(probe, false)
    assert.are.same({2, 3, 5, 6, 7}, ran)

    for _, id in pairs(ids) do
      killAlias(id)
    end
  end)
end)
