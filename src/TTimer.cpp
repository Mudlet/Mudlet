/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2019 by Stephen Lyons - slysven@virginmedia.com         *
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


#include "TTimer.h"


#include "Host.h"
#include "TDebug.h"
#include "mudlet.h"


using namespace std;

const char* TTimer::scmProperty_HostName = "HostName";
const char* TTimer::scmProperty_TTimerId = "TTimerId";

TTimer::TTimer(TTimer* parent, Host* pHost)
: Tree<TTimer>(parent)
, mRegisteredAnonymousLuaFunction(false)
, exportItem(true)
, mModuleMasterFolder(false)
, mpHost(pHost)
, mNeedsToBeCompiled(true)
, mpQTimer(new QTimer)
, mModuleMember(false)
{
    mpQTimer->stop();
    mpQTimer->setProperty(scmProperty_HostName, mpHost->getName());
    mpHost->getTimerUnit()->mQTimerSet.insert(mpQTimer);
    mpQTimer->setProperty(scmProperty_TTimerId, 0);
}

TTimer::TTimer(const QString& name, QTime time, Host* pHost)
: Tree<TTimer>(nullptr)
, mRegisteredAnonymousLuaFunction(false)
, exportItem(true)
, mModuleMasterFolder(false)
, mName(name)
, mTime(time)
, mpHost(pHost)
, mNeedsToBeCompiled(true)
, mpQTimer(new QTimer)
, mModuleMember(false)
{
    mpQTimer->stop();
    mpQTimer->setProperty(scmProperty_HostName, mpHost->getName());
    mpHost->getTimerUnit()->mQTimerSet.insert(mpQTimer);
    mpQTimer->setProperty(scmProperty_TTimerId, 0);
}

TTimer::~TTimer()
{
    mpQTimer->stop();
    if (mpHost) {
        mpHost->getTimerUnit()->unregisterTimer(this);
    }

    mpQTimer->deleteLater();
}

void TTimer::setName(const QString& name)
{
    // temp timers do not need to check for names referring to multiple
    // timer objects as names=ID -> much faster tempQTimer creation
    if (!isTemporary()) {
        mpHost->getTimerUnit()->mLookupTable.remove(mName, this);
    }
    mName = name;
    // Merely for information if needed later:
    mpQTimer->setObjectName(QStringLiteral("timer(Host:%1)(TTimerId:%2)").arg(mpHost->getName(), name));
    mpHost->getTimerUnit()->mLookupTable.insertMulti(name, this);
}

void TTimer::setTime(QTime time)
{
    QMutexLocker locker(&mLock);
    // Stop the timer before doing anything else:
    mpQTimer->stop();
    mTime = time;
    mpQTimer->setInterval(time.msecsSinceStartOfDay());
}

// children of folder = regular timers
// children of timers = offset timers
//     offset timers: -> their time interval is interpreted as an offset to their parent timer
bool TTimer::isOffsetTimer()
{
    if (mpParent) {
        return !mpParent->isFolder();
    } else {
        return false;
    }
}

bool TTimer::setIsActive(bool b)
{
    bool condition1 = Tree<TTimer>::setIsActive(b);
    bool condition2 = canBeUnlocked();
    if (condition1 && condition2) {
        start();
    } else {
        stop();
    }
    return condition1 && condition2;
}


void TTimer::start()
{
    mpQTimer->setSingleShot(isTemporary());

    if (!isFolder()) {
        mpQTimer->start();
    } else {
        stop();
    }
}

void TTimer::stop()
{
    mpQTimer->stop();
}

void TTimer::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::debugMode) {
                TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of timer:" << mName << "\n" >> 0;
            }
            mOK_code = false;
        }
    }
    for (auto timer : *mpMyChildrenList) {
        timer->compile();
    }
}

void TTimer::compileAll()
{
    mNeedsToBeCompiled = true;
    if (!compileScript()) {
        if (mudlet::debugMode) {
            TDebug(QColor(Qt::white), QColor(Qt::red)) << "ERROR: Lua compile error. compiling script of timer:" << mName << "\n" >> 0;
        }
        mOK_code = false;
    }
    for (auto timer : *mpMyChildrenList) {
        timer->compileAll();
    }
}

bool TTimer::setScript(const QString& script)
{
    mScript = script;
    if (script.isEmpty()) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
    } else {
        mNeedsToBeCompiled = true;
        mOK_code = compileScript();
    }
    return mOK_code;
}

bool TTimer::compileScript()
{
    mFuncName = QStringLiteral("Timer%1").arg(mID);
    QString code = QStringLiteral("function %1()\n%2\nend\n").arg(mFuncName, mScript);
    QString error;
    if (mpHost->mLuaInterpreter.compile(code, error, QStringLiteral("Timer: %1").arg(mName))) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
    } else {
        mOK_code = false;
        setError(error);
    }
    return mOK_code;
}

bool TTimer::checkRestart()
{
    return (!isTemporary() && !isOffsetTimer() && isActive() && !isFolder());
}

void TTimer::execute()
{
    if (!isActive() || isFolder()) {
        mpQTimer->stop();
        return;
    }

    if (isTemporary()) {
        if (mScript.isEmpty()) {
            mpHost->mLuaInterpreter.call_luafunction(this);
        } else {
            mpHost->mLuaInterpreter.compileAndExecuteScript(mScript);
        }
        mpQTimer->stop();
        mpHost->getTimerUnit()->markCleanup(this);
        return;
    }

    if ((!isFolder() && hasChildren()) || (isOffsetTimer())) {
        for (auto timer : *mpMyChildrenList) {
            if (timer->isOffsetTimer()) {
                timer->enableTimer(timer->getID());
            }
        }
        if (isOffsetTimer()) {
            disableTimer(mID);
            deactivate();
        }
    }

    if (!mCommand.isEmpty()) {
        mpHost->send(mCommand);
    }

    if (!mScript.isEmpty()) {
        if (mNeedsToBeCompiled) {
            if (!compileScript()) {
                disableTimer();
                return;
            }
        }

        if (!mpHost->mLuaInterpreter.call(mFuncName, mName, (mTime < mpHost->mTimerDebugOutputSuppressionInterval))) {

            mpQTimer->stop();
        }
    }
}

bool TTimer::canBeUnlocked()
{
    if (shouldBeActive()) {
        if (!mpParent) {
            return true;
        } else {
            return mpParent->canBeUnlocked();
        }
    } else {
        return false;
    }
}

void TTimer::enableTimer(int id)
{
    if (mID == id) {
        if (canBeUnlocked()) {
            if (activate()) {
                // CHECKME: Should this not also check for a non-empty "command" as well?
                if (!mScript.isEmpty()) {
                    mpQTimer->start();
                }
            } else {
                deactivate();
                mpQTimer->stop();
            }
        }
    }

    if (isFolder()) {
        for (auto timer : *mpMyChildrenList) {
            if (!timer->isOffsetTimer()) {
                timer->enableTimer(timer->getID());
            }
        }
    }
}

void TTimer::disableTimer(int id)
{
    if (mID == id) {
        deactivate();
        mpQTimer->stop();
    }

    for (auto timer : *mpMyChildrenList) {
        if (!timer->isOffsetTimer() && timer->shouldBeActive()) {
            timer->disableTimer(timer->getID());
        }
    }
}

void TTimer::enableTimer()
{
    if (canBeUnlocked()) {
        if (activate()) {
            // CHECKME: Should this not also check for a non-empty "command" as well?
            if (!mScript.isEmpty()) {
                mpQTimer->start();
            }
        } else {
            deactivate();
            mpQTimer->stop();
        }
    }
    if (!isOffsetTimer()) {
        for (auto timer : *mpMyChildrenList) {
            if (!timer->isOffsetTimer()) {
                timer->enableTimer();
            }
        }
    }
}

void TTimer::disableTimer()
{
    deactivate();
    mpQTimer->stop();
    for (auto timer : *mpMyChildrenList) {
        timer->disableTimer();
    }
}


void TTimer::enableTimer(const QString& name)
{
    if (mName == name) {
        if (canBeUnlocked()) {
            if (activate()) {
                mpQTimer->start();
            } else {
                deactivate();
                mpQTimer->stop();
            }
        }
    }

    if (!isOffsetTimer()) {
        for (auto timer : *mpMyChildrenList) {
            timer->enableTimer(timer->getName());
        }
    }
}

void TTimer::disableTimer(const QString& name)
{
    if (mName == name) {
        deactivate();
        mpQTimer->stop();
    }

    for (auto timer : *mpMyChildrenList) {
        timer->disableTimer(timer->getName());
    }
}


void TTimer::killTimer()
{
    deactivate();
    mpQTimer->stop();
}

void TTimer::setID(const int id)
{
    Tree<TTimer>::setID(id);
    mpQTimer->setProperty(scmProperty_TTimerId, id);
}
