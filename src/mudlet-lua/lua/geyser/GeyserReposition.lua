--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
-- @author guy
-- @module GeyserReposition

--- Responds to sysWindowResizeEvent and causes all windows managed
-- by Geyser to update their sizes and positions.
-- Called without an event by Geyser:reposition(), which is how Geyser:end_update()
-- applies the layout it deferred; that call is meant for every window Geyser owns.
-- @param event a sysWindowResizeEvent or sysUserWindowResizeEvent event, or nil to reposition everything
-- @param w the new width
-- @param h the new height
-- @param arg additional arguments
function GeyserReposition(event, w, h, arg)
  if event ~= nil and event ~= "sysWindowResizeEvent" and event ~= "sysUserWindowResizeEvent" then
    -- otherwise a mistyped event name is indistinguishable from a no-op
    debugc(string.format("GeyserReposition: ignoring the unknown event '%s'", tostring(event)))
    return
  end
  if event == "sysUserWindowResizeEvent" and not arg then
    debugc("GeyserReposition: sysUserWindowResizeEvent needs the name of the user window that was resized")
    return
  end
  -- repositioning raises events, and a handler that creates or deletes a top
  -- level window would be mutating windowList mid-traversal, so work off a
  -- snapshot of the names and re-check each one is still there
  local names = {}
  for name in pairs(Geyser.windowList) do
    names[#names + 1] = name
  end
  for _, name in ipairs(names) do
    local window = Geyser.windowList[name]
    if window then
      if event == nil then
        window:reposition()
      elseif event == "sysUserWindowResizeEvent" and window.type == "userwindow" and arg.."Container" == window.name then
        window:reposition()
      elseif event == "sysWindowResizeEvent" and window.type ~= "userwindow" then
        window:reposition()
      end
    end
  end
end

registerAnonymousEventHandler("sysWindowResizeEvent", "GeyserReposition")
registerAnonymousEventHandler("sysUserWindowResizeEvent", "GeyserReposition")
