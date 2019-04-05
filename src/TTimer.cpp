/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
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

TTimer::TTimer(TTimer* parent, Host* pHost)
: Tree<TTimer>(parent)
, mRegisteredAnonymousLuaFunction(false)
, exportItem(true)
, mModuleMasterFolder(false)
, mpHost(pHost)
, mNeedsToBeCompiled(true)
, mpQTimer(new QTimer)
, mModuleMember(false)
, mKnownToBeUnregistered(false)
{
    mpQTimer->stop();
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
, mKnownToBeUnregistered(false)
{
    mpQTimer->stop();
}

TTimer::~TTimer()
{
    mpQTimer->stop();
    if (!mpHost) {
        return;
    }
    mpHost->getTimerUnit()->unregisterTimer(this);
    mudlet::self()->unregisterTimer(this);
    mpQTimer->deleteLater();
}

bool TTimer::registerTimer()
{
    if (!mpHost) {
        return false;
    }
    setTime(mTime);
    mudlet::self()->registerTimer(this);
    return mpHost->getTimerUnit()->registerTimer(this);
}

void TTimer::setName(const QString& name)
{
    // temp timers do not need to check for names referring to multiple
    // timer objects as names=ID -> much faster tempTimer creation
    if (!isTemporary()) {
        mpHost->getTimerUnit()->mLookupTable.remove(mName, this);
    }
    mName = name;
    mpHost->getTimerUnit()->mLookupTable.insertMulti(name, this);
}

void TTimer::setTime(QTime time)
{
    QMutexLocker locker(&mLock);
    mTime = time;
    mpQTimer->setInterval(mTime.msec() + (1000 * mTime.second()) + (1000 * 60 * mTime.minute()) + (1000 * 60 * 60 * mTime.hour()));
    mpQTimer->stop();
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
    bool condition2 = canBeUnlocked(nullptr);
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

bool TTimer::canBeUnlocked(TTimer* pChild)
{
    if (shouldBeActive()) {
        if (!mpParent) {
            return true;
        } else {
            return mpParent->canBeUnlocked(nullptr);
        }
    } else {
        return false;
    }
}

void TTimer::enableTimer(int id)
{
    if (mID == id) {
        if (canBeUnlocked(nullptr)) {
            if (activate()) {
                if (mScript.size() > 0) {
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
    if (canBeUnlocked(nullptr)) {
        if (activate()) {
            if (mScript.size() > 0) {
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
        if (canBeUnlocked(nullptr)) {
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
