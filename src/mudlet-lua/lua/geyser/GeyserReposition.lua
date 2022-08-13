--------------------------------------
--                                  --
-- The Geyser Layout Manager by guy --
--                                  --
--------------------------------------

--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
function GeyserReposition(event, w, h, arg)
  for _, window in pairs(Geyser.windowList) do
    if event == "sysUserWindowResizeEvent" and window.type == "userwindow" and arg.."Container" == window.name then
      window:reposition()
    elseif event == "sysWindowResizeEvent" and window.type ~= "userwindow" then 
      window:reposition()
    end
  end
end

function GeyserRepaint(event, gainedNotLostFocus)
  if event == "sysProfileFocusChangeEvent" then
    for _, window in pairs(Geyser.windowList) do
--      if window.type ~= "userwindow" then
        window:reposition()
--      end
    end
  end
end

registerAnonymousEventHandler("sysWindowResizeEvent", "GeyserReposition")
registerAnonymousEventHandler("sysUserWindowResizeEvent", "GeyserReposition")
registerAnonymousEventHandler("sysProfileFocusChangeEvent", "GeyserRepaint")
