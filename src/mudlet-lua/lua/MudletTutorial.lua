--[[
Mudlet Tutorial System - Main Module

This module provides a comprehensive, interactive tutorial for new Mudlet users.
It simulates a MUD-like environment where users can safely learn both MUD basics
and Mudlet-specific features without connecting to a real game server.

Features:
- Interactive MUD simulation
- Step-by-step Mudlet feature tutorials
- Command syntax exploration ("syntax game")
- Progress verification system
- Safe, offline learning environment
--]]

local MudletTutorial = {}

-- Tutorial state management
MudletTutorial.state = {
    currentLesson = 1,
    playerProgress = {},
    currentRoom = "newbie_park",
    playerStats = {
        level = 1,
        health = 100,
        maxHealth = 100,
        experience = 0
    },
    inventory = {},
    roomsVisited = {},
    commandsLearned = {},
    scriptsCreated = {}
}

-- Tutorial lesson structure
MudletTutorial.lessons = {
    {
        id = "welcome",
        title = "Welcome to the World of MUDs",
        description = "Learn the basics of text-based gaming",
        completed = false
    },
    {
        id = "movement",
        title = "Moving Around",
        description = "Master basic movement commands",
        completed = false
    },
    {
        id = "interaction",
        title = "Interacting with the World",
        description = "Learn to examine, get, and use objects",
        completed = false
    },
    {
        id = "communication",
        title = "Talking to Others",
        description = "Master communication commands",
        completed = false
    },
    {
        id = "mudlet_basics",
        title = "Mudlet Interface",
        description = "Understanding Mudlet's interface",
        completed = false
    },
    {
        id = "triggers",
        title = "Creating Triggers",
        description = "Automate responses to game events",
        completed = false
    },
    {
        id = "aliases",
        title = "Creating Aliases",
        description = "Create shortcuts for complex commands",
        completed = false
    },
    {
        id = "scripts",
        title = "Lua Scripting Basics",
        description = "Write your first Lua scripts",
        completed = false
    },
    {
        id = "syntax_game",
        title = "The Syntax Game",
        description = "Master command exploration and experimentation",
        completed = false
    },
    {
        id = "advanced_features",
        title = "Advanced Mudlet Features",
        description = "Explore timers, keys, and more",
        completed = false
    }
}

-- Room database for the tutorial world
MudletTutorial.rooms = {
    newbie_park = {
        name = "Newbie Park - Under the Great Oak",
        description = [[You stand beneath a massive oak tree in the center of Newbie Park. 
This peaceful area serves as a safe haven for new adventurers learning the ways of the world.
The ancient tree's branches stretch wide overhead, providing comfortable shade.
A small wooden bench sits beneath the tree, and you notice several signs posted around the area.
        
To the north, you can see a path leading to the Training Grounds.
To the east, there's a trail heading toward the Village Square.
To the south, the path leads to the Tutorial Library.
To the west, you notice stairs going down to what appears to be a Practice Cave.]],
        exits = {
            north = "training_grounds",
            east = "village_square", 
            south = "tutorial_library",
            west = "practice_cave"
        },
        items = {
            "welcome_sign",
            "instruction_board", 
            "wooden_bench"
        },
        special = "starting_room"
    },
    
    training_grounds = {
        name = "Training Grounds",
        description = [[This open area is set up for combat practice and skill training.
Training dummies made of straw and wood are scattered around the grounds.
A weapons rack stands against one wall, holding practice weapons.
Several other students are here practicing their combat techniques.

You can return south to Newbie Park.]],
        exits = {
            south = "newbie_park"
        },
        items = {
            "training_dummy",
            "practice_sword",
            "target_dummy"
        },
        npcs = {
            "combat_instructor"
        }
    },
    
    village_square = {
        name = "Village Square", 
        description = [[A bustling village square with a stone fountain in the center.
Villagers go about their daily business, chatting and trading.
Several shops line the square, and you can hear the sound of a blacksmith's hammer nearby.
This area demonstrates the social aspects of MUD gaming.

You can go west to return to Newbie Park, or explore the shops to the north.]],
        exits = {
            west = "newbie_park",
            north = "general_store"
        },
        items = {
            "stone_fountain",
            "village_notice"
        },
        npcs = {
            "friendly_villager",
            "merchant_bob"
        }
    },
    
    tutorial_library = {
        name = "Tutorial Library",
        description = [[A quiet library filled with books about MUD gaming and Mudlet features.
Tall bookshelves stretch to the ceiling, filled with ancient tomes and modern guides.
A helpful librarian sits at a large desk, ready to answer questions about Mudlet.
Several reading tables are available for studying.

You can return north to Newbie Park.]],
        exits = {
            north = "newbie_park"
        },
        items = {
            "mudlet_manual",
            "mud_guide",
            "scripting_book"
        },
        npcs = {
            "wise_librarian"
        }
    },
    
    practice_cave = {
        name = "Practice Cave",
        description = [[A dimly lit cave that serves as a safe practice area for trying out scripts.
Strange glowing crystals provide just enough light to see by.
This is the perfect place to test triggers, aliases, and other Mudlet automation safely.
Ancient runes on the walls seem to pulse with magical energy.

Stairs lead up and east back to Newbie Park.]],
        exits = {
            up = "newbie_park",
            east = "newbie_park"
        },
        items = {
            "glowing_crystal",
            "practice_journal"
        },
        special = "script_testing_area"
    },
    
    general_store = {
        name = "Adventurer's General Store",
        description = [[A well-stocked store catering to adventurers of all levels.
Shelves are lined with various supplies, from basic provisions to magical trinkets.
The shopkeeper greets you with a warm smile.

You can head south to return to the Village Square.]],
        exits = {
            south = "village_square"
        },
        items = {
            "health_potion",
            "newbie_backpack",
            "torch"
        },
        npcs = {
            "shop_keeper"
        }
    }
}

-- NPCs with their dialogue and functionality
MudletTutorial.npcs = {
    combat_instructor = {
        name = "a skilled combat instructor",
        description = "A weathered veteran who teaches combat basics to newcomers.",
        dialogue = {
            greeting = "Welcome to the training grounds! Ready to learn some combat basics?",
            help = "Try 'practice combat' to learn fighting techniques, or 'ask instructor about weapons' to learn about equipment.",
            combat = "Combat in MUDs is usually turn-based. You enter commands like 'kill target' or 'cast spell'. Let me show you!",
            weapons = "Different weapons have different advantages. Swords are reliable, axes hit hard, and daggers are fast!"
        },
        teaches = "combat_basics"
    },
    
    friendly_villager = {
        name = "a friendly villager",
        description = "A cheerful local resident who enjoys talking to travelers.",
        dialogue = {
            greeting = "Hello there, newcomer! Welcome to our village!",
            help = "If you need directions, just ask! Our village has many interesting places to explore.",
            village = "Our village is known for being very welcoming to new adventurers. Everyone here is happy to help!",
            mudlet = "I hear you're learning to use Mudlet. That's wonderful! It will make your adventures much easier."
        }
    },
    
    wise_librarian = {
        name = "the wise librarian",
        description = "An elderly scholar with vast knowledge of both MUDs and Mudlet.",
        dialogue = {
            greeting = "Greetings, young scholar. How may I assist your learning today?",
            help = "I can teach you about triggers, aliases, scripts, and many other Mudlet features. What interests you?",
            triggers = "Triggers automatically respond to specific text from the game. Very useful for automation!",
            aliases = "Aliases let you create shortcuts for complex commands. Type 'alias help' to learn more!",
            scripts = "Lua scripts give you tremendous power to customize Mudlet. Shall we start with something simple?",
            mudlet = "Mudlet is a powerful MUD client. Its scripting capabilities are nearly limitless!"
        },
        teaches = "mudlet_features"
    },
    
    merchant_bob = {
        name = "Bob the merchant",
        description = "A portly merchant with a friendly demeanor and a keen eye for business.",
        dialogue = {
            greeting = "Welcome to the square! Looking to buy or sell anything today?",
            help = "I sell basic supplies and buy items from adventurers. Great for learning trading commands!",
            trade = "Trading is an important part of MUD life. Try 'buy item' or 'sell item' to practice!",
            economy = "Every MUD has its own economy. Learning to trade effectively can make you rich!"
        }
    },
    
    shop_keeper = {
        name = "the shop keeper",
        description = "A busy shopkeeper who runs the general store.",
        dialogue = {
            greeting = "Welcome to my store! We have everything an adventurer needs.",
            help = "Feel free to browse my wares. Use 'list' to see what's for sale, 'buy item' to purchase.",
            inventory = "I stock health potions, backpacks, torches, and other essential supplies.",
            prices = "My prices are fair - perfect for newcomers learning about MUD economics!"
        }
    }
}

-- Items that can be found or purchased
MudletTutorial.items = {
    welcome_sign = {
        name = "a welcome sign",
        description = "A wooden sign with carved letters that reads: 'Welcome to Mudlet Tutorial! Type HELP to get started.'",
        readable = true,
        text = [[
╔══════════════════════════════════════════════════════════════╗
║                    WELCOME TO MUDLET TUTORIAL               ║
║                                                              ║
║  This interactive tutorial will teach you:                  ║
║  • Basic MUD commands and gameplay                          ║
║  • Mudlet's powerful features                               ║
║  • Script creation and automation                           ║
║  • The art of command exploration ("syntax game")           ║
║                                                              ║
║  Commands to get started:                                   ║
║  • HELP - Shows available commands                          ║
║  • TUTORIAL - Shows your progress                           ║
║  • LOOK - Examine your surroundings                         ║
║  • NORTH/SOUTH/EAST/WEST - Move around                      ║
║                                                              ║
║  This is a safe environment - experiment freely!            ║
╚══════════════════════════════════════════════════════════════╝
        ]]
    },
    
    instruction_board = {
        name = "an instruction board",
        description = "A large board covered with helpful instructions for new players.",
        readable = true,
        text = [[
Basic MUD Commands:
==================
Movement: north, south, east, west (or n, s, e, w)
Look: look, look at <item>, examine <item>
Inventory: inventory (or i), get <item>, drop <item>
Communication: say <message>, tell <player> <message>
Help: help, help <topic>

Mudlet-Specific Features:
========================
To create a trigger: Click Scripts → New → Trigger
To create an alias: Click Scripts → New → Alias  
To run Lua code: Type "lua <code>" in the command line
To see script editor: Press F4

Advanced Tips:
=============
• Use TAB completion for commands
• Try variations of commands to learn syntax
• Read in-game help files for detailed information
• Experiment safely in this tutorial environment
        ]]
    },
    
    wooden_bench = {
        name = "a comfortable wooden bench",
        description = "A well-crafted wooden bench that looks perfect for resting and contemplation.",
        usable = true,
        use_message = "You sit on the comfortable bench and feel refreshed. Your health is restored to maximum."
    },
    
    practice_sword = {
        name = "a practice sword",
        description = "A wooden sword designed for safe training. It won't hurt anyone, but great for learning combat commands.",
        takeable = true,
        weapon = true,
        use_message = "You practice some sword forms with the wooden blade."
    },
    
    mudlet_manual = {
        name = "the official Mudlet manual",
        description = "A comprehensive guide to using Mudlet effectively.",
        readable = true,
        text = "This manual contains everything you need to know about Mudlet. In the real world, you can find it at: https://wiki.mudlet.org/"
    },
    
    glowing_crystal = {
        name = "a glowing crystal",
        description = "A mysterious crystal that pulses with magical energy. It seems to respond to Mudlet scripts.",
        special = "script_trigger",
        usable = true
    },
    
    health_potion = {
        name = "a health potion",
        description = "A small bottle filled with a red liquid that glows faintly.",
        takeable = true,
        usable = true,
        use_message = "You drink the health potion and feel much better! (+50 health)"
    }
}

-- Tutorial command processor
function MudletTutorial.processCommand(command)
    command = command:lower():trim()
    
    -- Handle movement commands
    local exits = {"north", "south", "east", "west", "up", "down", "n", "s", "e", "w", "u", "d"}
    local exitMap = {n="north", s="south", e="east", w="west", u="up", d="down"}
    
    local direction = exitMap[command] or command
    
    if table.contains(exits, direction) then
        return MudletTutorial.handleMovement(direction)
    end
    
    -- Handle tutorial-specific commands
    if command == "help" then
        return MudletTutorial.showHelp()
    elseif command == "tutorial" then
        return MudletTutorial.showProgress()
    elseif command:starts("look") then
        return MudletTutorial.handleLook(command)
    elseif command:starts("examine") or command:starts("exam") then
        return MudletTutorial.handleExamine(command)
    elseif command:starts("read") then
        return MudletTutorial.handleRead(command)
    elseif command:starts("get") or command:starts("take") then
        return MudletTutorial.handleGet(command)
    elseif command:starts("inventory") or command == "i" then
        return MudletTutorial.showInventory()
    elseif command:starts("use") then
        return MudletTutorial.handleUse(command)
    elseif command:starts("say") then
        return MudletTutorial.handleSay(command)
    elseif command:starts("tell") then
        return MudletTutorial.handleTell(command)
    elseif command:starts("ask") then
        return MudletTutorial.handleAsk(command)
    elseif command:starts("practice") then
        return MudletTutorial.handlePractice(command)
    elseif command:starts("buy") then
        return MudletTutorial.handleBuy(command)
    elseif command:starts("list") then
        return MudletTutorial.handleList(command)
    else
        return MudletTutorial.handleUnknownCommand(command)
    end
end

-- Movement handling
function MudletTutorial.handleMovement(direction)
    local currentRoom = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    
    if not currentRoom.exits[direction] then
        return "You can't go that way."
    end
    
    local newRoomId = currentRoom.exits[direction]
    MudletTutorial.state.currentRoom = newRoomId
    
    -- Mark room as visited
    MudletTutorial.state.roomsVisited[newRoomId] = true
    
    -- Check if this completes any lesson objectives
    MudletTutorial.checkLessonProgress("movement", newRoomId)
    
    return MudletTutorial.describeCurrentRoom()
end

-- Room description
function MudletTutorial.describeCurrentRoom()
    local room = MudletTutorial.rooms[MudletTutorial.state.currentRoom]
    local output = string.format("%s\n%s\n", room.name, string.rep("=", string.len(room.name)))
    output = output .. room.description .. "\n"
    
    -- Show exits
    local exitList = {}
    for dir, _ in pairs(room.exits) do
        table.insert(exitList, dir)
    end
    if #exitList > 0 then
        output = output .. "\nObvious exits: " .. table.concat(exitList, ", ") .. "\n"
    end
    
    -- Show items
    if room.items and #room.items > 0 then
        output = output .. "\nYou see:\n"
        for _, itemId in ipairs(room.items) do
            local item = MudletTutorial.items[itemId]
            if item then
                output = output .. "  " .. item.name .. "\n"
            end
        end
    end
    
    -- Show NPCs
    if room.npcs and #room.npcs > 0 then
        for _, npcId in ipairs(room.npcs) do
            local npc = MudletTutorial.npcs[npcId]
            if npc then
                output = output .. npc.name .. " is here.\n"
            end
        end
    end
    
    return output
end

-- Initialize the tutorial
function MudletTutorial.initialize()
    -- Display welcome message with proper formatting
    local welcomeText = [[
╔══════════════════════════════════════════════════════════════════════════════╗
║                            MUDLET TUTORIAL                                  ║
║                                                                              ║
║        Welcome to an interactive learning experience!                       ║
║                                                                              ║
║    This tutorial will teach you both MUD gaming basics and                  ║
║    Mudlet's powerful features in a safe, offline environment.               ║
║                                                                              ║
║    • Learn fundamental MUD commands and concepts                             ║
║    • Master Mudlet's automation features                                    ║
║    • Practice the "syntax game" of command exploration                      ║
║    • Create your first triggers, aliases, and scripts                       ║
║                                                                              ║
║    Type HELP at any time to see available commands.                         ║
║    Type TUTORIAL to check your progress.                                    ║
║                                                                              ║
║    Remember: This is a safe learning environment - experiment freely!       ║
╚══════════════════════════════════════════════════════════════════════════════╝

    ]]
    
    cecho("<yellow>" .. welcomeText)
    cecho("<white>" .. MudletTutorial.describeCurrentRoom())
    
    -- Initialize progress tracking
    MudletTutorial.state.roomsVisited[MudletTutorial.state.currentRoom] = true
    MudletTutorial.checkLessonProgress("welcome", "started")
    
    -- Show first lesson hint
    cecho("<cyan>" .. MudletTutorial.getNextLessonHint() .. "\n")
end

-- Help system
function MudletTutorial.showHelp()
    return [[
Available Commands:
==================

Movement:
  north, south, east, west (n, s, e, w) - Move in those directions
  up, down (u, d) - Move up or down

Examination:
  look - Look around the current room
  look <item> - Look at a specific item
  examine <item> - Examine an item closely  
  read <item> - Read text on an item

Inventory:
  inventory (i) - Show what you're carrying
  get <item> - Pick up an item
  take <item> - Same as get
  use <item> - Use an item

Communication:
  say <message> - Say something out loud
  tell <person> <message> - Send a private message
  ask <person> about <topic> - Ask someone about something

Tutorial Commands:
  help - Show this help message
  tutorial - Show your progress
  practice <skill> - Practice a particular skill

Shopping (in stores):
  list - Show items for sale
  buy <item> - Purchase an item

Remember: You can usually abbreviate commands and try variations!
    ]]
end

return MudletTutorial