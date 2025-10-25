#ifndef MUDLET_TTIMER_H
#define MUDLET_TTIMER_H

/***************************************************************************
 *   Copyright (C) 2008-2012 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2024 by Stephen Lyons - slysven@virginmedia.com         *
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


#include <QDebug>
#include <QPointer>
#include <QTime>

class Host;

class QTimer;


class TTimer : public Tree<TTimer>
{
    friend class TimerUnit;
    friend class XMLexport;
    friend class XMLimport;

public:
    ~TTimer() override;
    TTimer(TTimer* parent, Host* pHost);
    TTimer(const QString& name, QTime time, Host* pHost, bool repeating = false);
    void compileAll();
    const QString& getName() const { return mName; }
    void setName(const QString& name);
    const QTime& getTime() const { return mTime; }
    void compile();
    bool checkRestart();
    bool compileScript();
    void execute();
    void setTime(QTime time);
    const QString& getCommand() const { return mCommand; }
    void setCommand(const QString& cmd) { mCommand = cmd; }
    const QString& getScript() const { return mScript; }
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
    int remainingTime();
    // children of folder = regular timers
    // children of timers = offset timers
    //     offset timers: -> their time interval is interpreted as an offset to their parent timer
    bool isOffsetTimer()
    {
        if (mpParent) {
            return !mpParent->isFolder();
        }
        return false;
    }
    // Offset timers do not work correctly with the isAncestorsActive() base method
    bool shouldAncestorsBeActive() const {
        TTimer* node(mpParent);
        while (node) {
            if (node->isOffsetTimer() ? !node->shouldBeActive() : !node->isActive()) {
                return false;
            }
            node = node->mpParent;
        }
        return true;
    }

    QPointer<Host> getHost() { return mpHost; }
    QTimer* getQTimer() { return mpQTimer; }
    // Override the Tree version as we need to insert the id number as a
    // property into the QTimer that mpQTimer points to as well:
    void setID(int) override;
    QString packageName(TTimer* pTimer);
    QString moduleName(TTimer* pTimer);



    // specifies whenever the payload is Lua code as a string
    // or a function
    bool mRegisteredAnonymousLuaFunction = false;
    bool exportItem = true;
    bool mModuleMasterFolder = false;

    static const char* scmProperty_HostName;
    static const char* scmProperty_TTimerId;

    // temporary timers are single-shot by default, unless repeating is set
    bool mRepeating = false;

private:
    TTimer() = default;
    QString mName;
    QString mScript;
    QTime mTime;
    QString mCommand;
    QString mFuncName;
    QPointer<Host> mpHost;
    bool mNeedsToBeCompiled = true;
    QTimer* mpQTimer;
    bool mModuleMember = false;
};

#ifndef QT_NO_DEBUG_STREAM
inline QDebug& operator<<(QDebug& debug, const TTimer* timer)
{
    QDebugStateSaver saver(debug);
    Q_UNUSED(saver)
    debug.nospace() << "TTimer("
                    << "name= " << timer->getName()
                    << " time= " << timer->getTime()
                    << " command= " << timer->getCommand()
                    << " script is in= " << (timer->mRegisteredAnonymousLuaFunction ? "string" : "Lua function")
                    << " script= " << timer->getScript()
                    << " repeating= " << timer->mRepeating
                    << ")";
    return debug;
}
#endif // QT_NO_DEBUG_STREAM

#endif // MUDLET_TTIMER_H
