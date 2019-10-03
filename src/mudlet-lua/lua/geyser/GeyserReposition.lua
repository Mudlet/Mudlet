--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
function GeyserReposition()
  for _, window in pairs(Geyser.windowList) do
    window:reposition()
  end
end

registerAnonymousEventHandler("sysWindowResizeEvent", "GeyserReposition")

--- Responds to sysAppStyleSheetChange event and causes all windows 
-- managed by Geyser to reapply their stylesheet, if they have it.
function GeyserRestyle()
  for _, window in pairs(Geyser.windowList) do
    window:restyle()
  end
end

registerAnonymousEventHandler("sysAppStyleSheetChange", "GeyserRestyle")