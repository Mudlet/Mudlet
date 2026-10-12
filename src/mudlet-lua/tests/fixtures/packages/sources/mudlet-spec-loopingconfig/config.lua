mpackage = [[mudlet-spec-loopingconfig-renamed]]
author = [[Mudlet test suite]]
title = [[Fixture whose config.lua never finishes, for Package_spec.lua]]
version = [[3.5]]
-- never finishes, and catches whatever is raised inside the inner loop
while true do
  pcall(function()
    while true do
    end
  end)
end
