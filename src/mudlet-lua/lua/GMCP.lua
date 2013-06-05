-- [[ A framework to track GMCP module usage ]] --

module("gmod",package.seeall)

local registeredUsers = registeredUsers or {}
local registeredModules = registeredModules or {}

-- Central text output function for the module.
function print(text)
    cecho(string.format("\n<yellow>[GMCP Tracker] <white>%s\n",text))
end

-- Register a user to the system.
function registerUser(user)
    if registeredUsers[user] then return "user exists"
    else registeredUsers[user] = true
    end

    return "user registered"
end

-- Add a user to a module
local function addUser(user,module)
    registeredModules[module][user] = true

    return "user added"
end

-- Drop a user from a module
local function dropUser(user,module)
    if not registeredModules[module] then return "no registered module" end
    if not registeredModules[module][user] then return "user not linked" end
    registeredModules[module][user] = nil

    return "user dropped"
end

-- Get the user count of a module.
local function getUserCount(module)
    if not registeredModules[module] then return 0 end
    local count = 0
    for i,v in pairs(registeredModules[module]) do count = count + 1 end
    return count
end

-- Check to see if the module is registered. Return a user list if it is.
function isRegisteredModule(mod)
    if not registeredModules[mod] then return false end
    return registeredModules[mod]
end

-- Enable a module that isn't already enabled, and register it's use to a user.
function enableModule(user,module)
    registerUser(user)

    -- Check to see if the module is already enabled.
    if not isRegisteredModule(module) then
        local addString = nil
        local ttlString = ""
        for _,mod in pairs(module:split("%.")) do
            if not addString then addString = mod
            else addString = string.format("%s.%s",addString,mod)
            end
            ttlString = string.format("%s\"%s 1\",",ttlString,addString)
        end
        ttlString = string.format("[%s]",ttlString:gsub("(.+),","%1"))
        sendGMCP(string.format([[Core.Supports.Add %s ]],ttlString))
        registeredModules[module] = {}
    end

    -- Add the user to the module's user list.
    addUser(user,module)
end


-- Send the module list again on reconnect, so activated modules stay enabled during
--   a session unless explicitly disabled
function gmod.reenableModules()
    if not next(gmcp) then return end

    local list = {}
    for module, users in pairs(registeredModules) do
        list[#list+1] = module.." 1"
    end
    if list[1] then sendGMCP("Core.Supports.Add "..yajl.to_string(list)) end
end
registerAnonymousEventHandler("sysConnectionEvent", "gmod.reenableModules")

-- Remove a user from a module's user list. Disable the module if nobody is using it.
function disableModule(user,module)
    registerUser(user)

    -- Make sure the module is enabled in the first place
    if not isRegisteredModule(module) then return "no registered module" end

    dropUser(user,module)

    -- The user is now removed from the list. Check the user count.
    if getUserCount(module) < 1 then
        sendGMCP(string.format([[Core.Supports.Remove ["%s"] ]],module))
        registeredModules[module] = nil
    end
end

function printModules(user)
    -- If a user is specified, print the name of all associated modules.
    if user then
        if not registeredUsers[user] then
            print(string.format("<cyan>%s <white>is not a registered user.",user))
            return
        end
        cecho(string.format("\n\n<white>Module listing for user: <cyan>%s",user))
        cecho(string.format("\n<white>%s",string.rep("-",45)))
        for i,v in pairs(registeredModules) do
            if v[user] then cecho(string.format("\n<yellow>%s",i)) end
        end
        return
    end

    -- No user specified, print all existing Modules with a user list.
    print("Current modules:")
    for i,v in pairs(registeredModules) do
        cecho(string.format("\n<yellow>%s\n  ",i))
        local doComma = false
        for j,_ in pairs(v) do
            if not doComma then doComma = true else cecho("<white>, ") end
            cecho(string.format("<cyan>%s",j))
        end
    end
end
