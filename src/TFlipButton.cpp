/***************************************************************************
 *   Copyright (C) 2009 by Heiko Koehn - KoehnHeiko@googlemail.com         *
 *   Copyright (C) 2014 by Ahmed Charles - acharles@outlook.com            *
 *   Copyright (C) 2017, 2922 by Stephen Lyons - slysven@virginmedia.com   *
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


#include "TFlipButton.h"


#include "Host.h"
#include "TAction.h"
#include "TEasyButtonBar.h"
#include "TToolBar.h"

#include "pre_guard.h"
#include <QMenu>
#include <QStyleOptionButton>
#include <QStylePainter>
#include "post_guard.h"

TFlipButton::TFlipButton(TAction* pTAction, Host* pHost)
: QPushButton(nullptr)
, mpTAction(pTAction)
, mID(pTAction->getID())
, mpHost(pHost)
{
    // This should make it easier to track the button within the GammaRay tool!
    setObjectName(qsl("flipButton_%1_%2").arg(mpHost->getName(), QString::number(mID)));
}

Qt::Orientation TFlipButton::orientation() const
{
    return mOrientation;
}

void TFlipButton::setOrientation(Qt::Orientation orientation)
{
    mOrientation = orientation;
    switch (orientation) {
    case Qt::Horizontal:
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        break;

    case Qt::Vertical:
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        break;
    }
}

bool TFlipButton::mirrored() const
{
    return mMirrored;
}

void TFlipButton::setMirrored(bool mirrored)
{
    mMirrored = mirrored;
}

QSize TFlipButton::sizeHint() const
{
    QSize size = QPushButton::sizeHint();
    if (mOrientation == Qt::Vertical) {
        size.transpose();
    }
    return size;
}

QSize TFlipButton::minimumSizeHint() const
{
    QSize size = QPushButton::minimumSizeHint();
    if (mOrientation == Qt::Vertical) {
        size.transpose();
    }
    return size;
}

void TFlipButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);

    switch (mOrientation) {
    case Qt::Horizontal:
        if (mMirrored) {
            painter.rotate(180);
            painter.translate(-width(), -height());
        }
        break;

    case Qt::Vertical:
        if (mMirrored) {
            painter.rotate(-90);
            painter.translate(-height(), 0);
        } else {
            painter.rotate(90);
            painter.translate(0, -width());
        }
        break;
    }

    painter.drawControl(QStyle::CE_PushButton, getStyleOption());
}

QStyleOptionButton TFlipButton::getStyleOption() const
{
    QStyleOptionButton opt;
    opt.initFrom(this);
    if (mOrientation == Qt::Vertical) {
        QSize size = opt.rect.size();
        size.transpose();
        opt.rect.setSize(size);
    }
    opt.features = QStyleOptionButton::None;
    if (menu()) {
        opt.features |= QStyleOptionButton::HasMenu;
    }
    if (isDown() || (menu() && menu()->isVisible())) {
        opt.state |= QStyle::State_Sunken;
    }
    if (isChecked()) {
        opt.state |= QStyle::State_On;
    }
    opt.text = text();
    opt.icon = icon();
    opt.iconSize = iconSize();
    return opt;
}
