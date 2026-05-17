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


#include "EAction.h"
#include "Host.h"
#include "TConsole.h"
#include "TDebug.h"
#include "TEasyButtonBar.h"
#include "TFlipButton.h"
#include "TToolBar.h"
#include "mudlet.h"

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
        if (mudlet::smDebugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of action:" << mName << "\n" >> mpHost;
        }
        mOK_code = false;
    }
    for (auto pTAction : *mpMyChildrenList) {
        pTAction->compileAll();
    }
}

void TAction::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::smDebugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of action:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
    for (auto pTAction : *mpMyChildrenList) {
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
    QString error;
    if (mpHost->mLuaInterpreter.compile(code, error, qsl("Button: %1").arg(getName()))) {
        mNeedsToBeCompiled = false;
        mOK_code = true;
        return true;
    } else {
        mOK_code = false;
        setError(error);
        return false;
    }
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

    // Moved this to be before the testing/compilation of the script so that
    // the "command"s still work even if the script doesn't!
    mpHost->mpConsole->mButtonState = (mButtonState ? 2 : 1);

    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            mpHost->setFocusOnHostActiveCommandLine();
            return;
        }
    }

    mpHost->mLuaInterpreter.call(mFuncName, mName);
    // move focus back to the active console / command line:
    mpHost->setFocusOnHostActiveCommandLine();
}

void TAction::expandToolbar(TToolBar* pT)
{
    // The -1 is needed to compensate for the initial pre-increment to TToolBar::mItemCount
    pT->resetItemCount(mButtonFillerOffset - 1);
    for (auto pTAction : *mpMyChildrenList) {
        if (!pTAction->isActive()) {
            // This test and conditional loop abort was missing from this method
            // but is needed so that disabled buttons do not appear on
            // floating toolbars - possible future scope here to have "disabled"
            // buttons show in a "greyed-out" state... - Slysven
            continue;
        }
        const QIcon icon(pTAction->mIcon);
        const QString name = pTAction->getName();
        auto pTFlipButton = new TFlipButton(pTAction, mpHost);
        pTFlipButton->setIcon(icon);
        pTFlipButton->setText(name);
        pTFlipButton->setCheckable(pTAction->mIsPushDownButton);

        if (pTAction->mIsPushDownButton) {
            pTFlipButton->setChecked(pTAction->mButtonState);
        } else {
            // The following was added to ensure a non-Pushdown button is never
            // left in a checked state - Slysven
            pTFlipButton->setChecked(false);
        }

        pTFlipButton->setFlat(mButtonFlat);
        // This applies the CSS for THIS TAction to a CHILD's representation on the Toolbar
        pTFlipButton->setStyleSheet(css);

        if (pTAction->isFolder()) {
            auto pNewMenu = new QMenu(pT);
            // This applies the CSS for THIS TAction to a CHILD's own menu - is this right
            pNewMenu->setStyleSheet(css);
            // CHECK: Use the Child's CSS instead for a menu on it? - Slysven:
            // pNewMenu->setStyleSheet( pTAction->css );
            pTAction->insertActions(pT, pNewMenu);
            // This has been move until AFTER the child's menu has been
            // populated, it was being done straight after pNewMenu was created,
            // but I think we ought to insert the items into the menu before
            // applying the menu to the button - Slysven
            pTFlipButton->setMenu(pNewMenu);
        }

        if (pTAction->mpFButton) {
            pTAction->mpFButton->deleteLater();
        }
        pTAction->mpFButton = pTFlipButton;

        // Moved to be AFTER the pTAction->mIsFolder test as I think we ought to
        // add the button to the toolbar AFTER any menu (children) items have
        // been put on the button - Slysven
        pT->addButton(pTFlipButton);
    }
}

// This seems to be the TToolBar version of TAction::fillMenu(TEasyButtonBar *, QMenu *)
// Unlike the other this one seems to introduce an "intermediate" single menu
// item to which the sub-menu is added.
void TAction::insertActions(TToolBar* pT, QMenu* pMenu)
{
    mpToolBar = pT;
    auto pEAction = new EAction(mpHost, QIcon(mIcon), mName, mID);
    pEAction->setCheckable(mIsPushDownButton);
    pEAction->setStatusTip(mName);
    if (mpEAction) {
        mpEAction->deleteLater();
    }
    mpEAction = pEAction;
    pMenu->addAction(pEAction);

    if (isFolder()) {
        // The use of mudlet::self() here meant that the QMenu was not destroyed
        // until the mudlet instance is at the end of the application!
        // Changed to use pT, the toolbar
        auto pNewMenu = new QMenu(pT);
        pNewMenu->setStyleSheet(css);
        pEAction->setMenu(pNewMenu);

        for (auto childAction : *mpMyChildrenList) {
            childAction->insertActions(pT, pNewMenu);
        }
    }
}


void TAction::expandToolbar(TEasyButtonBar* pT)
{
    // The -1 is needed to compensate for the initial pre-increment to TEasyButtonBar::mItemCount
    pT->resetItemCount(mButtonFillerOffset - 1);
    for (auto pTAction : *mpMyChildrenList) {
        if (!pTAction->isActive()) {
            continue;
        }
        const QIcon icon(pTAction->mIcon);
        const QString name = pTAction->getName();
        auto pTFlipButton = new TFlipButton(pTAction, mpHost);
        pTFlipButton->setIcon(icon);
        pTFlipButton->setText(name);
        pTFlipButton->setCheckable(pTAction->mIsPushDownButton);

        if (pTAction->mIsPushDownButton) {
            pTFlipButton->setChecked(pTAction->mButtonState);
        } else {
            // The following was added to ensure a non-Pushdown button is never
            // left in a checked state - Slysven
            pTFlipButton->setChecked(false);
        }

        pTFlipButton->setFlat(mButtonFlat);
        // This applies the CSS for THIS TAction to a CHILD's representation on the Toolbar
        pTFlipButton->setStyleSheet(css);

        //FIXME: Heiko April 2012: only run checkbox button scripts, but run them even if unchecked
        if (pTAction->mIsPushDownButton && mpHost->mIsProfileLoadingSequence) {
            qDebug() << "expandToolBar() name=" << pTAction->mName << " executing script";
            pTAction->execute();
        }


        if (pTAction->isFolder()) {
            auto pNewMenu = new QMenu(pTFlipButton);

            // This applied the CSS for THIS TAction to a CHILD's own menu - is this right
            // CHECK: consider using the Child's CSS instead for a menu on it
            // - Slysven:
            // pNewMenu->setStyleSheet( pTAction->css );
            pNewMenu->setStyleSheet(css);

            pTAction->fillMenu(pT, pNewMenu);

            // This has been moved until AFTER the child's menu has been
            // populated, it was being done straight after pNewMenu was created,
            // but I think we ought to insert the items into the menu before
            // applying the menu to the button - Slysven
            pTFlipButton->setMenu(pNewMenu);
        }

        if (pTAction->mpFButton) {
            pTAction->mpFButton->deleteLater();
        }
        pTAction->mpFButton = pTFlipButton;

        // Moved to be AFTER the pTAction->mIsFolder test as I think we ought to
        // add the button to the toolbar AFTER any menu (children) items have
        // been put on the button - Slysven
        pT->addButton(pTFlipButton);
    }
}

// This seems to be the second half of TEasyButtonBar version of:
//   TAction::insertActions( TToolBar *, QMenu * )
// the need for the split is not yet clear to me! - Slysven
void TAction::fillMenu(TEasyButtonBar* pT, QMenu* pMenu)
{
    for (auto pTAction : *mpMyChildrenList) {
        if (!pTAction->isActive()) {
            continue;
        }
        mpEasyButtonBar = pT;
        auto pEAction = new EAction(mpHost, QIcon(mIcon), pTAction->mName, pTAction->mID);
        pEAction->setStatusTip(pTAction->mName);
        pEAction->setCheckable(pTAction->mIsPushDownButton);
        if (pTAction->mIsPushDownButton) {
            pEAction->setChecked(pTAction->mButtonState);
        } else {
            pEAction->setChecked(false);
        }

        if (pTAction->mpEAction) {
            pTAction->mpEAction->deleteLater();
        }
        pTAction->mpEAction = pEAction;

        //FIXME: Heiko April 2012 -> expandToolBar()
        if (pTAction->mIsPushDownButton && mpHost->mIsProfileLoadingSequence) {
            pTAction->execute();
        }

        if (pTAction->isFolder()) {
            // Adding a QWidget derived pointer to new QMenu() means the menu
            // will be destroyed when the pointed to item is, we just need to
            // find the item that it is attached to - ah ha, try the toolbar...
            auto pNewMenu = new QMenu(pT);
            pEAction->setMenu(pNewMenu);

            // CHECK: consider using the Child's CSS instead for a menu on it
            // - Slysven:
            // pNewMenu->setStyleSheet( pTAction->css );
            pNewMenu->setStyleSheet(css);

            pTAction->fillMenu(pT, pNewMenu);
        }

        // Menu is PARENT'S menu pEAction, this line moved to be AFTER child builds its own menu if it is a folder
        pMenu->addAction(pEAction);
    }
}

// This only has code corresponding to the first part of:
//   TAction::insertActions( TToolBar * pT, QMenu * pMenu )
void TAction::insertActions(TEasyButtonBar* pT, QMenu* pMenu)
{
    mpEasyButtonBar = pT;
    auto pEAction = new EAction(mpHost, QIcon(mIcon), mName, mID);
    pEAction->setCheckable(mIsPushDownButton);
    pEAction->setStatusTip(mName);
    if (mpEAction) {
        mpEAction->deleteLater();
    }
    mpEAction = pEAction;
    Q_ASSERT_X(pMenu, "TAction::insertActions(TEasyButtonBar*, QMenu*)", "method called with a NULL QMenu pointer!");
    pMenu->addAction(pEAction);
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
