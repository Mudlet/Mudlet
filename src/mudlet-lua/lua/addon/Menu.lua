--- Menu Module for Mudlet
-- Provides Lua-based control of menu items with object-oriented API.
-- @module Menu

Menu = Menu or {}

-- Internal state tracking
Menu._items = {}
Menu._eventHandler = nil

--- MenuItem class for menu items
-- @type Menu.Item
Menu.Item = {}
Menu.Item.__index = Menu.Item

--- Add a menu item.
-- @param cons table of item configuration:
--   - menu: string - Menu path (e.g., "Speech" or "Speech/Settings")
--   - name: string - Menu item text
--   - shortcut: string - Optional keyboard shortcut (e.g., "Ctrl+Shift+M")
--   - onClick: function - Optional callback when clicked
--   - enabled: boolean - Initial enabled state (default true)
--   - checkable: boolean - Whether item is checkable (default false)
--   - checked: boolean - Initial checked state if checkable (default false)
-- @return Menu.Item the created menu item object
function Menu.addItem(cons)
  cons = cons or {}

  if not cons.name then
    error("Menu.addItem requires 'name' parameter", 2)
  end

  local menuPath = cons.menu or ""
  local name = cons.name
  local shortcut = cons.shortcut or ""

  -- Call C++ to create the menu item
  local itemId = addMenuItem(menuPath, name, shortcut)
  if not itemId or itemId < 0 then
    return nil, "Failed to create menu item"
  end

  -- Create item object
  local item = setmetatable({}, Menu.Item)
  item._id = itemId
  item._menuPath = menuPath
  item._name = name
  item._onClick = cons.onClick
  item._enabled = cons.enabled ~= false
  item._checkable = cons.checkable or false
  item._checked = cons.checked or false

  -- Track the item
  Menu._items[itemId] = item

  -- Set initial enabled state
  if not item._enabled then
    setMenuItemEnabled(itemId, false)
  end

  -- Set initial checked state if checkable
  if item._checkable then
    setMenuItemChecked(itemId, item._checked)
  end

  return item
end

--- Remove a menu item.
-- @param item Menu.Item the item to remove
-- @return boolean success
function Menu.removeItem(item)
  if not item or not item._id then
    return false
  end

  local success = removeMenuItem(item._id)
  if success then
    Menu._items[item._id] = nil
  end
  return success
end

--- Add a separator to a menu (convenience function).
-- Note: Separators are implemented as disabled empty items
-- @param menuPath string the menu path
-- @return Menu.Item the separator item
function Menu.addSeparator(menuPath)
  return Menu.addItem({
    menu = menuPath,
    name = "---",
    enabled = false,
  })
end

--- Get the item ID.
-- @return number the item ID
function Menu.Item:getId()
  return self._id
end

--- Get the item name.
-- @return string the item name
function Menu.Item:getName()
  return self._name
end

--- Get the menu path.
-- @return string the menu path
function Menu.Item:getMenuPath()
  return self._menuPath
end

--- Set the item's click callback.
-- @param callback function to call when clicked
function Menu.Item:setOnClick(callback)
  self._onClick = callback
end

--- Enable or disable the item.
-- @param enabled boolean enabled state
-- @return boolean success
function Menu.Item:setEnabled(enabled)
  self._enabled = enabled
  return setMenuItemEnabled(self._id, enabled)
end

--- Check if the item is enabled.
-- @return boolean enabled state
function Menu.Item:isEnabled()
  return self._enabled
end

--- Set the checked state (makes item checkable if not already).
-- @param checked boolean checked state
-- @return boolean success
function Menu.Item:setChecked(checked)
  self._checkable = true
  self._checked = checked
  return setMenuItemChecked(self._id, checked)
end

--- Check if the item is checked.
-- @return boolean checked state
function Menu.Item:isChecked()
  return self._checked
end

--- Toggle the checked state.
-- @return boolean new checked state
function Menu.Item:toggleChecked()
  self:setChecked(not self._checked)
  return self._checked
end

--- Remove this item from the menu.
-- @return boolean success
function Menu.Item:remove()
  return Menu.removeItem(self)
end

-- Event handler for menu item clicks
local function handleMenuItemClick(event, itemId)
  itemId = tonumber(itemId)
  if itemId and Menu._items[itemId] then
    local item = Menu._items[itemId]
    -- Toggle checked state for checkable items and sync with host UI
    if item._checkable then
      item._checked = not item._checked
      setMenuItemChecked(itemId, item._checked)
    end
    if item._onClick then
      item._onClick(item)
    end
  end
end

--- Initialize the menu event handler (called automatically).
function Menu._setupEventHandler()
  if Menu._eventHandler then
    return
  end

  if registerAnonymousEventHandler then
    Menu._eventHandler = registerAnonymousEventHandler(
      "sysMenuItemClicked",
      handleMenuItemClick
    )
  end
end

-- Auto-initialize on load
Menu._setupEventHandler()
