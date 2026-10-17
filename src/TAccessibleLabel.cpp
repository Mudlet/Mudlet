/***************************************************************************
 *   Copyright (C) 2026 by Michael Conley - sousesider@gmail.com           *
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

#include "TAccessibleLabel.h"

#include "TLabel.h"

TAccessibleLabel::TAccessibleLabel(QWidget* widget)
: QAccessibleWidget(widget, QAccessible::StaticText)
{
}

TLabel* TAccessibleLabel::label() const
{
    return static_cast<TLabel*>(widget());
}

bool TAccessibleLabel::actsAsButton() const
{
    return label()->model().actsAsButton();
}

QAccessible::Role TAccessibleLabel::role() const
{
    if (actsAsButton()) {
        return QAccessible::Button;
    }
    // As Qt's own interface for a QLabel reports them
    if (label()->movie()) {
        return QAccessible::Animation;
    }
    return label()->showsImage() ? QAccessible::Graphic : QAccessible::StaticText;
}

QString TAccessibleLabel::text(QAccessible::Text t) const
{
    if (t == QAccessible::Name) {
        return label()->model().effectiveAccessibleName();
    }
    // A tooltip serving as the name would otherwise be read a second time as the description
    if (t == QAccessible::Description && label()->model().namedByToolTip()) {
        return {};
    }
    return QAccessibleWidget::text(t);
}

QStringList TAccessibleLabel::actionNames() const
{
    return actsAsButton() ? QStringList{pressAction()} : QAccessibleWidget::actionNames();
}

void TAccessibleLabel::doAction(const QString& actionName)
{
    if (actionName == pressAction() && actsAsButton()) {
        label()->press();
        return;
    }
    QAccessibleWidget::doAction(actionName);
}

/*static*/ QAccessibleInterface* TAccessibleLabel::labelFactory(const QString& classname, QObject* object)
{
    // pInterface, not interface: the latter is a macro on MinGW
    QAccessibleInterface* pInterface = nullptr;
    if (classname == QLatin1String("TLabel") && object && object->isWidgetType()) {
        pInterface = new TAccessibleLabel(static_cast<QWidget*>(object));
    }
    return pInterface;
}
