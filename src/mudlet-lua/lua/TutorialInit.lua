--[[
Mudlet Tutorial Package Initialization

This script sets up the tutorial environment when the tutorial profile is loaded.
It handles the connection simulation and integrates with Mudlet's systems.
--]]

-- Tutorial package metadata
tutorialPackage = tutorialPackage or {}
tutorialPackage.name = "MudletTutorial"
tutorialPackage.version = "1.0.0"
tutorialPackage.author = "Mudlet Team"
tutorialPackage.description = "Interactive tutorial for learning MUD gaming and Mudlet features"

-- Load tutorial modules
local MudletTutorial = require("MudletTutorial") 
require("MudletTutorialProgress")

-- Tutorial state management
if not tutorialState then
    tutorialState = {
        isActive = false,
        initialized = false,
        simulateConnection = false
    }
end

-- Check if this is the tutorial profile
local function isTutorialProfile()
    local profileName = getProfileName() or ""
    return profileName == "Mudlet Tutorial" or 
           (mpHost and mpHost.getUrl and mpHost:getUrl() == "tutorial.mudlet.local")
end

-- Initialize tutorial when profile loads
function initializeTutorial()
    if tutorialState.initialized or not isTutorialProfile() then
        return
    end
    
    tutorialState.initialized = true
    tutorialState.isActive = true
    
    -- Set up tutorial environment
    setupTutorialAutomation()
    
    -- Simulate connection process
    simulateConnectionProcess()
end

-- Simulate MUD connection process
function simulateConnectionProcess()
    tutorialState.simulateConnection = true
    
    -- Clear the console first
    clearConsole()
    
    -- Show connection messages with proper timing
    cecho("<white>Resolving tutorial.mudlet.local...")
    tempTimer(1, function()
        cecho("<green>\nConnected to Mudlet Tutorial Server.")
        cecho("<white>\nLoading world data...")
        
        tempTimer(1.5, function()
            cecho("<green>\nCharacter login successful.")
            cecho("<yellow>\nWelcome to the Tutorial Realm!\n")
            
            -- Initialize the actual tutorial
            MudletTutorial.initialize()
            tutorialState.simulateConnection = false
            
            -- Enable command processing
            enableTrigger("Tutorial Command Handler")
        end)
    end)
end

-- Set up tutorial-specific automation
function setupTutorialAutomation()
    -- Create main command interceptor
    if not exists("Tutorial Command Handler", "trigger") then
        permRegexTrigger("Tutorial Command Handler", "^(.+)$", [[
            if tutorialState and tutorialState.isActive and not tutorialState.simulateConnection then
                local command = matches[2] or ""
                command = command:trim()
                
                -- Skip empty commands or tutorial output
                if command == "" or 
                   command:starts("You ") or 
                   command:starts("The ") or 
                   command:starts("╔") or 
                   command:starts("║") or
                   command:starts("Resolving") or
                   command:starts("Connected") or
                   command:starts("Loading") or
                   command:starts("Character") or
                   command:starts("Welcome") then
                    return
                end
                
                -- Process the command through tutorial system
                deleteLine()
                local response = MudletTutorial.processCommand(command)
                if response then
                    cecho("<white>" .. response .. "\n")
                end
            end
        ]])
        -- Start disabled until connection simulation is complete
        disableTrigger("Tutorial Command Handler")
    end
    
    -- Create tutorial help alias
    if not exists("tutorial help", "alias") then
        permAlias("tutorial help", "^(?:tutorial|progress|help tutorial)$", [[
            if MudletTutorial and MudletTutorial.showProgress then
                cecho(MudletTutorial.showProgress())
            end
        ]])
    end
    
    -- Create Mudlet learning aliases
    if not exists("learn trigger", "alias") then
        permAlias("learn trigger", "^learn trigger$", [[
            cecho("<yellow>To create a trigger in Mudlet:\n")
            cecho("<white>1. Press F4 to open the Script Editor\n")
            cecho("<white>2. Click 'Triggers' in the left panel\n")
            cecho("<white>3. Click the '+' button to add a new trigger\n")
            cecho("<white>4. Enter a name for your trigger\n")
            cecho("<white>5. Enter the pattern to match (e.g., 'You hit' or 'Health: (.+)')\n")
            cecho("<white>6. Write the response code in the editor below\n")
            cecho("<white>7. Click 'Save' to activate your trigger\n\n")
            cecho("<cyan>Try creating a trigger that responds to 'hello' and echoes 'Hello back!'\n")
        ]])
    end
    
    if not exists("learn alias", "alias") then  
        permAlias("learn alias", "^learn alias$", [[
            cecho("<yellow>To create an alias in Mudlet:\n")
            cecho("<white>1. Press F4 to open the Script Editor\n")
            cecho("<white>2. Click 'Aliases' in the left panel\n")
            cecho("<white>3. Click the '+' button to add a new alias\n")
            cecho("<white>4. Enter a name for your alias\n")
            cecho("<white>5. Enter the pattern (e.g., 'hp' or 'cast (.+)')\n")
            cecho("<white>6. Write the command(s) to execute in the editor\n")
            cecho("<white>7. Click 'Save' to activate your alias\n\n")
            cecho("<cyan>Try creating an alias 'hp' that shows your health status!\n")
        ]])
    end
    
    if not exists("learn script", "alias") then
        permAlias("learn script", "^learn script$", [[
            cecho("<yellow>To create a script in Mudlet:\n")
            cecho("<white>1. Press F4 to open the Script Editor\n")
            cecho("<white>2. Click 'Scripts' in the left panel\n")
            cecho("<white>3. Click the '+' button to add a new script\n")
            cecho("<white>4. Enter a name for your script\n")
            cecho("<white>5. Write Lua code in the editor (e.g., echo('Hello, World!'))\n")
            cecho("<white>6. Click 'Save' to save your script\n\n")
            cecho("<white>Scripts run when they're saved or when events they register for occur.\n")
            cecho("<cyan>Try creating a script that echoes 'Welcome to Mudlet!' when saved!\n")
        ]])
    end
    
    -- Create syntax exploration helpers
    if not exists("syntax explorer", "alias") then
        permAlias("syntax explorer", "^syntax (.+)$", [[
            local command = matches[2]
            cecho("<yellow>Exploring syntax for: " .. command .. "\n")
            cecho("<white>Common variations to try:\n")
            cecho("<cyan>• " .. command .. "\n")
            cecho("<cyan>• " .. command .. " <target>\n") 
            cecho("<cyan>• " .. command .. " at <target>\n")
            cecho("<cyan>• " .. command .. " <target> with <item>\n")
            cecho("<cyan>• " .. string.sub(command, 1, 3) .. " (abbreviated)\n")
            cecho("<cyan>• " .. string.sub(command, 1, 4) .. " (abbreviated)\n")
            cecho("<green>\nThe 'syntax game' is about experimenting with different forms!\n")
        ]])
    end
end

-- Event handlers for tutorial integration
function onTutorialProfileLoaded()
    if isTutorialProfile() then
        -- Start tutorial automatically after a short delay
        tempTimer(2, function()
            if not tutorialState.initialized then
                initializeTutorial()
            end
        end)
    end
end

function onTutorialConnect()
    if isTutorialProfile() then
        initializeTutorial()
    end
end

function onTutorialDisconnect()
    if tutorialState.isActive then
        tutorialState.isActive = false
        cecho("<yellow>Tutorial session ended. Thanks for learning with Mudlet!\n")
    end
end

-- Special handlers for script editor integration
function onTriggerCreated(triggerName)
    if tutorialState.isActive then
        tempTimer(0.5, function()
            if MudletTutorial and MudletTutorial.createFirstTrigger then
                MudletTutorial.createFirstTrigger()
            end
        end)
    end
end

function onAliasCreated(aliasName)
    if tutorialState.isActive then
        tempTimer(0.5, function()
            if MudletTutorial and MudletTutorial.createFirstAlias then
                MudletTutorial.createFirstAlias()
            end
        end)
    end
end

function onScriptCreated(scriptName)
    if tutorialState.isActive then
        tempTimer(0.5, function()
            if MudletTutorial and MudletTutorial.createFirstScript then
                MudletTutorial.createFirstScript()
            end
        end)
    end
end

-- Register event handlers for proper integration
if not tutorialEventHandlers then
    tutorialEventHandlers = {}
    tutorialEventHandlers.connect = registerAnonymousEventHandler("sysConnectionEvent", "onTutorialConnect")
    tutorialEventHandlers.disconnect = registerAnonymousEventHandler("sysDisconnectionEvent", "onTutorialDisconnect")
end

-- Tutorial-specific commands
function startTutorial()
    if not isTutorialProfile() then
        cecho("<red>This command only works in the Mudlet Tutorial profile.\n")
        return
    end
    
    if not tutorialState.isActive then
        initializeTutorial()
    else
        cecho("<yellow>Tutorial is already active. Type 'help' for available commands.\n")
    end
end

function resetTutorial()
    if not isTutorialProfile() then
        cecho("<red>This command only works in the Mudlet Tutorial profile.\n")
        return
    end
    
    tutorialState.isActive = false
    tutorialState.initialized = false
    
    -- Reset tutorial state
    if MudletTutorial then
        MudletTutorial.state.currentLesson = 1
        MudletTutorial.state.playerProgress = {}
        MudletTutorial.state.currentRoom = "newbie_park"
        MudletTutorial.state.inventory = {}
        MudletTutorial.state.roomsVisited = {}
        
        for _, lesson in ipairs(MudletTutorial.lessons) do
            lesson.completed = false
        end
    end
    
    clearConsole()
    cecho("<yellow>Tutorial has been reset. The tutorial will restart automatically.\n")
    
    -- Restart tutorial
    tempTimer(2, function()
        startTutorial()
    end)
end

-- Create global commands
if not exists("start tutorial command", "alias") then
    permAlias("start tutorial command", "^start tutorial$", "startTutorial()")
end

if not exists("reset tutorial command", "alias") then
    permAlias("reset tutorial command", "^reset tutorial$", "resetTutorial()")
end

-- Syntax game enhancement
function exploreCommand(baseCommand)
    local variations = {
        baseCommand,
        string.sub(baseCommand, 1, 3),
        string.sub(baseCommand, 1, 4), 
        baseCommand .. " all",
        baseCommand .. " here",
        "ex " .. baseCommand,
        baseCommand .. " carefully",
        baseCommand .. " quickly"
    }
    
    cecho("<yellow>Syntax variations for '" .. baseCommand .. "':\n")
    for _, variation in ipairs(variations) do
        cecho("<cyan>  " .. variation .. "\n")
    end
    cecho("<green>\nTry these variations to see what works!\n")
end

-- Initialize tutorial state tracking
tutorialStats = tutorialStats or {
    commandsTyped = 0,
    roomsExplored = 0,
    itemsExamined = 0,
    npcsSpokenTo = 0,
    triggersCreated = 0,
    aliasesCreated = 0,
    scriptsCreated = 0
}

-- Hook into command processing to track statistics
function trackTutorialStats(command)
    if tutorialState.isActive then
        tutorialStats.commandsTyped = tutorialStats.commandsTyped + 1
        
        -- Track specific activities
        if command:match("^[nsew]") or command:match("north") or command:match("south") or 
           command:match("east") or command:match("west") then
            tutorialStats.roomsExplored = tutorialStats.roomsExplored + 1
        elseif command:match("examine") or command:match("look") then
            tutorialStats.itemsExamined = tutorialStats.itemsExamined + 1  
        elseif command:match("say") or command:match("ask") or command:match("tell") then
            tutorialStats.npcsSpokenTo = tutorialStats.npcsSpokenTo + 1
        end
    end
end

-- Auto-start tutorial for Mudlet Tutorial profile
if isTutorialProfile() then
    -- Check if we should auto-start the tutorial
    tempTimer(1, function()
        if not tutorialState.initialized then
            cecho("<green>Welcome to the Mudlet Tutorial! Starting automatically...\n")
            tempTimer(2, function()
                onTutorialProfileLoaded()
            end)
        end
    end)
end

cecho("<yellow>Mudlet Tutorial package loaded successfully!\n")
if isTutorialProfile() then
    cecho("<white>The tutorial will start automatically, or type 'start tutorial' to begin manually.\n")
else
    cecho("<white>Type 'start tutorial' when using the Mudlet Tutorial profile.\n")
end