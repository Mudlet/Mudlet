/***************************************************************************
 *   Copyright (C) 2008-2013 by Heiko Koehn - KoehnHeiko@googlemail.com    *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2019 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "TEasyButtonBar.h"

#include "EAction.h"
#include "Host.h"
#include "TAction.h"
#include "TConsole.h"
#include "TFlipButton.h"

#include <QGridLayout>
#include <QIcon>
#include <QMenu>
#include <QScopeGuard>


TEasyButtonBar::TEasyButtonBar(TAction* pA, QString name, QWidget* pW)
: QWidget(pW)
, mpTAction(pA)
, mpWidget(new QWidget(this))
{
    mButtonList.clear();
    auto hostName(pA->mpHost->getName());
    auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(mpWidget);
    if (!mpTAction->mUseCustomLayout) {
        mpLayout = new QGridLayout(mpWidget);
        setContentsMargins(0, 0, 0, 0);
        mpLayout->setContentsMargins(0, 0, 0, 0);
        mpLayout->setSpacing(0);
        const QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mpWidget->setSizePolicy(sizePolicy);
    } else {
        mpWidget->setMaximumSize(mpTAction->getSize());
        mpWidget->setMinimumSize(mpTAction->getSize());
        mpWidget->setGeometry(mpTAction->mPosX, mpTAction->mPosY, mpTAction->getSizeX(), mpTAction->getSizeY());
    }
    setStyleSheet(mpTAction->css);
    mpWidget->setStyleSheet(mpTAction->css);
    setObjectName(qsl("easyButtonBar_%1_%2").arg(hostName, name));
    mpWidget->setObjectName(qsl("easyButtonBar_Widget_%1_%2").arg(hostName, name));
    // It is not entirely clear if this is ever visible:
    setWindowTitle(tr("Easybutton Bar - %1 - %2").arg(hostName, name));
}

void TEasyButtonBar::addButton(TFlipButton* pB)
{
    if (!mpTAction->mUseCustomLayout) {
        QSize size = pB->minimumSizeHint();
        if (pB->mpTAction->getButtonRotation() > 0) {
            size.transpose();
            pB->setMaximumSize(size);
        }
    } else {
        qDebug() << "setting up custom sizes";
        const QSize size = pB->mpTAction->getSize();
        pB->setMaximumSize(size);
        pB->setMinimumSize(size);
        pB->setParent(mpWidget);
        pB->setGeometry(pB->mpTAction->mPosX, pB->mpTAction->mPosY, pB->mpTAction->getSizeX(), pB->mpTAction->getSizeY());
    }

    pB->setStyleSheet(pB->mpTAction->css);
    pB->setFlat(pB->mpTAction->getButtonFlat());
    const int rotation = pB->mpTAction->getButtonRotation();
    switch (rotation) {
    case 0:
        pB->setOrientation(Qt::Horizontal);
        break;
    case 1:
        pB->setOrientation(Qt::Vertical);
        break;
    case 2:
        pB->setOrientation(Qt::Vertical);
        pB->setMirrored(true);
        break;
    }

    if (!mpTAction->mUseCustomLayout) {
        // tool bar mButtonColumns > 0 -> autolayout
        // case == 0: use individual button placement for user defined layouts
        int columns = std::max(1, mpTAction->getButtonColumns());
        const int row = ++mItemCount / columns;
        const int col = mItemCount % columns;
        if (mVerticalOrientation) {
            mpLayout->addWidget(pB, row, col);
        } else {
            mpLayout->addWidget(pB, col, row);
        }
    } else {
        pB->move(pB->mpTAction->mPosX, pB->mpTAction->mPosY);
    }


    // Was using released() signal but now we want to track the ACTUAL state of
    // the underlying QAbstractButton
    connect(pB, &QAbstractButton::clicked, this, &TEasyButtonBar::slot_pressed);
    mButtonList.push_back(pB);
    pB->setChecked(pB->mpTAction->mButtonState);
}


void TEasyButtonBar::addActionButtons(TAction* pAction)
{
    // The -1 is needed to compensate for the initial pre-increment to TEasyButtonBar::mItemCount
    resetItemCount(pAction->getButtonFillerOffset() - 1);
    for (auto* pTActionNode : *pAction->mpMyChildrenList) {
        auto* pTAction = static_cast<TAction*>(pTActionNode);
        if (!pTAction->isActive()) {
            continue;
        }
        const QIcon icon(pTAction->getIcon());
        const QString name = pTAction->getName();
        auto pTFlipButton = new TFlipButton(pTAction, pAction->mpHost);
        pTFlipButton->setIcon(icon);
        pTFlipButton->setText(name);
        pTFlipButton->setCheckable(pTAction->isPushDownButton());

        if (pTAction->isPushDownButton()) {
            pTFlipButton->setChecked(pTAction->mButtonState);
        } else {
            // The following was added to ensure a non-Pushdown button is never
            // left in a checked state - Slysven
            pTFlipButton->setChecked(false);
        }

        pTFlipButton->setFlat(pAction->getButtonFlat());
        // This applies the CSS for THIS TAction to a CHILD's representation on the Toolbar
        pTFlipButton->setStyleSheet(pAction->css);

        //FIXME: Heiko April 2012: only run checkbox button scripts, but run them even if unchecked
        if (pTAction->isPushDownButton() && pAction->mpHost->mIsProfileLoadingSequence) {
            qDebug() << "addActionButtons() name=" << pTAction->getName() << " executing script";
            pTAction->execute();
        }


        if (pTAction->isFolder()) {
            auto pNewMenu = new QMenu(pTFlipButton);

            // This applied the CSS for THIS TAction to a CHILD's own menu - is this right
            // CHECK: consider using the Child's CSS instead for a menu on it
            // - Slysven:
            // pNewMenu->setStyleSheet( pTAction->css );
            pNewMenu->setStyleSheet(pAction->css);

            fillMenu(pTAction, pNewMenu);

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
        addButton(pTFlipButton);
    }
}

// This seems to be the second half of TEasyButtonBar version of:
//   TToolBar::addActionToMenu( TAction *, QMenu * )
// the need for the split is not yet clear to me! - Slysven
void TEasyButtonBar::fillMenu(TAction* pAction, QMenu* pMenu)
{
    for (auto* pTActionNode : *pAction->mpMyChildrenList) {
        auto* pTAction = static_cast<TAction*>(pTActionNode);
        if (!pTAction->isActive()) {
            continue;
        }
        pAction->mpEasyButtonBar = this;
        auto pEAction = new EAction(pAction->mpHost, QIcon(pAction->getIcon()), pTAction->getName(), pTAction->mID);
        pEAction->setStatusTip(pTAction->getName());
        pEAction->setCheckable(pTAction->isPushDownButton());
        if (pTAction->isPushDownButton()) {
            pEAction->setChecked(pTAction->mButtonState);
        } else {
            pEAction->setChecked(false);
        }

        if (pTAction->mpEAction) {
            pTAction->mpEAction->deleteLater();
        }
        pTAction->mpEAction = pEAction;

        //FIXME: Heiko April 2012 -> addActionButtons()
        if (pTAction->isPushDownButton() && pAction->mpHost->mIsProfileLoadingSequence) {
            pTAction->execute();
        }

        if (pTAction->isFolder()) {
            // Adding a QWidget derived pointer to new QMenu() means the menu
            // will be destroyed when the pointed to item is, we just need to
            // find the item that it is attached to - ah ha, try the toolbar...
            auto pNewMenu = new QMenu(this);
            pEAction->setMenu(pNewMenu);

            // CHECK: consider using the Child's CSS instead for a menu on it
            // - Slysven:
            // pNewMenu->setStyleSheet( pTAction->css );
            pNewMenu->setStyleSheet(pAction->css);

            fillMenu(pTAction, pNewMenu);
        }

        // Menu is PARENT'S menu pEAction, this line moved to be AFTER child builds its own menu if it is a folder
        pMenu->addAction(pEAction);
    }
}


void TEasyButtonBar::finalize()
{
    if (mpTAction->mUseCustomLayout || !mpTAction->getButtonFillerOffset()) {
        return;
    }
    auto fillerWidget = new QWidget(this);
    QPushButton dummy;
    fillerWidget->setMinimumSize(dummy.minimumSizeHint());
    fillerWidget->setMaximumSize(dummy.minimumSizeHint());
    if (mpLayout) {
        if (mpTAction->mOrientation == 1) {
            // The toolbar is to be filled with rows of mpTAction->getButtonColumns() wide
            // The filler widget is to be one or more columns wide
            mpLayout->addWidget(fillerWidget, 0, 0, mpTAction->getButtonFillerOffset(), 1);
        } else {
            // The toolbar is to be filled with columns of mpTAction->getButtonColumns() tall
            // The filler widget is to be one or more rows tall
            mpLayout->addWidget(fillerWidget, 0, 0, 1, mpTAction->getButtonFillerOffset());
        }
    }
}

// Used by buttons directly on an TEasyButtonBar instance - we now retrieve the
// button state to ensure the visible representation is used.
void TEasyButtonBar::slot_pressed(const bool isChecked)
{
    auto* pB = dynamic_cast<TFlipButton*>(sender());
    if (!pB) {
        return;
    }

    TAction* pA = pB->mpTAction;

    // Hold off ActionUnit deletes for this whole slot: showMenu() below blocks in
    // a modal event loop in which a menu item's script (or inbound game data) can
    // uninstall pA's own package. beginProcessing() keeps that delete deferred -
    // even against a Host catch-all doCleanup() firing at depth 0 mid-loop - so pA
    // survives every dereference here; the scope guard then flushes once, after pA
    // is no longer touched (see ActionUnit::uninstall()):
    ActionUnit* pActionUnit = pA->mpHost->getActionUnit();
    pActionUnit->beginProcessing();
    const auto processingGuard = qScopeGuard([pActionUnit] {
        pActionUnit->endProcessing();
        pActionUnit->doCleanup();
    });

    // NOTE: This function blocks until an item is selected from the menu, and,
    // as the action to "pop-up" the menu is the same as "buttons" use to
    // perform their command/scripts is why "commands" are (no longer) permitted
    // on a "menu".  It also means that the script for a "menu" is run every
    // time it is "clicked" upon to display the pop-up containing the menu
    // entries...
    pB->showMenu();

    if (pA->isPushDownButton()) {
        // DO NOT MANIPULATE THE BUTTON STATE OURSELF NOW
        pA->mButtonState = isChecked;
        pA->mpHost->mpConsole->mButtonState = (pA->mButtonState ? 2 : 1);
    } else {
        pA->mButtonState = false;                // Forces a fixup if not correct
        pB->setChecked(false);                   // This does NOT invoke the clicked() signal!
        pA->mpHost->mpConsole->mButtonState = 1; // Was effectively 0 but that is wrong
    }

    pA->execute();
}

void TEasyButtonBar::clear()
{
    auto pW = new QWidget;
    for (auto& flipButton : mButtonList) {
        disconnect(flipButton, &QAbstractButton::clicked, this, &TEasyButtonBar::slot_pressed);
    }
    mButtonList.clear();
    // Transfer the object name to the new instance:
    auto widgetObjectName(mpWidget->objectName());
    mpWidget->setObjectName(QString());
    mpWidget->deleteLater();
    mpWidget = pW;
    mpWidget->setObjectName(widgetObjectName);

    if (!mpTAction->mUseCustomLayout) {
        mpLayout = new QGridLayout;
        mpWidget->setLayout(mpLayout);
        mpLayout->setContentsMargins(0, 0, 0, 0);
        mpLayout->setSpacing(0);
        const QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mpWidget->setSizePolicy(sizePolicy);

        mpWidget->setContentsMargins(0, 0, 0, 0);
    } else {
        mpLayout = nullptr;
        mpWidget->setMinimumSize(mpTAction->getSize());
        mpWidget->setMaximumSize(mpTAction->getSize());
        mpWidget->setGeometry(mpTAction->mPosX, mpTAction->mPosY, mpTAction->getSizeX(), mpTAction->getSizeY());
    }
    layout()->addWidget(pW);
    setStyleSheet(mpTAction->css);
    mpWidget->setStyleSheet(mpTAction->css);
}
