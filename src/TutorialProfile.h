#ifndef TUTORIALPROFILE_H
#define TUTORIALPROFILE_H

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

#include "Host.h"
#include "ctelnet.h"

#include "pre_guard.h"
#include <QObject>
#include <QString>
#include "post_guard.h"

class TutorialProfile : public QObject
{
    Q_OBJECT

public:
    explicit TutorialProfile(Host* host);
    ~TutorialProfile();

    // Initialize the tutorial profile with default content
    void initializeTutorialContent();
    
    // Handle incoming user commands
    void handleUserCommand(const QString& command);
    
    // Handle tutorial-specific triggers
    void handleTutorialTrigger(const QString& triggerName);
    
    // Validate user's work in the tutorial
    bool validateUserWork(const QString& task, const QString& userSolution);
    
    // Progress tracking
    void markTaskCompleted(const QString& taskName);
    bool isTaskCompleted(const QString& taskName) const;
    
    // Get tutorial messages
    QString getWelcomeMessage() const;
    QString getTaskDescription(const QString& taskName) const;
    QString getHintForTask(const QString& taskName) const;

private:
    Host* mpHost;
    QMap<QString, bool> mCompletedTasks;
    
    // Tutorial content
    void setupTutorialTriggers();
    void setupTutorialAliases();
    void setupTutorialScripts();
    void setupTutorialTimers();
    
    // Tutorial tasks
    void setupTask1(); // Basic movement
    void setupTask2(); // Creating a trigger
    void setupTask3(); // Creating an alias
    void setupTask4(); // Creating a script
    void setupTask5(); // Using the mapper
    
    // Tutorial responses
    void sendTutorialResponse(const QString& response);
    void sendTaskCompletionMessage(const QString& taskName);
};

#endif // TUTORIALPROFILE_H
