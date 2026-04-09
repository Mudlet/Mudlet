-- Test script for small area viewport clamping
-- Creates a small area and walks through edge rooms to demonstrate the behavior

-- Clean up any previous test
if getAreaTable()["Clamp Test"] then
  deleteArea("Clamp Test")
end

local areaId = addAreaName("Clamp Test")

-- Create a 7x7 grid of rooms, offset so the area is NOT centered on (0,0)
-- This makes the clamping effect visible
local rooms = {}
local id = 50000 -- start from a high ID to avoid conflicts
for x = 0, 6 do
  rooms[x] = {}
  for y = 0, 6 do
    id = id + 1
    rooms[x][y] = id
    addRoom(id)
    setRoomArea(id, areaId)
    setRoomCoordinates(id, x + 10, y + 10, 0) -- offset to (10-16, 10-16)
  end
end

-- Connect rooms with exits
for x = 0, 6 do
  for y = 0, 6 do
    if x < 6 then setExit(rooms[x][y], rooms[x+1][y], 4) end -- east
    if x > 0 then setExit(rooms[x][y], rooms[x-1][y], 5) end -- west
    if y < 6 then setExit(rooms[x][y], rooms[x][y+1], 1) end -- north
    if y > 0 then setExit(rooms[x][y], rooms[x][y-1], 6) end -- south
  end
end

-- Label corner rooms
setRoomChar(rooms[0][0], "SW")
setRoomChar(rooms[6][0], "SE")
setRoomChar(rooms[0][6], "NW")
setRoomChar(rooms[6][6], "NE")
setRoomChar(rooms[3][3], "C")

-- Set up the walk path: center -> corners -> center
-- This shows clamping at each edge
local path = {
  {3, 3, "Starting at CENTER"},
  {0, 0, "Moving to SW corner - watch: player stays near center, area doesn't scroll off"},
  {6, 0, "Moving to SE corner"},
  {6, 6, "Moving to NE corner"},
  {0, 6, "Moving to NW corner"},
  {3, 3, "Back to CENTER"},
  {0, 3, "West edge"},
  {6, 3, "East edge"},
  {3, 0, "South edge"},
  {3, 6, "North edge"},
  {3, 3, "Done! Back to center"},
}

-- Start the walk
centerview(rooms[3][3])

local delay = 0
local step = 2 -- seconds between moves

for i, p in ipairs(path) do
  tempTimer(delay, function()
    local x, y, label = p[1], p[2], p[3]
    centerview(rooms[x][y])
    echo("\n--- Step " .. i .. ": " .. label .. " (room at " .. (x+10) .. "," .. (y+10) .. ") ---\n")
  end)
  delay = delay + step
end

echo("\n\n=== Small Area Clamp Test ===\n")
echo("Watch the mapper - the player walks through a 7x7 grid.\n")
echo("At corners/edges, the viewport should clamp to keep all rooms visible\n")
echo("while keeping the player as close to center as possible.\n")
echo("Steps every " .. step .. "s, total " .. delay .. "s\n\n")
