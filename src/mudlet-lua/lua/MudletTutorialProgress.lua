--[[
Mudlet Tutorial Progress System

This module handles lesson progression, verification, and the educational components
of the tutorial system.
--]]

local MudletTutorial = require("MudletTutorial")

-- Progress tracking and lesson management functions

function MudletTutorial.showProgress()
    local output = "\n╔══════════════════════════════════════════════════════════════╗\n"
    output = output .. "║                    TUTORIAL PROGRESS                        ║\n"
    output = output .. "╠══════════════════════════════════════════════════════════════╣\n"
    
    for i, lesson in ipairs(MudletTutorial.lessons) do
        local status = lesson.completed and "✓" or "○"
        local current = (i == MudletTutorial.state.currentLesson) and " ← Current" or ""
        output = output .. string.format("║ %s %-50s%s ║\n", status, lesson.title, current)
    end
    
    output = output .. "╚══════════════════════════════════════════════════════════════╝\n"
    
    -- Show current lesson details
    local currentLesson = MudletTutorial.lessons[MudletTutorial.state.currentLesson]
    if currentLesson and not currentLesson.completed then
        output = output .. "\nCurrent Lesson: " .. currentLesson.title .. "\n"
        output = output .. currentLesson.description .. "\n\n"
        output = output .. MudletTutorial.getLessonObjectives(currentLesson.id)
    end
    
    return output
end

function MudletTutorial.checkLessonProgress(lessonType, data)
    local currentLesson = MudletTutorial.lessons[MudletTutorial.state.currentLesson]
    
    if not currentLesson or currentLesson.completed then
        return
    end
    
    local completed = false
    
    -- Check completion criteria based on lesson type
    if currentLesson.id == "welcome" and lessonType == "welcome" then
        completed = true
    elseif currentLesson.id == "movement" and lessonType == "movement" then
        -- Check if player has visited at least 3 different rooms
        local roomCount = 0
        for _ in pairs(MudletTutorial.state.roomsVisited) do
            roomCount = roomCount + 1
        end
        completed = roomCount >= 3
    elseif currentLesson.id == "interaction" and lessonType == "interaction" then
        -- Check if player has examined items, read something, and picked up an item
        completed = MudletTutorial.state.playerProgress.hasExamined and 
                   MudletTutorial.state.playerProgress.hasRead and
                   #MudletTutorial.state.inventory > 0
    elseif currentLesson.id == "communication" and lessonType == "communication" then
        -- Check if player has talked to NPCs
        completed = MudletTutorial.state.playerProgress.hasCommunicated
    elseif currentLesson.id == "triggers" and lessonType == "trigger_created" then
        completed = MudletTutorial.state.playerProgress.hasCreatedTrigger
    elseif currentLesson.id == "aliases" and lessonType == "alias_created" then
        completed = MudletTutorial.state.playerProgress.hasCreatedAlias
    elseif currentLesson.id == "scripts" and lessonType == "script_created" then
        completed = MudletTutorial.state.playerProgress.hasCreatedScript
    end
    
    if completed then
        currentLesson.completed = true
        MudletTutorial.state.currentLesson = MudletTutorial.state.currentLesson + 1
        
        echo("\n" .. string.rep("=", 60) .. "\n")
        cecho("<green>*** LESSON COMPLETED: " .. currentLesson.title .. " ***</green>\n")
        echo(string.rep("=", 60) .. "\n")
        
        if MudletTutorial.state.currentLesson <= #MudletTutorial.lessons then
            echo("\n" .. MudletTutorial.getNextLessonHint())
        else
            echo("\n" .. MudletTutorial.showTutorialCompletion())
        end
    end
end

function MudletTutorial.getLessonObjectives(lessonId)
    local objectives = {
        welcome = "• Read the welcome sign\n• Look around your environment\n• Try the 'help' command",
        movement = "• Move north to the Training Grounds\n• Explore at least 3 different rooms\n• Return to Newbie Park",
        interaction = "• Examine objects in the world\n• Read at least one readable item\n• Pick up an item",
        communication = "• Talk to an NPC using 'say'\n• Ask an NPC about a topic\n• Practice social commands",
        mudlet_basics = "• Open the Script Editor (F4)\n• Explore Mudlet's interface\n• Learn about the main console",
        triggers = "• Create your first trigger\n• Test the trigger functionality\n• Understand pattern matching",
        aliases = "• Create a useful alias\n• Test your alias\n• Learn alias parameters",
        scripts = "• Write a simple Lua script\n• Execute the script\n• Understand script events",
        syntax_game = "• Try command variations\n• Experiment with parameters\n• Learn command discovery",
        advanced_features = "• Create a timer\n• Set up key bindings\n• Explore advanced options"
    }
    
    return "Lesson Objectives:\n" .. (objectives[lessonId] or "Objectives not defined yet.")
end

function MudletTutorial.getNextLessonHint()
    local nextLesson = MudletTutorial.lessons[MudletTutorial.state.currentLesson]
    
    if not nextLesson then
        return ""
    end
    
    local hints = {
        welcome = "💡 Hint: Start by reading the welcome sign with 'read welcome sign'",
        movement = "💡 Hint: Try going 'north' to explore the Training Grounds",
        interaction = "💡 Hint: Use 'examine' or 'look at' to inspect objects closely",
        communication = "💡 Hint: Find an NPC and try 'say hello' or 'ask <npc> about help'",
        mudlet_basics = "💡 Hint: Press F4 to open the Script Editor and explore Mudlet's interface",
        triggers = "💡 Hint: In the Script Editor, create a new trigger that responds to 'hello'",
        aliases = "💡 Hint: Create an alias like 'hp' that displays your health status",
        scripts = "💡 Hint: Write a script that echoes 'Hello, Mudlet!' to the console",
        syntax_game = "💡 Hint: Try variations like 'look', 'look at', 'examine', 'inspect' to see what works",
        advanced_features = "💡 Hint: Explore timers, key bindings, and other advanced Mudlet features"
    }
    
    return "Next: " .. nextLesson.title .. "\n" .. (hints[nextLesson.id] or "")
end

function MudletTutorial.showTutorialCompletion()
    return [[
╔══════════════════════════════════════════════════════════════════════════════╗
║                         🎉 TUTORIAL COMPLETED! 🎉                          ║
║                                                                              ║
║   Congratulations! You have successfully completed the Mudlet Tutorial.     ║
║                                                                              ║
║   You have learned:                                                          ║
║   ✓ Basic MUD commands and navigation                                        ║
║   ✓ Object interaction and communication                                     ║
║   ✓ Mudlet's powerful automation features                                    ║
║   ✓ Trigger and alias creation                                               ║
║   ✓ Basic Lua scripting                                                      ║
║   ✓ The art of command exploration                                           ║
║                                                                              ║
║   You're now ready to explore real MUD worlds with confidence!              ║
║                                                                              ║
║   Next Steps:                                                                ║
║   • Visit https://mudconnect.com to find MUDs to play                       ║
║   • Join the Mudlet community at https://forums.mudlet.org                  ║
║   • Explore the Mudlet manual at https://wiki.mudlet.org                    ║
║   • Share your experience and help other new players!                       ║
║                                                                              ║
║                     Thank you for learning with Mudlet!                     ║
╚══════════════════════════════════════════════════════════════════════════════╝
    ]]
end

-- Enhanced command handlers with educational components

function MudletTutorial.handleLook(command)
    local parts = command:split(" ")
    
    if #parts == 1 then
        -- Just "look" - show current room
        return MudletTutorial.describeCurrentRoom()
    else
        -- "look at something" 
        local target = table.concat(parts, " ", 3) -- Skip "look at"
        if not target or target == "" then
            target = parts[2] -- Handle "look something"
        end
        
        return MudletTutorial.examineTarget(target)
    end
end

function MudletTutorial.handleExamine(command)
    local parts = command:split(" ")
    local target = table.concat(parts, " ", 2) -- Skip "examine"
    
    if not target or target == "" then
        return "Examine what?"
    end
    
    MudletTutorial.state.playerProgress.hasExamined = true
    MudletTutorial.checkLessonProgress("interaction", "examined")
    
    return MudletTutorial.examineTarget(target)
end

function MudletTutorial.examineTarget(target)
    local room = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    
    -- Check room items
    if room.items then
        for _, itemId in ipairs(room.items) do
            local item = MudletTutorial.items[itemId]
            if item and item.name:lower():find(target:lower()) then
                return item.description
            end
        end
    end
    
    -- Check inventory items
    for _, itemId in ipairs(MudletTutorial.state.inventory) do
        local item = MudletTutorial.items[itemId]
        if item and item.name:lower():find(target:lower()) then
            return item.description
        end
    end
    
    -- Check NPCs
    if room.npcs then
        for _, npcId in ipairs(room.npcs) do
            local npc = MudletTutorial.npcs[npcId]
            if npc and npc.name:lower():find(target:lower()) then
                return npc.description
            end
        end
    end
    
    return "You don't see that here."
end

function MudletTutorial.handleRead(command)
    local parts = command:split(" ")
    local target = table.concat(parts, " ", 2) -- Skip "read"
    
    if not target or target == "" then
        return "Read what?"
    end
    
    local room = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    
    -- Check room items
    if room.items then
        for _, itemId in ipairs(room.items) do
            local item = MudletTutorial.items[itemId]
            if item and item.name:lower():find(target:lower()) and item.readable then
                MudletTutorial.state.playerProgress.hasRead = true
                MudletTutorial.checkLessonProgress("interaction", "read")
                return item.text or item.description
            end
        end
    end
    
    -- Check inventory items
    for _, itemId in ipairs(MudletTutorial.state.inventory) do
        local item = MudletTutorial.items[itemId]
        if item and item.name:lower():find(target:lower()) and item.readable then
            MudletTutorial.state.playerProgress.hasRead = true
            MudletTutorial.checkLessonProgress("interaction", "read")
            return item.text or item.description
        end
    end
    
    return "You can't read that."
end

function MudletTutorial.handleGet(command)
    local parts = command:split(" ")
    local target = table.concat(parts, " ", 2) -- Skip "get"/"take"
    
    if not target or target == "" then
        return "Get what?"
    end
    
    local room = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    
    if room.items then
        for i, itemId in ipairs(room.items) do
            local item = MudletTutorial.items[itemId]
            if item and item.name:lower():find(target:lower()) and item.takeable then
                -- Remove from room, add to inventory
                table.remove(room.items, i)
                table.insert(MudletTutorial.state.inventory, itemId)
                
                MudletTutorial.checkLessonProgress("interaction", "taken")
                return "You take " .. item.name .. "."
            elseif item and item.name:lower():find(target:lower()) then
                return "You can't take that."
            end
        end
    end
    
    return "You don't see that here."
end

function MudletTutorial.showInventory()
    if #MudletTutorial.state.inventory == 0 then
        return "You aren't carrying anything."
    end
    
    local output = "You are carrying:\n"
    for _, itemId in ipairs(MudletTutorial.state.inventory) do
        local item = MudletTutorial.items[itemId]
        if item then
            output = output .. "  " .. item.name .. "\n"
        end
    end
    
    return output
end

function MudletTutorial.handleSay(command)
    local parts = command:split(" ")
    local message = table.concat(parts, " ", 2) -- Skip "say"
    
    if not message or message == "" then
        return "Say what?"
    end
    
    MudletTutorial.state.playerProgress.hasCommunicated = true
    MudletTutorial.checkLessonProgress("communication", "said")
    
    local output = "You say, '" .. message .. "'\n"
    
    -- Check if NPCs respond
    local room = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    if room.npcs then
        for _, npcId in ipairs(room.npcs) do
            local npc = MudletTutorial.npcs[npcId]
            if npc and npc.dialogue.greeting then
                output = output .. npc.name .. " says, '" .. npc.dialogue.greeting .. "'\n"
                break -- Only one NPC responds
            end
        end
    end
    
    return output
end

function MudletTutorial.handleAsk(command)
    -- Parse "ask person about topic"
    local parts = command:split(" ")
    if #parts < 4 then
        return "Ask who about what? Try: ask <person> about <topic>"
    end
    
    local aboutIndex = nil
    for i, part in ipairs(parts) do
        if part:lower() == "about" then
            aboutIndex = i
            break
        end
    end
    
    if not aboutIndex then
        return "Ask who about what? Try: ask <person> about <topic>"
    end
    
    local person = table.concat(parts, " ", 2, aboutIndex - 1)
    local topic = table.concat(parts, " ", aboutIndex + 1)
    
    local room = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    if room.npcs then
        for _, npcId in ipairs(room.npcs) do
            local npc = MudletTutorial.npcs[npcId]
            if npc and npc.name:lower():find(person:lower()) then
                MudletTutorial.state.playerProgress.hasCommunicated = true
                MudletTutorial.checkLessonProgress("communication", "asked")
                
                local response = npc.dialogue[topic:lower()] or npc.dialogue.help or "I don't know about that."
                return "You ask " .. npc.name .. " about " .. topic .. ".\n" .. 
                       npc.name .. " says, '" .. response .. "'"
            end
        end
    end
    
    return "There's no one here by that name."
end

function MudletTutorial.handleUnknownCommand(command)
    -- This is part of the "syntax game" - encouraging experimentation
    local suggestions = {
        "I don't understand that command. Try 'help' for a list of available commands.",
        "That's not a command I recognize. Experiment with variations - that's part of learning!",
        "Unknown command. In MUDs, trying different syntax is part of the game!",
        "I don't know that command. Try abbreviated versions or similar words.",
        "That command doesn't exist here. Keep experimenting - discovery is part of the fun!"
    }
    
    return suggestions[math.random(#suggestions)]
end

-- Advanced tutorial features

function MudletTutorial.createFirstTrigger()
    -- This would be called when the user creates their first trigger
    echo([[
🎉 Congratulations! You've created your first trigger!

Triggers are one of Mudlet's most powerful features. They automatically respond to 
specific text from the game, allowing you to:

• Highlight important information
• Automatically perform actions  
• Track game statistics
• Create complex automation

Your trigger will now fire whenever it sees its pattern. Try typing the pattern
to see it in action!

Tip: Triggers can use regular expressions for complex pattern matching.
    ]])
    
    MudletTutorial.state.playerProgress.hasCreatedTrigger = true
    MudletTutorial.checkLessonProgress("trigger_created", true)
end

function MudletTutorial.createFirstAlias()
    -- This would be called when the user creates their first alias
    echo([[
🎉 Excellent! You've created your first alias!

Aliases are shortcuts that expand into longer commands. They're perfect for:

• Creating shortcuts for complex commands
• Adding parameters to customize behavior
• Building command macros
• Making gameplay more efficient

Your alias is now active. Try typing it to see it work!

Tip: Use ^(.*)$ patterns to capture arguments in your aliases.
    ]])
    
    MudletTutorial.state.playerProgress.hasCreatedAlias = true
    MudletTutorial.checkLessonProgress("alias_created", true)
end

function MudletTutorial.createFirstScript()
    -- This would be called when the user creates their first script
    echo([[
🎉 Outstanding! You've written your first Lua script!

Lua scripting gives you unlimited power to customize Mudlet. You can:

• Create complex automation systems
• Build custom interfaces  
• Manage game data and statistics
• Create mini-applications within Mudlet

The script is now part of your profile. Lua is a powerful programming language,
and Mudlet provides many built-in functions to work with.

Tip: Check the Mudlet manual for a complete list of available Lua functions.
    ]])
    
    MudletTutorial.state.playerProgress.hasCreatedScript = true
    MudletTutorial.checkLessonProgress("script_created", true)
end

-- Export the enhanced tutorial system
return MudletTutorial