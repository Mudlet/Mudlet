--------------------------------------
--                                  --
-- The Geyser Layout Manager        --
--                                  --
--------------------------------------

--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
function Geyser.updateStylesheets()
  for _, window in pairs(Geyser.windowList) do
  	print("checking ".._)
    if window.updatestylesheet then window:updatestylesheet() end
  end
end

registerAnonymousEventHandler("sysAppStyleSheetChange", "Geyser.updateStylesheets")
