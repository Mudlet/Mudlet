--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
-- @author guy
-- @module GeyserReposition

--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
-- @param event a sysWindowResizeEvent or sysUserWindowResizeEvent event
-- @param w the new width
-- @param h the new height
-- @param arg additional arguments
function GeyserReposition(event, w, h, arg)
  for _, window in pairs(Geyser.windowList) do
    if event == "sysUserWindowResizeEvent" and window.type == "userwindow" and arg.."Container" == window.name then
      window:reposition()
    elseif event == "sysWindowResizeEvent" and window.type ~= "userwindow" then 
      window:reposition()
    end
  end
end

registerAnonymousEventHandler("sysWindowResizeEvent", "GeyserReposition")
registerAnonymousEventHandler("sysUserWindowResizeEvent", "GeyserReposition")
