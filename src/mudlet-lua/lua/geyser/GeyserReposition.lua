--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
function GeyserReposition()
	for _,window in pairs(Geyser.windowList) do
		window:reposition()
	end
end

registerAnonymousEventHandler("sysWindowResizeEvent", "GeyserReposition")
