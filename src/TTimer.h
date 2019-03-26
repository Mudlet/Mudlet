#ifndef MUDLET_TTIMER_H
#define MUDLET_TTIMER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia,com         *
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


#include "Tree.h"


#include "pre_guard.h"
#include <QMutex>
#include <QPointer>
#include <QTime>
#include "post_guard.h"

class Host;

class QTimer;


class TTimer : public Tree<TTimer>
{
    friend class TimerUnit;
    friend class XMLexport;
    friend class XMLimport;

public:
    ~TTimer();
    TTimer(TTimer* parent, Host* pHost);
    TTimer(const QString& name, QTime time, Host* pHost);
    void compileAll();
    QString& getName() { return mName; }
    void setName(const QString& name);
    QTime& getTime() { return mTime; }
    void compile();
    bool checkRestart();
    bool compileScript();
    void execute();
    void setTime(QTime time);
    QString getCommand() { return mCommand; }
    void setCommand(const QString& cmd) { mCommand = cmd; }
    QString getScript() { return mScript; }
    bool setScript(const QString& script);
    bool canBeUnlocked();
    bool setIsActive(bool);
    void stop();
    void start();
    void enableTimer();
    void disableTimer();
    void enableTimer(const QString&);
    void disableTimer(const QString&);
    void enableTimer(int);
    void disableTimer(int);
    void killTimer();

    bool isOffsetTimer();
    QPointer<Host> getHost() { return mpHost; }
    QTimer* getQTimer() { return mpQTimer; }
    // Override the Tree version as we need to insert the id number as a
    // property into the mQTimer as well:
    void setID(const int);


    // specifies whenever the payload is Lua code as a string
    // or a function
    bool mRegisteredAnonymousLuaFunction;
    bool exportItem;
    bool mModuleMasterFolder;

    static const char* scmProperty_HostName;
    static const char* scmProperty_TTimerId;

private:
    TTimer() = default;

    QString mName;
    QString mScript;
    QTime mTime;
    QString mCommand;
    QString mFuncName;
    QPointer<Host> mpHost;
    bool mNeedsToBeCompiled;
    QMutex mLock;
    // Renamed to reduce confusion:
    QTimer* mpQTimer;
    bool mModuleMember;
};

#endif // MUDLET_TTIMER_H
