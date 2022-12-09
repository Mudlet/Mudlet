/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2021-2022 by Stephen Lyons                        *
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
        if (mudlet::debugMode) {
            TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of action:" << mName << "\n" >> mpHost;
        }
        mOK_code = false;
    }
    for (auto action : *mpMyChildrenList) {
        action->compileAll();
    }
}

void TAction::compile()
{
    if (mNeedsToBeCompiled) {
        if (!compileScript()) {
            if (mudlet::debugMode) {
                TDebug(Qt::white, Qt::red) << "ERROR: Lua compile error. compiling script of action:" << mName << "\n" >> mpHost;
            }
            mOK_code = false;
        }
    }
    for (auto action : *mpMyChildrenList) {
        action->compile();
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
    mFuncName = QString("Action") + QString::number(mID);
    QString code = QString("function ") + mFuncName + QString("()\n") + mScript + QString("\nend\n");
    QString error;
    if (mpHost->mLuaInterpreter.compile(code, error, QString("Button: ") + getName())) {
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
            mpHost->setFocusOnHostMainConsole();
            return;
        }
    }

    mpHost->mLuaInterpreter.call(mFuncName, mName);
    // move focus back to the active console / command line:
    mpHost->setFocusOnHostMainConsole();
}

void TAction::expandToolbar(TToolBar* pT)
{
    for (auto action : *mpMyChildrenList) {
        if (!action->isActive()) {
            // This test and conditional loop abort was missing from this method
            // but is needed so that disabled buttons do not appear on
            // floating toolbars - possible future scope here to have "disabled"
            // buttons show in a "greyed-out" state... - Slysven
            continue;
        }
        QIcon icon(action->mIcon);
        QString name = action->getName();
        auto button = new TFlipButton(action, mpHost);
        button->setIcon(icon);
        button->setText(name);
        button->setCheckable(action->mIsPushDownButton);

        if (action->mIsPushDownButton) {
            button->setChecked(action->mButtonState);
        } else {
            // The following was added to ensure a non-Pushdown button is never
            // left in a checked state - Slysven
            button->setChecked(false);
        }

        button->setFlat(mButtonFlat);
        // This applies the CSS for THIS TAction to a CHILD's representation on the Toolbar
        button->setStyleSheet(css);

        /*
         * CHECK: The other expandToolbar(...) has the following in this position:
         *       //FIXME: Heiko April 2012: only run checkbox button scripts, but run them even if unchecked
         *       if( action->mIsPushDownButton && mpHost->mIsProfileLoadingSequence )
         *       {
         *          qDebug()<<"expandToolBar() name="<<action->mName<<" executing script";
         *          action->execute();
         *       }
         * Why does it have this and we do not? - Slysven
         */

        if (action->isFolder()) {
            auto newMenu = new QMenu(pT);
            // This applies the CSS for THIS TAction to a CHILD's own menu - is this right
            newMenu->setStyleSheet(css);
            // CHECK: Use the Child's CSS instead for a menu on it? - Slysven:
            // newMenu->setStyleSheet( action->css );
            action->insertActions(pT, newMenu);
            // This has been move until AFTER the child's menu has been
            // populated, it was being done straight after newMenu was created,
            // but I think we ought to insert the items into the menu before
            // applying the menu to the button - Slysven
            button->setMenu(newMenu);
        }

        if (action->mpFButton) {
            action->mpFButton->deleteLater();
        }
        action->mpFButton = button;

        // Moved to be AFTER the action->mIsFolder test as I think we ought to
        // add the button to the toolbar AFTER any menu (children) items have
        // been put on the button - Slysven
        pT->addButton(button);
    }
}

// This seems to be the TToolBar version of TAction::fillMenu(TEasyButtonBar *, QMenu *)
// Unlike the other this one seems to introduce an "intermediate" single menu
// item to which the sub-menu is added.
void TAction::insertActions(TToolBar* pT, QMenu* menu)
{
    mpToolBar = pT;
    auto action = new EAction(mpHost, QIcon(mIcon), mName, mID);
    action->setCheckable(mIsPushDownButton);
    action->setStatusTip(mName);
    if (mpEButton) {
        mpEButton->deleteLater();
    }
    mpEButton = action;
    menu->addAction(action);

    if (isFolder()) {
        // The use of mudlet::self() here meant that the QMenu was not destroyed
        // until the mudlet instance is at the end of the application!
        // Changed to use pT, the toolbar
        auto newMenu = new QMenu(pT);
        newMenu->setStyleSheet(css);
        action->setMenu(newMenu);

        for (auto childAction : *mpMyChildrenList) {
            childAction->insertActions(pT, newMenu);
        }
    }
}


void TAction::expandToolbar(TEasyButtonBar* pT)
{
    for (auto action : *mpMyChildrenList) {
        if (!action->isActive()) {
            continue;
        }
        QIcon icon(action->mIcon);
        QString name = action->getName();
        auto button = new TFlipButton(action, mpHost);
        button->setIcon(icon);
        button->setText(name);
        button->setCheckable(action->mIsPushDownButton);

        if (action->mIsPushDownButton) {
            button->setChecked(action->mButtonState);
        } else {
            // The following was added to ensure a non-Pushdown button is never
            // left in a checked state - Slysven
            button->setChecked(false);
        }

        button->setFlat(mButtonFlat);
        // This applies the CSS for THIS TAction to a CHILD's representation on the Toolbar
        button->setStyleSheet(css);

        //FIXME: Heiko April 2012: only run checkbox button scripts, but run them even if unchecked
        if (action->mIsPushDownButton && mpHost->mIsProfileLoadingSequence) {
            qDebug() << "expandToolBar() name=" << action->mName << " executing script";
            action->execute();
        }


        if (action->isFolder()) {
            // This applied the CSS for THIS TAction to a CHILD's own menu - is this right
            auto newMenu = new QMenu(button);

            // CHECK: consider using the Child's CSS instead for a menu on it
            // - Slysven:
            // newMenu->setStyleSheet( action->css );
            newMenu->setStyleSheet(css);

            // This has been moved until AFTER the child's menu has been
            // populated, it was being done straight after newMenu was created,
            // but I think we ought to insert the items into the menu before
            // applying the menu to the button - Slysven
            action->fillMenu(pT, newMenu);

            button->setMenu(newMenu);
        }

        if (action->mpFButton) {
            action->mpFButton->deleteLater();
        }
        action->mpFButton = button;

        // Moved to be AFTER the action->mIsFolder test as I think we ought to
        // add the button to the toolbar AFTER any menu (children) items have
        // been put on the button - Slysven
        pT->addButton(button);
    }
}

// This seems to be the second half of TEasyButtonBar version of:
//   TAction::insertActions( TToolBar *, QMenu * )
// the need for the split is not yet clear to me! - Slysven
void TAction::fillMenu(TEasyButtonBar* pT, QMenu* menu)
{
    for (auto action : *mpMyChildrenList) {
        if (!action->isActive()) {
            continue;
        }
        mpEasyButtonBar = pT;
        auto newAction = new EAction(mpHost, QIcon(mIcon), action->mName, mID);
        newAction->setStatusTip(action->mName);
        newAction->setCheckable(action->mIsPushDownButton);
        if (action->mIsPushDownButton) {
            newAction->setChecked(action->mButtonState);
        } else {
            newAction->setChecked(false);
        }

        if (action->mpEButton) {
            action->mpEButton->deleteLater();
        }
        action->mpEButton = newAction;

        //FIXME: Heiko April 2012 -> expandToolBar()
        if (action->mIsPushDownButton && mpHost->mIsProfileLoadingSequence) {
            action->execute();
        }

        if (action->isFolder()) {
            // Adding a QWidget derived pointer to new QMenu() means the menu
            // will be destroyed when the pointed to item is, we just need to
            // find the item that it is attached to - ah ha, try the toolbar...
            auto newMenu = new QMenu(pT);
            newAction->setMenu(newMenu);

            // CHECK: consider using the Child's CSS instead for a menu on it
            // - Slysven:
            // newMenu->setStyleSheet( action->css );
            newMenu->setStyleSheet(css);

            action->fillMenu(pT, newMenu);
        }

        // Menu is PARENT'S menu newAction, this line moved to be AFTER child builds its own menu if it is a folder
        menu->addAction(newAction);
    }
}

// This only has code corresponding to the first part of:
//   TAction::insertActions( TToolBar * pT, QMenu * menu )
void TAction::insertActions(TEasyButtonBar* pT, QMenu* menu)
{
    mpEasyButtonBar = pT;
    auto action = new EAction(mpHost, QIcon(mIcon), mName, mID);
    action->setCheckable(mIsPushDownButton);
    action->setStatusTip(mName);
    if (mpEButton) {
        mpEButton->deleteLater();
    }
    mpEButton = action;
    Q_ASSERT_X(menu, "TAction::insertActions(TEasyButtonBar*, QMenu*)", "method called with a NULL QMenu pointer!");
    menu->addAction(action);
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
