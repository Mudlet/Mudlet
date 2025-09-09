/***************************************************************************
 *   Copyright (C) 2025 by Mudlet Team                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "TutorialProfile.h"

// Include Qt headers first to avoid conflicts
#include "pre_guard.h"
#include <QMap>
#include <QTime>
#include <QStringList>
#include "post_guard.h"

// Include the necessary header files for the tutorial profile implementation
#include "Host.h"
#include "TTrigger.h"
#include "TAlias.h"
#include "TScript.h"
#include "TTimer.h"
// Include map-related headers last to avoid conflicts
#include "TMap.h"
#include "TRoom.h"
#include "TArea.h"

TutorialProfile::TutorialProfile(Host* host)
    : QObject(host)
    , mpHost(host)
{
}

TutorialProfile::~TutorialProfile()
{
}

void TutorialProfile::initializeTutorialContent()
{
    // Send welcome message
    sendTutorialResponse(getWelcomeMessage());
    
    // Setup tutorial elements
    setupTutorialTriggers();
    setupTutorialAliases();
    setupTutorialScripts();
    setupTutorialTimers();
    
    // Setup initial tasks
    setupTask1();
}

void TutorialProfile::handleUserCommand(const QString& command)
{
    // Handle tutorial-specific commands
    if (command == "help") {
        sendTutorialResponse("Welcome to the Mudlet Tutorial!\n"
                           "Available tutorial commands:\n"
                           "  help - Show this help message\n"
                           "  task1 - Start task 1 (Basic Movement)\n"
                           "  task2 - Start task 2 (Creating a Trigger)\n"
                           "  task3 - Start task 3 (Creating an Alias)\n"
                           "  task4 - Start task 4 (Creating a Script)\n"
                           "  task5 - Start task 5 (Using the Mapper)\n"
                           "  progress - Show your tutorial progress\n"
                           "  hint <task> - Get a hint for a task\n"
                           "  north, south, east, west - Move in directions (for task 1)\n");
    } else if (command == "progress") {
        QString progressMessage = "Tutorial Progress:\n";
        progressMessage += "Task 1 (Basic Movement): ";
        progressMessage += isTaskCompleted("task1") ? "Completed" : "Not completed";
        progressMessage += "\n";
        progressMessage += "Task 2 (Creating a Trigger): ";
        progressMessage += isTaskCompleted("task2") ? "Completed" : "Not completed";
        progressMessage += "\n";
        progressMessage += "Task 3 (Creating an Alias): ";
        progressMessage += isTaskCompleted("task3") ? "Completed" : "Not completed";
        progressMessage += "\n";
        progressMessage += "Task 4 (Creating a Script): ";
        progressMessage += isTaskCompleted("task4") ? "Completed" : "Not completed";
        progressMessage += "\n";
        progressMessage += "Task 5 (Using the Mapper): ";
        progressMessage += isTaskCompleted("task5") ? "Completed" : "Not completed";
        progressMessage += "\n";
        sendTutorialResponse(progressMessage);
    } else if (command.startsWith("hint ")) {
        QString task = command.mid(5).trimmed();
        sendTutorialResponse(getHintForTask(task));
    } else if (command == "task1") {
        sendTutorialResponse(getTaskDescription("task1"));
    } else if (command == "task2") {
        if (isTaskCompleted("task1")) {
            sendTutorialResponse(getTaskDescription("task2"));
        } else {
            sendTutorialResponse("You need to complete Task 1 (Basic Movement) first. Type 'north' or 'n' to move north and complete Task 1.");
        }
    } else if (command == "task3") {
        if (isTaskCompleted("task2")) {
            sendTutorialResponse(getTaskDescription("task3"));
        } else {
            sendTutorialResponse("You need to complete Task 2 (Creating a Trigger) first. Type 'task2' to see instructions for creating a trigger.");
        }
    } else if (command == "task4") {
        if (isTaskCompleted("task3")) {
            sendTutorialResponse(getTaskDescription("task4"));
        } else {
            sendTutorialResponse("You need to complete Task 3 (Creating an Alias) first. Type 'task3' to see instructions for creating an alias.");
        }
    } else if (command == "task5") {
        if (isTaskCompleted("task4")) {
            sendTutorialResponse(getTaskDescription("task5"));
        } else {
            sendTutorialResponse("You need to complete Task 4 (Creating a Script) first. Type 'task4' to see instructions for creating a script.");
        }
    } else if (command == "north" || command == "n") {
        if (!isTaskCompleted("task1")) {
            markTaskCompleted("task1");
        }
        sendTutorialResponse("You move north.\n"
                           "The path leads to a small clearing where you can see a wooden sign.\n"
                           "Type 'task2' to continue with the tutorial.");
    } else if (command == "south" || command == "s") {
        sendTutorialResponse("You move south.\n"
                           "You find yourself back where you started.");
    } else if (command == "east" || command == "e") {
        sendTutorialResponse("You move east.\n"
                           "You find yourself back where you started.");
    } else if (command == "west" || command == "w") {
        sendTutorialResponse("You move west.\n"
                           "You find yourself back where you started.");
    } else if (command == "h") {
        // This is the alias test command for task 3
        if (!isTaskCompleted("task3")) {
            validateUserWork("task3", command);
        }
        // Still send the normal response for help
        sendTutorialResponse("This is the help command. In a real MUD, this would show help information.\n"
                           "In this tutorial, typing 'h' tests your alias. You've just used your alias!");
    } else if (command == "someone says hello") {
        // This is the trigger test command for task 2
        if (!isTaskCompleted("task2")) {
            validateUserWork("task2", command);
        }
        // The trigger will handle the response, but we still send a message
        sendTutorialResponse("In a real MUD, this text would trigger your response.\n"
                           "In this tutorial, we're simulating that behavior.");
    } else if (command == "lua Tutorial Script()") {
        // This is the script test command for task 4
        if (!isTaskCompleted("task4")) {
            validateUserWork("task4", command);
        }
        // The script will handle the response, but we still send a message
        sendTutorialResponse("In a real Mudlet, this would run your script.\n"
                           "In this tutorial, we're simulating that behavior.");
    } else {
        // For non-tutorial commands, simulate a MUD response
        QString responseMessage = "You entered: " + command + "\n";
        responseMessage += "In a real MUD, this command would be sent to the server.\n";
        responseMessage += "In this tutorial, we're simulating MUD responses to teach you Mudlet features.\n";
        sendTutorialResponse(responseMessage);
    }
}

void TutorialProfile::handleTutorialTrigger(const QString& triggerName)
{
    if (triggerName == "tutorial_welcome") {
        sendTutorialResponse("Welcome to the Mudlet Tutorial!\n"
                           "Type 'help' to see available tutorial commands.\n");
    } else if (triggerName == "Tutorial Detection Trigger") {
        // This is triggered when the user types "someone says hello"
        validateUserWork("task2", "someone says hello");
    } else if (triggerName == "Script Detection Trigger") {
        // This is triggered when the user runs the tutorial script
        validateUserWork("task4", "lua Tutorial Script()");
    }
}

bool TutorialProfile::validateUserWork(const QString& task, const QString& userSolution)
{
    // In a real implementation, this would check the actual user-created elements
    if (task == "task2") {
        // Check if user created a trigger with the expected pattern
        TTrigger* pTrigger = mpHost->getTriggerUnit()->findTrigger("Tutorial Trigger");
        if (pTrigger) {
            QStringList patterns = pTrigger->getPatternsList();
            if (patterns.contains("*says hello*")) {
                if (!isTaskCompleted("task2")) {
                    markTaskCompleted("task2");
                }
                return true;
            }
        }
        // Also check for the pattern the user would type in the tutorial
        if (userSolution.contains("someone says hello") && !isTaskCompleted("task2")) {
            markTaskCompleted("task2");
            return true;
        }
        return false;
    } else if (task == "task3") {
        // Check if user created an alias with the expected pattern
        TAlias* pAlias = mpHost->getAliasUnit()->findFirstAlias("Tutorial Alias");
        if (pAlias) {
            if (pAlias->getRegexCode() == "h") {
                if (!isTaskCompleted("task3")) {
                    markTaskCompleted("task3");
                }
                return true;
            }
        }
        // Also check for the pattern the user would type in the tutorial
        if (userSolution == "h" && !isTaskCompleted("task3")) {
            markTaskCompleted("task3");
            return true;
        }
        return false;
    } else if (task == "task4") {
        // Check if user created a script with the expected name
        TScript* pScript = mpHost->getScriptUnit()->getScript(mpHost->getScriptUnit()->getNewID() - 1);
        // This is a simplified check - in a real implementation we would search for a script with a specific name
        if (pScript != nullptr && !isTaskCompleted("task4")) {
            // Check if it's a tutorial script by looking for specific content
            QString scriptContent = pScript->getScript();
            if (scriptContent.contains("Hello from the tutorial script!")) {
                markTaskCompleted("task4");
                return true;
            }
        }
        // Also check for the command the user would type in the tutorial
        if (userSolution.contains("lua Tutorial Script()") && !isTaskCompleted("task4")) {
            markTaskCompleted("task4");
            return true;
        }
        return false;
    }
    return false;
}

void TutorialProfile::markTaskCompleted(const QString& taskName)
{
    mCompletedTasks[taskName] = true;
    sendTaskCompletionMessage(taskName);
}

bool TutorialProfile::isTaskCompleted(const QString& taskName) const
{
    return mCompletedTasks.value(taskName, false);
}

QString TutorialProfile::getWelcomeMessage() const
{
    return QString("========================================\n"
                   "    Welcome to the Mudlet Tutorial!\n"
                   "========================================\n"
                   "\n"
                   "This profile will teach you how to use Mudlet's features in a safe, interactive environment.\n"
                   "You can experiment with triggers, aliases, scripts, and more without worrying about making mistakes.\n"
                   "\n"
                   "Type 'help' to see available tutorial commands.\n"
                   "\n"
                   "Let's get started!\n");
}

QString TutorialProfile::getTaskDescription(const QString& taskName) const
{
    if (taskName == "task1") {
        return QString("Task 1: Basic Movement\n"
                       "=====================\n"
                       "In MUDs, you move by typing directions like 'north', 'south', 'east', 'west'.\n"
                       "Try typing 'north' or 'n' to move to the next room.\n"
                       "\n"
                       "Tip: You can also use shortcuts like 'n', 's', 'e', 'w'.\n");
    } else if (taskName == "task2") {
        return QString("Task 2: Creating a Trigger\n"
                       "=========================\n"
                       "Triggers automatically respond to text from the MUD.\n"
                       "\n"
                       "1. Click the 'Triggers' button in the toolbar\n"
                       "2. Click the 'Add Item' button\n"
                       "3. Give it a name like 'Tutorial Trigger'\n"
                       "4. In the 'Pattern' field, enter: '*says hello*'\n"
                       "5. In the 'Script' field, enter: mmp.echo('Someone said hello!')\n"
                       "6. Click 'Save Item'\n"
                       "\n"
                       "Now when you type 'someone says hello' in this tutorial, you'll see the trigger response.\n"
                       "After creating the trigger, type 'someone says hello' to test it and complete this task.\n");
    } else if (taskName == "task3") {
        return QString("Task 3: Creating an Alias\n"
                       "========================\n"
                       "Aliases let you create shortcuts for complex commands.\n"
                       "\n"
                       "1. Click the 'Aliases' button in the toolbar\n"
                       "2. Click the 'Add Item' button\n"
                       "3. Give it a name like 'Tutorial Alias'\n"
                       "4. In the 'Pattern' field, enter: h\n"
                       "5. In the 'Script' field, enter: send('help')\n"
                       "6. Click 'Save Item'\n"
                       "\n"
                       "Now typing 'h' will be the same as typing 'help'.\n"
                       "After creating the alias, type 'h' to test it and complete this task.\n");
    } else if (taskName == "task4") {
        return QString("Task 4: Creating a Script\n"
                       "========================\n"
                       "Scripts are blocks of Lua code that can be executed.\n"
                       "\n"
                       "1. Click the 'Scripts' button in the toolbar\n"
                       "2. Click the 'Add Item' button\n"
                       "3. Give it a name like 'Tutorial Script'\n"
                       "4. In the script editor, enter:\n"
                       "   mmp.echo('Hello from my script!')\n"
                       "5. Click 'Save Item'\n"
                       "6. To run it, type in the command line: lua Tutorial Script()\n"
                       "\n"
                       "After creating the script, run it to complete this task.\n");
    } else if (taskName == "task5") {
        return QString("Task 5: Using the Mapper\n"
                       "=======================\n"
                       "The mapper helps you navigate MUD worlds visually.\n"
                       "\n"
                       "1. Click the 'Map' button in the toolbar\n"
                       "2. You'll see a visual representation of rooms\n"
                       "3. Click on rooms to move to them\n"
                       "4. Right-click to create new rooms\n"
                       "5. Use the 'Speedwalk' feature to automatically move between rooms\n"
                       "\n"
                       "The mapper is especially useful in complex MUDs with many areas.\n"
                       "Explore the mapper to complete this task.\n");
    }
    return QString("Unknown task: " + taskName);
}

QString TutorialProfile::getHintForTask(const QString& taskName) const
{
    if (taskName == "task1") {
        return QString("Hint for Task 1: Try typing 'north' or 'n' to move.\n");
    } else if (taskName == "task2") {
        return QString("Hint for Task 2: Look for the 'Triggers' button in the toolbar.\n"
                       "Remember to save your trigger after creating it.\n"
                       "After creating the trigger, type 'someone says hello' to test it.\n");
    } else if (taskName == "task3") {
        return QString("Hint for Task 3: Aliases are created through the 'Aliases' button.\n"
                       "The pattern is what you type, the script is what gets executed.\n"
                       "After creating the alias, type 'h' to test it.\n");
    } else if (taskName == "task4") {
        return QString("Hint for Task 4: Scripts are created through the 'Scripts' button.\n"
                       "Use the Lua scripting language in the script editor.\n"
                       "After creating the script, run it with 'lua Tutorial Script()'.\n");
    } else if (taskName == "task5") {
        return QString("Hint for Task 5: The mapper is accessed through the 'Map' button.\n"
                       "You can create rooms by right-clicking in the map view.\n");
    }
    return QString("No hint available for task: " + taskName);
}

void TutorialProfile::setupTutorialTriggers()
{
    // Setup tutorial-specific triggers
    // Create a trigger to detect when the user types "someone says hello"
    TTrigger* pTrigger = new TTrigger("Tutorial Detection Trigger", QStringList() << "someone says hello", QList<int>() << REGEX_SUBSTRING, false, mpHost);
    pTrigger->setScript("mmp.echo('Someone said hello! This is your trigger working.')\n"
                        "tutorial.markTaskCompleted('task2')");
    pTrigger->setIsActive(true);
    mpHost->getTriggerUnit()->registerTrigger(pTrigger);
    
    // Create a trigger to detect when the user runs a script
    TTrigger* pScriptTrigger = new TTrigger("Script Detection Trigger", QStringList() << "lua Tutorial Script()", QList<int>() << REGEX_SUBSTRING, false, mpHost);
    pScriptTrigger->setScript("tutorial.markTaskCompleted('task4')");
    pScriptTrigger->setIsActive(true);
    mpHost->getTriggerUnit()->registerTrigger(pScriptTrigger);
    
    // Create a trigger to detect when the user types "h" (for task 3 completion)
    TTrigger* pAliasTrigger = new TTrigger("Alias Detection Trigger", QStringList() << "^h$", QList<int>() << REGEX_EXACT_MATCH, false, mpHost);
    pAliasTrigger->setScript("mmp.echo('You used your alias! This is your alias working.')\n"
                             "tutorial.markTaskCompleted('task3')");
    pAliasTrigger->setIsActive(true);
    mpHost->getTriggerUnit()->registerTrigger(pAliasTrigger);
}

void TutorialProfile::setupTutorialAliases()
{
    // Setup tutorial-specific aliases
    // Create an alias to detect when the user types "h"
    TAlias* pAlias = new TAlias("Tutorial Detection Alias", mpHost);
    pAlias->setRegexCode("h");
    pAlias->setCommand("help");
    pAlias->setIsActive(true);
    mpHost->getAliasUnit()->registerAlias(pAlias);
}

void TutorialProfile::setupTutorialScripts()
{
    // Setup tutorial-specific scripts
    // Create a simple script that can be run
    TScript* pScript = new TScript("Tutorial Script", mpHost);
    pScript->setScript("mmp.echo('Hello from the tutorial script!')\n"
                       "mmp.echo('This is an example of how scripts work in Mudlet.')\n"
                       "mmp.echo('You can write complex Lua code here to automate tasks.')");
    pScript->setIsActive(true);
    mpHost->getScriptUnit()->registerScript(pScript);
}

void TutorialProfile::setupTutorialTimers()
{
    // Setup tutorial-specific timers
    // Create a timer that fires after 10 seconds to give hints
    TTimer* pTimer = new TTimer("Tutorial Hint Timer", QTime(0, 0, 10), mpHost, false);
    pTimer->setScript("if not tutorial.isTaskCompleted('task1') then\n"
                      "  mmp.echo('Hint: Try typing \\'north\\' or \\'n\\' to move to the next room.')\n"
                      "end");
    pTimer->setIsActive(true);
    mpHost->getTimerUnit()->registerTimer(pTimer);
    pTimer->start();
}

void TutorialProfile::setupTask1()
{
    // Setup content for task 1
    sendTutorialResponse("You are in a small forest clearing. The sun filters through the trees above.\n"
                         "To the north, you can see a path leading away from this clearing.\n"
                         "Try moving north by typing 'north' or 'n'.\n");
}

void TutorialProfile::setupTask2()
{
    // Setup content for task 2
    // This will be called when task 1 is completed
    sendTutorialResponse("Great! You've learned how to move around.\n"
                         "Now let's learn about triggers. Triggers automatically respond to text from the MUD.\n"
                         "Type 'task2' to see instructions for creating your first trigger.");
}

void TutorialProfile::setupTask3()
{
    // Setup content for task 3
    // This will be called when task 2 is completed
    sendTutorialResponse("Excellent! You've created your first trigger.\n"
                         "Now let's learn about aliases. Aliases let you create shortcuts for complex commands.\n"
                         "Type 'task3' to see instructions for creating your first alias.");
}

void TutorialProfile::setupTask4()
{
    // Setup content for task 4
    // This will be called when task 3 is completed
    sendTutorialResponse("Well done! You've created your first alias.\n"
                         "Now let's learn about scripts. Scripts are blocks of Lua code that can automate tasks.\n"
                         "Type 'task4' to see instructions for creating your first script.");
}

void TutorialProfile::setupTask5()
{
    // Setup content for task 5
    // This will be called when task 4 is completed
    sendTutorialResponse("Fantastic! You've created your first script.\n"
                         "Now let's learn about the mapper. The mapper helps you navigate MUD worlds visually.\n"
                         "Type 'task5' to see instructions for using the mapper.");
}

void TutorialProfile::sendTutorialResponse(const QString& response)
{
    if (mpHost) {
        mpHost->postMessage(response);
    }
}

void TutorialProfile::sendTaskCompletionMessage(const QString& taskName)
{
    QString message;
    if (taskName == "task1") {
        message = "Congratulations! You've completed Task 1: Basic Movement.\n"
                  "You can now move around in MUDs. Type 'task2' to continue with the tutorial.";
        setupTask2();  // Setup the next task
    } else if (taskName == "task2") {
        message = "Congratulations! You've completed Task 2: Creating a Trigger.\n"
                  "You can now automatically respond to text from the MUD. Type 'task3' to continue.";
        setupTask3();  // Setup the next task
    } else if (taskName == "task3") {
        message = "Congratulations! You've completed Task 3: Creating an Alias.\n"
                  "You can now create shortcuts for complex commands. Type 'task4' to continue.";
        setupTask4();  // Setup the next task
    } else if (taskName == "task4") {
        message = "Congratulations! You've completed Task 4: Creating a Script.\n"
                  "You can now write custom Lua code. Type 'task5' to continue.";
        setupTask5();  // Setup the next task
    } else if (taskName == "task5") {
        message = "Congratulations! You've completed Task 5: Using the Mapper.\n"
                  "You can now navigate MUDs visually.\n\n"
                  "You've completed the Mudlet Tutorial! Feel free to continue exploring Mudlet's features.\n"
                  "When you're ready, connect to a real MUD using one of the profiles in the connection dialog.";
    } else {
        message = "Task completed: " + taskName;
    }
    
    sendTutorialResponse(message);
}
