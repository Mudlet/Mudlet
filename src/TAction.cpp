/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2021-2023, 2026 by Stephen Lyons                  *
 *                                               - slysven@virginmedia.com *
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


#include "TAction.h"


#include "ActionUnit.h"
#include "Host.h"
#include "TConsoleModel.h"
#include "TDebug.h"
#include "TEasyButtonBar.h"
#include "TLuaInterpreter.h"
#include "TToolBar.h"
#include "utils.h"

#include <QColor>
#include <QDebug>
#include <QMap>
#include <QScopeGuard>

#include <list>

TAction::TAction(TAction* parent, Host* pHost)
: Tree<TAction>(parent)
, mpHost(pHost)
{
}

TAction::TAction(const QString& name, Host* pHost)
: Tree<TAction>(nullptr)
, mpHost(pHost)
, mName(name)
{
}

TAction::~TAction()
{
    if (mpHost) {
        mpHost->getActionUnit()->unregisterAction(this);

        if (isTemporary()) {
            if (mScript.isEmpty()) {
                mpHost->mLuaInterpreter.delete_luafunction(this);
            } else {
                mpHost->mLuaInterpreter.delete_luafunction(mFuncName);
            }
        }
    }

    if (mpToolBar) {
        mpToolBar->hide();
    }

    if (mpEasyButtonBar) {
        mpEasyButtonBar->hide();
    }
}

bool TAction::registerAction()
{
    if (!mpHost) {
        qDebug() << "ERROR: TAction::registerTrigger() pHost=0";
        return false;
    }
    return mpHost->getActionUnit()->registerAction(this);
}

void TAction::compileAll()
{
    mNeedsToBeCompiled = true;
    if (!compileScript()) {
        if (TDebug::wants(TDebug::Category::Error)) {
            TDebug(Qt::white, Qt::red, TDebug::Category::Error, mName) << "ERROR: Lua compile error. compiling script of action:" << mName << "\n" >> mpHost;
        }
        mOK_code = false;
    }
    for (auto* pTActionNode : *mpMyChildrenList) {
        auto* pTAction = static_cast<TAction*>(pTActionNode);
        pTAction->compileAll();
    }
}

void TAction::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (TDebug::wants(TDebug::Category::Error)) {
                TDebug(Qt::white, Qt::red, TDebug::Category::Error, mName) << "ERROR: Lua compile error. compiling script of action:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
    for (auto* pTActionNode : *mpMyChildrenList) {
        auto* pTAction = static_cast<TAction*>(pTActionNode);
        pTAction->compile();
    }
}

bool TAction::setScript(const QString& script)
{
    if (script != mScript) {
        setDataChanged();
    }
    mScript = script;
    mNeedsToBeCompiled = true;
    mOK_code = compileScript();
    return mOK_code;
}

bool TAction::compileScript()
{
    mFuncName = qsl("Action%1").arg(QString::number(mID));
    const QString code = qsl("function %1() %2\nend").arg(mFuncName, mScript);
    if (QString error; mpHost->mLuaInterpreter.compile(code, error, qsl("Button: %1").arg(getName()))) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
    } else {
        mOK_code = false;
        setError(error);
    }
    return mOK_code;
}

void TAction::execute()
{
    if (mIsPushDownButton) {
        if (mButtonState) {
            if (!mCommandButtonDown.isEmpty()) {
                mpHost->send(mCommandButtonDown);
            }
        } else {
            if (!mCommandButtonUp.isEmpty()) {
                mpHost->send(mCommandButtonUp);
            }
        }
    } else {
        if (!mCommandButtonDown.isEmpty()) {
            mpHost->send(mCommandButtonDown);
        }
    }

    // Recorded before the compile step below, which returns early on failure
    mpHost->mainConsoleModel().mButtonState = (mButtonState ? 2 : 1);

    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            mpHost->setFocusOnHostActiveCommandLine();
            return;
        }
    }

    // Whilst this frame is on the stack ActionUnit::uninstall() must defer
    // deleting this profile's actions: the script run below can uninstall its
    // own package (e.g. a "reload package" button calling uninstallPackage())
    // and freeing this TAction mid-execute() is a use-after-free - the members
    // read after the call would be dangling. The guard defers that delete past
    // the last member access here; see ActionUnit::mProcessingDepth.
    ActionUnit* pUnit = mpHost->getActionUnit();
    pUnit->beginProcessing();
    const auto processingGuard = qScopeGuard([pUnit] {
        pUnit->endProcessing();
    });

    mpHost->mLuaInterpreter.call(mFuncName, mName);
    // move focus back to the active console / command line:
    mpHost->setFocusOnHostActiveCommandLine();
}

void TAction::setName(const QString& name)
{
    if (name != mName) {
        setDataChanged();
        mName = name;
        if (mpToolBar) {
            // Need to revise the objectName and displayed name in the titlebar
            // if floating and the main window context menu:
            mpToolBar->setName(name);
        }
    }
}

QString TAction::packageName(TAction* pTAction) const
{
    if (!pTAction) {
        return QString();
    }

    if (!pTAction->mPackageName.isEmpty()) {
        return !mpHost->mInstalledModules.contains(pTAction->mPackageName) ? pTAction->mPackageName : QString();
    }

    if (pTAction->getParent()) {
        return packageName(pTAction->getParent());
    }

    return QString();
}

QString TAction::moduleName(TAction* pTAction) const
{
    if (!pTAction) {
        return QString();
    }

    if (!pTAction->mPackageName.isEmpty()) {
        return mpHost->mInstalledModules.contains(pTAction->mPackageName) ? pTAction->mPackageName : QString();
    }

    if (pTAction->getParent()) {
        return moduleName(pTAction->getParent());
    }

    return QString();
}
