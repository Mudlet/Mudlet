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

#include "Host.h"
#include "TAction.h"
#include "TConsole.h"
#include "TFlipButton.h"

#include "pre_guard.h"
#include <QGridLayout>
#include "post_guard.h"


TEasyButtonBar::TEasyButtonBar(TAction* pA, QString name, QWidget* pW)
: QWidget(pW)
, mpTAction(pA)
, mVerticalOrientation(false)
, mpWidget(new QWidget(this))
, mRecordMove(false)
, mpLayout(nullptr)
, mItemCount(0)
{
    mButtonList.clear();
    auto hostName(pA->mpHost->getName());
    auto layout = new QVBoxLayout;
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(mpWidget);
    if (!mpTAction->mUseCustomLayout) {
        mpLayout = new QGridLayout(mpWidget);
        setContentsMargins(0, 0, 0, 0);
        mpLayout->setContentsMargins(0, 0, 0, 0);
        mpLayout->setMargin(0);
        mpLayout->setSpacing(0);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mpWidget->setSizePolicy(sizePolicy);
    } else {
        mpWidget->setMinimumHeight(mpTAction->mSizeY);
        mpWidget->setMaximumHeight(mpTAction->mSizeY);
        mpWidget->setMinimumWidth(mpTAction->mSizeX);
        mpWidget->setMaximumWidth(mpTAction->mSizeX);
        mpWidget->setGeometry(mpTAction->mPosX, mpTAction->mPosY, mpTAction->mSizeX, mpTAction->mSizeY);
    }
    setStyleSheet(mpTAction->css);
    mpWidget->setStyleSheet(mpTAction->css);
    setObjectName(QStringLiteral("easyButtonBar_%1_%2").arg(hostName, name));
    mpWidget->setObjectName(QStringLiteral("easyButtonBar_Widget_%1_%2").arg(hostName, name));
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
        QSize size = QSize(pB->mpTAction->mSizeX, pB->mpTAction->mSizeY);
        pB->setMaximumSize(size);
        pB->setMinimumSize(size);
        pB->setParent(mpWidget);
        pB->setGeometry(pB->mpTAction->mPosX, pB->mpTAction->mPosY, pB->mpTAction->mSizeX, pB->mpTAction->mSizeY);
    }

    pB->setStyleSheet(pB->mpTAction->css);
    pB->setFlat(pB->mpTAction->getButtonFlat());
    int rotation = pB->mpTAction->getButtonRotation();
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
        int columns = mpTAction->getButtonColumns();
        if (columns <= 0) {
            columns = 1;
        }
        if (columns > 0) {
            mItemCount++;
            int row = mItemCount / columns;
            int col = mItemCount % columns;
            if (mVerticalOrientation) {
                mpLayout->addWidget(pB, row, col);
            } else {
                mpLayout->addWidget(pB, col, row);
            }
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


void TEasyButtonBar::finalize()
{
    if (mpTAction->mUseCustomLayout) {
        return;
    }
    auto fillerWidget = new QWidget;

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    fillerWidget->setSizePolicy(sizePolicy);
    int columns = mpTAction->getButtonColumns();
    if (columns <= 0) {
        columns = 1;
    }
    int row = (++mItemCount) / columns;
    int column = mItemCount % columns;
    if (mpLayout) {
        mpLayout->addWidget(fillerWidget, row, column);
    }
}

// Used by buttons directly on an TEasyButtonBar instance - we now retrieve the
// button state to ensure the visible representation is used.
void TEasyButtonBar::slot_pressed(const bool isChecked)
{
    auto * pB = dynamic_cast<TFlipButton*>(sender());
    if (!pB) {
        return;
    }

    TAction* pA = pB->mpTAction;

    // NOTE: This function blocks until an item is selected from the menu, and,
    // as the action to "pop-up" the menu is the same as "buttons" use to
    // perform their command/scripts is why "commands" are (no longer) permitted
    // on a "menu".  It also means that the script for a "menu" is run every
    // time it is "clicked" upon to display the pop-up containing the menu
    // entries...
    pB->showMenu();

    if (pA->mIsPushDownButton) {
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
        mpLayout->setMargin(0);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        mpWidget->setSizePolicy(sizePolicy);

        mpWidget->setContentsMargins(0, 0, 0, 0);
        mpLayout->setMargin(0);
    } else {
        mpLayout = nullptr;
        mpWidget->setMinimumHeight(mpTAction->mSizeY);
        mpWidget->setMaximumHeight(mpTAction->mSizeY);
        mpWidget->setMinimumWidth(mpTAction->mSizeX);
        mpWidget->setMaximumWidth(mpTAction->mSizeX);
        mpWidget->setGeometry(mpTAction->mPosX, mpTAction->mPosY, mpTAction->mSizeX, mpTAction->mSizeY);
    }
    layout()->addWidget(pW);
    setStyleSheet(mpTAction->css);
    mpWidget->setStyleSheet(mpTAction->css);
}
