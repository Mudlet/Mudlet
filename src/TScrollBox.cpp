/***************************************************************************
 *   Copyright (C) 2021 by Manuel Wegmann - wegmann.manuel@yahoo.com       *
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


#include "TScrollBox.h"
#include "mudlet.h"

#include "pre_guard.h"
#include <QtEvents>
#include "post_guard.h"


TScrollBox::TScrollBox(Host* pH, QWidget* pW)
: QScrollArea(pW)
, mpHost(pH)
{
    setWidget(new TScrollBoxWidget());
    setWidgetResizable(false);
    widget()->resize(parentWidget()->size());
}


TScrollBoxWidget::TScrollBoxWidget(QWidget* pW) : QWidget(pW) {}
TScrollBoxWidget::~TScrollBoxWidget() {}

void TScrollBoxWidget::childEvent(QChildEvent* event)
{
    auto child = event->child();
    if (event->added())
    {
        child->installEventFilter(this);
    }
    if (event->removed())
    {
        child->removeEventFilter(this);
    }
}

void TScrollBoxWidget::setMinSize()
{
    QSize childrenSize(childrenRect().bottomRight().x(), childrenRect().bottomRight().y());
    int widthMargin = -2;
    int heightMargin = -2;
    bool changeWidth = childrenRect().bottomRight().x() >= parentWidget()->rect().bottomRight().x();
    bool changeHeight = childrenRect().bottomRight().y() >= parentWidget()->rect().bottomRight().y();
    if (changeWidth) {
        resize(childrenSize.width() + widthMargin, height());
    }
    if (changeHeight) {
        resize(width(), childrenSize.height() + heightMargin);
    }
    if (!changeWidth && !changeHeight) {
        resize(parentWidget()->size());
    }
}

bool TScrollBoxWidget::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object);
    if (event->type() == QMoveEvent::Move || event->type() == QResizeEvent::Resize || event->type() == QHideEvent::Hide || event->type() == QShowEvent::Show)
    {
      setMinSize();
    }
    return false;
}
