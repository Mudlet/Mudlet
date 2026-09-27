describe("issue 3334 isolate", function()
  it("sweeps", function()
    local out = {}
    local function p(s) out[#out+1] = s end
    local function run(label, scale)
      assert.is_true(loadMap("/home/vadi/.claude/jobs/3ae74264/tmp/sendar.dat"))
      local ids = {}
      for id in pairs(getRooms()) do ids[#ids+1] = id; if getRoomWeight(id) ~= 1 then setRoomWeight(id, 1) end end
      table.sort(ids)
      if scale then
        for _, id in ipairs(ids) do local x, y, z = getRoomCoordinates(id); setRoomCoordinates(id, math.floor(x / scale), math.floor(y / scale), math.floor(z / scale)) end
      end
      local adj = {}
      for _, id in ipairs(ids) do
        local n = {}
        local ew = getExitWeights(id) or {}
        local function add(dir, to) if roomExists(to) and not roomLocked(to) then local c = ew[dir] or getRoomWeight(to); if not n[to] or c < n[to] then n[to] = c end end end
        for dir, to in pairs(getRoomExits(id) or {}) do add(dir, to) end
        for cmd, to in pairs(getSpecialExitsSwap(id) or {}) do add(cmd, to) end
        adj[id] = n
      end
      local total, worse, bad = 0, 0, 0
      for _, from in ipairs(ids) do
        local dist, done = {[from] = 0}, {}
        while true do
          local best, bd
          for id, d in pairs(dist) do if not done[id] and (not bd or d < bd) then best, bd = id, d end end
          if not best then break end
          done[best] = true
          for to, c in pairs(adj[best]) do if not dist[to] or bd + c < dist[to] then dist[to] = bd + c end end
        end
        for _, to in ipairs(ids) do
          if to ~= from and dist[to] then
            total = total + 1
            if getPath(from, to) then
              local s = 0
              for _, w in ipairs(speedWalkWeight) do s = s + tonumber(w) end
              if s > dist[to] then worse = worse + 1 elseif s < dist[to] then bad = bad + 1 end
            else
              worse = worse + 1
            end
          end
        end
      end
      getPath(2, 183); local a = #speedWalkDir
      getPath(183, 2); local b = #speedWalkDir
      p(string.format("%s: getPath(2,183)=%d steps, getPath(183,2)=%d steps; sweep %d pairs, %d suboptimal (%.1f%%), %d reference mismatches", label, a, b, total, worse, 100 * worse / total, bad))
    end
    run("weights reset to 1, original coords", nil)
    run("weights reset to 1, coords divided by 6", 6)
    local f = io.open("/home/vadi/.claude/jobs/3ae74264/tmp/result3.txt", "w")
    f:write(table.concat(out, "\n"), "\n")
    f:close()
  end)
end)
