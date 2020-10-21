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


#include "TToolBar.h"


#include "TAction.h"
#include "TConsole.h"
#include "TFlipButton.h"
#include "mudlet.h"


TToolBar::TToolBar(TAction* pA, const QString& name, QWidget* pW)
: QDockWidget( pW )
, mpTAction( pA )
, mVerticalOrientation( false )
, mpWidget( new QWidget( this ) )
, mRecordMove( false )
, mpLayout( nullptr )
, mItemCount( 0 )
{
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    setWidget(mpWidget);
    // Needs a unique name across application so needs the Host name as well:
    setName(name);

    connect(this, &QDockWidget::dockLocationChanged, this, &TToolBar::slot_dockLocationChanged);
    connect(this, &QDockWidget::topLevelChanged, this, &TToolBar::slot_topLevelChanged);

    if (!mpTAction->mUseCustomLayout) {
        mpLayout = new QGridLayout(mpWidget);
        setContentsMargins(0, 0, 0, 0);
        mpLayout->setContentsMargins(0, 0, 0, 0);
        mpLayout->setSpacing(0);
        mpWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    setStyleSheet(mpTAction->css);
    mpWidget->setStyleSheet(mpTAction->css);
}

void TToolBar::resizeEvent(QResizeEvent* e)
{
    if (!mudlet::self()->mIsLoadingLayout) {
        mudlet::self()->setToolbarLayoutUpdated(mpTAction->mpHost, this);
    }
}

void TToolBar::setName(const QString& name)
{
    mName = name;
    QString hostName(mpTAction->mpHost->getName());
    setObjectName(QStringLiteral("dockToolBar_%1_%2").arg(hostName, name));
    // Actually put something in as the title so that the main window context
    // menu no longer has empty entries which are disabled:
    setWindowTitle(tr("Toolbar - %1 - %2").arg(hostName, name));
}

void TToolBar::moveEvent(QMoveEvent* e)
{
    if (!mpTAction) {
        return;
    }

    if (!mudlet::self()->mIsLoadingLayout) {
        mudlet::self()->setToolbarLayoutUpdated(mpTAction->mpHost, this);
    }

    if (mRecordMove) {
        mpTAction->mPosX = e->pos().x();
        mpTAction->mPosY = e->pos().y();
    }
    e->ignore();
}

void TToolBar::slot_dockLocationChanged(Qt::DockWidgetArea dwArea)
{
    if (mpTAction) {
        mpTAction->mToolbarLastDockArea = dwArea;
    }
}

void TToolBar::slot_topLevelChanged(bool topLevel)
{
    if (mpTAction) {
        mpTAction->mToolbarLastFloatingState = topLevel;
    }
}

void TToolBar::addButton(TFlipButton* pB)
{
    if (!mpTAction->mUseCustomLayout) {
        QSize size = pB->minimumSizeHint();
        if (pB->mpTAction->getButtonRotation() > 0) {
            size.transpose();
        }
        pB->setMaximumSize(size);
        pB->setMinimumSize(size);
    } else {
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

    // Was using pressed() signal but now we want to track the ACTUAL state of
    // the underlying QAbstractButton
    connect(pB, &QAbstractButton::clicked, this, &TToolBar::slot_pressed);
}

void TToolBar::finalize()
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
    int column = (mItemCount - 1) % columns;
    mpLayout->addWidget(fillerWidget, row, column);
    // 3 lines above are to avoid order of operations problem of original line
    // (-Wsequence-point warning on mItemCount) NEEDS TO BE CHECKED:
    //    mpLayout->addWidget( fillerWidget, ++mItemCount/columns, mItemCount%columns );
}

// Used by buttons directly on a TToolBar instance but NOT on sub-menu item - we
// now retrieve the button state to ensure the visible representation is used.
void TToolBar::slot_pressed(const bool isChecked)
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
    pB->menu();

    if (pA->mIsPushDownButton) {
        pA->mButtonState = isChecked;
        pA->mpHost->mpConsole->mButtonState = (pA->mButtonState ? 2 : 1); // Was using 1 and 0 but that was wrong
    } else {
        pA->mButtonState = false;
        pB->setChecked(false);                   // This does NOT invoke the clicked()!
        pA->mpHost->mpConsole->mButtonState = 1; // Was effectively 0 but that is wrong
    }

    pA->execute();
}

void TToolBar::clear()
{
    auto pW = new QWidget(this);
    setWidget(pW);
    mpWidget->deleteLater();
    mpWidget = pW;

    if (!mpTAction->mUseCustomLayout) {
        mpLayout = new QGridLayout(mpWidget);
        mpLayout->setContentsMargins(0, 0, 0, 0);
        mpLayout->setSpacing(0);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        mpWidget->setSizePolicy(sizePolicy);
    } else {
        mpLayout = nullptr;
    }
    setStyleSheet(mpTAction->css);
    mpWidget->setStyleSheet(mpTAction->css);

    mudlet::self()->removeDockWidget(this);
}
